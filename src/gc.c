#include <nscheme/vm.h>
#include <nscheme/values.h>

enum block_flags {
	FLAG_MARKED = 1 << 0,
};

typedef struct scm_gc_block {
	union {
		// lowest 4 bits of the pointer field to be used for GC state flags
		void *ptr;
		uintptr_t flags;
	};

	size_t size;
} scm_gc_block_t;

void *vm_alloc(vm_t *vm, size_t n) {
	void *ret = NULL;

	if ((ret = gc_alloc(&vm->gc, n)) == NULL) {
		gc_collect_vm(&vm->gc, vm);

		if ((ret = gc_alloc(&vm->gc, n)) == NULL) {
			vm_error(vm, "allocation failure! heap is full (TODO: expand heap)");
		}
	}

	return ret;
}

void gc_init(vm_gc_context_t *gc, size_t initial_size) {
	// TODO: need to make sure pointers are aligned to 16
	gc->base         = malloc(initial_size);
	gc->end          = gc->base + initial_size;
	gc->allocend     = gc->base;
	gc->current_mark = 1;
}

void *gc_alloc(vm_gc_context_t *gc, size_t n) {
	size_t required = n + sizeof(scm_gc_block_t);

	if (gc->allocend + required >= gc->end) {
		// allocation failure, need to run the collector 
		return NULL;
	}

	scm_gc_block_t *ret = (void*)gc->allocend;
	gc->allocend += required;
	// unmarked by default
	ret->flags = !gc->current_mark;

	// TODO: need to make sure pointers are aligned to 16
	return (uint8_t*)ret + sizeof(scm_gc_block_t);
}

static inline bool is_heap_type(scm_value_t val) {
	return (val & SCM_MASK_INTEGER) != 0 && (val & SCM_MASK_RUN_TYPE) != 0xff;
}

static inline
void gc_mark_value(vm_gc_context_t *gc, scm_value_t val) {
	uint8_t *ptr = get_heap_tagged_value(val);
	scm_gc_block_t *blk = (void*)(ptr - sizeof(scm_gc_block_t));
	blk->flags = (blk->flags & ~FLAG_MARKED) | (blk->flags ^ (gc->current_mark));
}

static void mark_traverse(vm_gc_context_t *gc, scm_value_t val) {
	if (is_heap_type(val)) {
		switch (get_heap_type(val)) {
			case SCM_TYPE_PAIR: {
				scm_pair_t *pair = get_heap_tagged_value(val);
				gc_mark_value(gc, val);
				mark_traverse(gc, pair->car);
				mark_traverse(gc, pair->cdr);
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
	}
}

size_t gc_collect_vm(vm_gc_context_t *gc, vm_t *vm) {
	return 0;
}
