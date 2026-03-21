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
- Lazily (wip) Interpreted Language
- Dynamic Typing
- N arity functions
- First class functions and function closures
- Primitives: 
    - if, lambda, define, force (kinda)
- Functions (coded in c): 
    - Addition, Multiplication, Subtraction, Division
    - Comparison operators
    - (yeah that's it)
- Functions (coded in Jayva):
    - compose, apply
    - (Yeah there's not much)

# Future Work:
## Basic Work:
- [x] make header files for Interpret.c
- [ ] make some standard functions for running code
- [x] maybe also a way to read a file so they don't have to always use string literals in c
- [ ] make a command line way to run thats fancy and all
- [ ] and an easier interface to combine the c and jayva code so i can use this in some basic boring dumb annoying terrible evil stupid pointless simple useless school project
- [ ] raylib speedrun?
- [ ] fixing random syntax error errors like:
    - ```lisp
        (force ((id id) 1)) ; i think it doesn't like the ((fun))
        (define const-unit (lambda (x) 'unit)) ; it is ok with (id 'unit) so i guess the raw value is bad
        (force (apply (compose (lambda (x) (spy x 1)) add1) 1)) ;idk why this fails, when you switch lambda and add1 it works. maybe because of raw value thing above?
        ```
- [ ] comments!!!!!!!!!!!!!!!
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
- [x] Function closures extend namespace state
    - Lowkey forgot why i had a recursive namespace until writing this
    - But I am still a genius so its ok
- [ ] Lowlevel namespace functions (in case you for some reason want to do something wierd)
    - I did add access to the Namespace and Env so it wouldn't be hard to implement
- [ ] fix laziness
    - currently laziness is all or nothing, which is pretty pointless
    - since its basically the program runs or doesn't run
    - ideally force should still allow a lazy return value
    - since it should only force the statement and not its params
    - so lazyness would have to propogate through 
    - however this requires some type of reflection to allow for the reformation of the syntax tree
    - i wonder if it would be possible to use some extern magic to have crazy recursision
    - where Value is part of the Syntax tagged union so that we don't have to fully unpack and repack
## Language
- [x] Add more functions than just addition
    - [ ] But string functions?
- [ ] make it easier to define functions
    - Like from c interface (probably impossible to be easier because c is only a toy language)
- [ ] Currying through macros
- [ ] Basic list, pair, and option data structure
    - Man I wish I had a type system
- [ ] Box type would be good too
- [ ] Traits (yeah this is probably going nowhere)
- [ ] And maybe type system ?!!??!
    - I think I would have to scrap most of the interpreter to add types
    - but it's probably a good idea to do so, if I want to take this project farther
    - but who knows
    - maybe at a later time since it really seems like a lot of extra stuff
    - especially when the base isn't the strongest
## Error Handling
- [ ] Unify all error types
- [ ] Provide info on error location
- [ ] Provide call stack info