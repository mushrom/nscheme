#include <nscheme/vm.h>
#include <nscheme/values.h>

#include <stdio.h>

enum block_flags {
	FLAG_MARKED = 1 << 0,
	FLAG_GREY   = 1 << 1,
};

typedef struct scm_gc_block {
	union {
		// lowest 4 bits of the pointer field to be used for GC state flags
		void *ptr;
		uintptr_t uintptr;
	};

	// TODO: bitfield, 4 bits or so for flags, rest for size
	size_t flags;
	size_t size;
} scm_gc_block_t;

void *vm_alloc(vm_t *vm, size_t n) {
	void *ret = NULL;

	//return calloc(1, n);

	if ((ret = gc_alloc(&vm->gc, n)) == NULL) {
		puts("Starting garbage collection...");
		gc_collect_vm(&vm->gc, vm);

		if ((ret = gc_alloc(&vm->gc, n)) == NULL) {
			vm_panic(vm, "allocation failure! heap is full (TODO: expand heap)");

		} else {
			puts("Allocated successfully?");
		}
	}

	return ret;
}

size_t align_size(size_t size, size_t align) {
	size_t off = size % align;
	return size + (off > 0)*(align - off);
}

void *align_ptr(void *ptr, size_t align) {
	uintptr_t temp = (uintptr_t)ptr;
	size_t off = temp % align;
	return (void *)(temp + (off > 0)*(align - off));
}

void gc_init(vm_gc_context_t *gc, size_t initial_size) {
	gc->base         = malloc(initial_size);
	gc->end          = gc->base + initial_size;
	gc->allocend     = align_ptr(gc->base, 16);

	printf("%p, %p\n", gc->base, gc->allocend);
}

void *gc_alloc(vm_gc_context_t *gc, size_t n) {
	// adjust offset of GC block so that the end of the block has 16 byte alignment
	uint8_t *block_end = align_ptr(gc->allocend + sizeof(scm_gc_block_t), 16);

	if (block_end + n >= gc->end) {
		// allocation failure, need to run the collector 
		return NULL;
	}

	gc->allocend = block_end + n;
	scm_gc_block_t *block = (void*)(block_end - sizeof(scm_gc_block_t));
	block->flags = 0; // unmarked by default
	block->size  = n;

	return block_end;
}

static inline bool is_heap_type(scm_value_t val) {
	return (val & SCM_MASK_INTEGER) != 0 && (val & SCM_MASK_RUN_TYPE) != 0xff;
}

// check whether the pointer is owned by the garbage collector
// (there might be externally owned pointers referenced places)
static inline bool is_gc_ptr(vm_gc_context_t *gc, void *ptr) {
	uint8_t *temp = ptr;
	return temp >= gc->base && temp < gc->allocend;
}

static inline
scm_gc_block_t *gc_get_block(void *ptr) {
	return (void *)((uint8_t*)ptr - sizeof(scm_gc_block_t));
}

static inline
void gc_mark_pointer(vm_gc_context_t *gc, void *ptr) {
	if (is_gc_ptr(gc, ptr)) {
		scm_gc_block_t *blk = gc_get_block(ptr);
		blk->flags |= FLAG_MARKED;
	}
}

static inline
void gc_mark_value(vm_gc_context_t *gc, scm_value_t val) {
	if (is_heap_type(val)) {
		gc_mark_pointer(gc, get_heap_tagged_value(val));
		printf("marking value: %lx\n", val);
	}
}

static inline
void link_end(scm_value_t *end, scm_value_t next) {
	scm_gc_block_t *blk = gc_get_block(get_heap_tagged_value(*end));
	scm_gc_block_t *nextblk = gc_get_block(get_heap_tagged_value(next));

	blk->uintptr = next;
	nextblk->flags = FLAG_GREY;
	nextblk->uintptr = 0;
	*end = next;
}

static inline
scm_value_t link_next(scm_value_t val) {
	if (val) {
		scm_gc_block_t *blk = gc_get_block(get_heap_tagged_value(val));
		return blk->uintptr;
	} else {
		return 0;
	}
}

static inline
bool is_marked(scm_value_t val) {
	scm_gc_block_t *blk = gc_get_block(get_heap_tagged_value(val));
	return blk->flags & FLAG_MARKED;
}

static inline
bool is_unmarked_ptr(void *ptr) {
	scm_gc_block_t *blk = gc_get_block(ptr);
	return blk->flags == 0;
}

static inline
bool is_unmarked(scm_value_t val) {
	return is_unmarked_ptr(get_heap_tagged_value(val));
}

static inline
bool should_link(vm_gc_context_t *gc, scm_value_t val) {
	return is_heap_type(val)
	    && is_gc_ptr(gc, get_heap_tagged_value(val))
	    && is_unmarked(val)
	    ;
}

static inline
void try_link(vm_gc_context_t *gc, scm_value_t *end, scm_value_t next) {
	if (should_link(gc, next)) {
		link_end(end, next);
	}
}

static void mark_traverse(vm_gc_context_t *gc, scm_value_t root) {
	if (!should_link(gc, root)) {
		// nothing to do here (external pointers have no block)
		// TODO: second recursive marker to trace external pointers
		return;
	}

	scm_value_t cur = root;
	scm_value_t end = root;

	while (cur != 0) {
		gc_mark_value(gc, cur);

		// should only have heap types here, other types
		// won't contain references to other values
		switch (get_heap_type(cur)) {
			case SCM_TYPE_PAIR: {
				scm_pair_t *pair = get_heap_tagged_value(cur);
				try_link(gc, &end, pair->car);
				try_link(gc, &end, pair->cdr);
				break;
			}

			case SCM_TYPE_VECTOR:
			case SCM_TYPE_STRING:
			case SCM_TYPE_CLOSURE:
			case SCM_TYPE_SYMBOL:
			case SCM_TYPE_SYNTAX_RULES:
			case SCM_TYPE_EXTERNAL_PTR:
			case SCM_TYPE_BIG_NUMBER:
			default:
				break;
		}

		cur = link_next(cur);
	}
}

static void mark_closure(vm_gc_context_t *gc, scm_closure_t *clsr) {
	mark_traverse(gc, clsr->definition);
	gc_mark_pointer(gc, clsr->closures);
	// TODO: mark other
}

static void mark_env_node(vm_gc_context_t *gc, env_node_t *node) {
	if (node && is_unmarked_ptr(node)) {
		puts("huh?");
		gc_mark_pointer(gc, node);
		mark_traverse(gc, node->key);
		mark_traverse(gc, node->value);
		// TODO: should be using a balanced tree for environment
		//       nodes, so stack depth shouldn't be an issue
		mark_env_node(gc, node->left);
		mark_env_node(gc, node->right);
	}
}

static void mark_environment(vm_gc_context_t *gc, environment_t *env) {
	for (environment_t *cur = env; env && is_unmarked_ptr(env); env = env->last) {
		gc_mark_pointer(gc, cur);
		mark_env_node(gc, cur->root);
	}
}

static void mark_vm(vm_gc_context_t *gc, vm_t *vm) {
	printf("got here too? %u, %u\n", vm->sp, vm->callp);
	for (unsigned i = 0; i < vm->sp; i++) {
		mark_traverse(gc, vm->stack[i]);
		printf("asdf: %u\n", i);
	}

	for (unsigned i = 0; i < vm->callp; i++) {
		if (vm->calls[i].runmode == false /* TODO: 'INTERPRETED' constant */) {
			mark_traverse(gc, vm->calls[i].ptr);
			// TODO: mark environment
		}

		mark_closure(gc, vm->calls[i].closure);
	}

	mark_environment(gc, vm->env);
	mark_traverse(gc, vm->ptr);
}

size_t gc_collect_vm(vm_gc_context_t *gc, vm_t *vm) {
	puts("got here");
	mark_vm(gc, vm);
	return 0;
}
