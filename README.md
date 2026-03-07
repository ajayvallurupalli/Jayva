Introducing the JAYVA language, my own toy language.  

# Work Completed:
## Parser
- Generalized context-free and context-full parser
    - based on Free Selective Applicative Parsers in Haskell
    - (As they say, c is the new Haskell)
    - see [paper](https://dl.acm.org/doi/10.1145/3341694)
## Tokenizer
- Pretty simple Tokenizer written with aformentioned parser
## Abstract Syntax Tree Builder
- Transforms Tokens into Abstract Syntax Tree
- Complex Recursive Schemes (idk if theres a point to doing so but its cool)
## Macro System
- Allows Runtime macros to act on AST
- Gives ability to cull or expand AST before execution
## Interpreting
- Lazily Interpreted Language
- Dynamic Typing
- N arity variables
- First class functions
- Primitives: 
    - if, lambda, 
- Functions (coded in c): 
    - Addition, Multiplication, Subtraction, Division
    - Comparison operators
    - (yeah that's it)

# Future Work:
## Basic Work:
- [ ] make header files for Interpret.c
- [ ] make some standard functions for running code
- [ ] maybe also a way to read a file so they don't have to always use string literals in c
## Memory: 
- [ ] freeing memory after interpreting (i know, i should have done this earlier)
    - should be pretty simple I just haven't bothered since i've been blitzing the basic primitives
- [ ] Maybe looking where memory can be freed during execution
    - probably in most cases its safe (other than for functions, it should be safe)
- [ ] Learn how to actually track memory usage in c so I know how performant it is
## Parser:
- [ ] Static Analysis and Optimisation
    - Currently parsing a simple fibonacci sequence implemention branches 505 times which is lowkey crazy
    - Should be able to improve performance with factoring out Alt Algebras
## Interpreter
- [x] Finish define and lambda primitives
- [ ] Allow runtime macro execution
    - Should be pretty simple to do with how the language is so lazy
- [ ] Function closures extend namespace state
    - Lowkey forgot why i had a recursive namespace until writing this
    - But I am still a genius so its ok
- [ ] Lowlevel namespace functions (in case you for some reason want to do something wierd)
## Language
- [x] Add more functions than just addition
    - [ ] But string functions?
- [ ] make it easier to define functions
    - Like from c interface (probably impossible to be easier because c is only a toy language)
- [ ] Currying through macros
- [ ] Basic list, pair, and option data structure
    - Man I wish I had a type system
- [ ] Traits
- [ ] And maybe type system ?!!??!
    - I think I would have to scrap most of the interpreter to add types
    - but it's probably a good idea to do so, if I want to take this project farther
    - but who knows
## Error Handling
- [ ] Unify all error types
- [ ] Provide info on error location
- [ ] Provide call stack info