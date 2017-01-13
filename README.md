nscheme readme
==============

A fast, stack-based, JIT-compiling, concurrency-friendly scheme interpreter.
Currently a work in progress, it doesn't do much yet.

To build, type `make`, and that's it for now. A ./configure script and testing
framework will be coming soon.

## TODO
- [ ] parser
    - [x] basic lexer+parser
    - [x] global symbol table
        - [ ] add locking once multithreading is implemented
    - [x] handle basic types (ints, characters, null, ...)
    - [ ] handle unicode
        - [ ] generic utf* handling or just utf8?
    - [x] handle quoted forms
    - [ ] handle dotted form pairs, (a . b)
    - [ ] handle vectors
    - [ ] handle strings
    - [ ] handle bytevectors
- [ ] I/O
    - [x] `write` primitive (note: currently named `display`)
    - [x] `read` primitive 
    - [ ] implement port handles
        - [ ] implement a generic port structure to handle I/O for many different
              sources eg. sockets, files, memory buffers
        - [ ] handle port argument for I/O primitives display, write, read, ...
        - [ ] Implement open/close procedures for different sources
- [ ] virtual machine
    - [x] basic stack-based structure for vm
    - [ ] tree walker interpreter
        - [x] some basic forms (lambda, if, define, top-level begin)
        - [ ] expression-like begin
        - [ ] tail recursion (note: may not be needed if all lambdas are JIT'd on first call)
        - [ ] syntax expansion
    - [ ] JIT compiler
        - [x] Tail recursion
        - [ ] Scope analysis pass
            - [ ] handle local (define ...) forms
                - [x] single local variable definitions
                - [ ] function definition form
            - [ ] handle expression-like begin forms that generate new scope
        - [ ] handle nested lambdas
        - [ ] syntax expansion
        - [ ] optimizations
            - [ ] common subexpression elimination
            - [ ] dead code elimination
            - [ ] optimizing code which is guaranteed to have no side effects
            - [ ] inline small functions
                - will be especially useful for syntax extensions
                  which generate lambdas, eg. named lets)
    - [ ] exception handling
        - [ ] implement exception stack in vm struct
        - [ ] add `with-exception-handler` and `error` primitives
    - [ ] module system
        - [ ] figure out what to put here
        - [ ] implement r7rs libraries and associated procedures
    - [ ] garbage collection
        - [ ] implement basic pointer bumping allocation
        - [ ] implement copying garbage collector
        - [ ] figure out a way to make it work with multiple threads
    - [ ] FFI
        - [ ] maybe look at how python does it, or lua or chicken or something
- [ ] command line interface
    - [x] add help output
    - [x] add file argument
    - [ ] allow specifying options like environment, modules, output location as arguments
- [ ] testing
    - [ ] I/O tests
        - [ ] for basic std{in,out}
        - [ ] for network sockets
        - [ ] for memory buffers
    - [ ] stress testing for compiler
    - [ ] unit tests for scheme procedures
