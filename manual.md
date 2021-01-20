# Ce

A simple language with algebraic data types, ownership semantics, and scoped
memory management (i.e. no garbage collection).

- [Install](README.md)
- Manual
    1. [Lexical Rules](TODO)
    2. [Types](TODO)
    3. [Expressions](TODO)
    4. [Statements](TODO)
    5. [Syntax](TODO)
- [Memory management](memory.md)
- [Comparison with other languages](other.md)

# 1. LEXICAL RULES

## Comment

A comment starts with `--` and runs until the end of the line:

```
-- this is a single line comment
```

## Keywords and Symbols

The following keywords are reserved:

```
    arg         -- function argument
    break       -- escape loop statement
    call        -- function invocation
    clone       -- clone function
    else        -- conditional statement
    func        -- function declaration
    if          -- conditional statement
    input       -- input invocation
    loop        -- loop statement
    native      -- native statement
    output      -- output invocation
    pre         -- native pre declaration
    rec         -- type recursive declaration
    return      -- function return
    set         -- assignment statement
    type        -- new type declaration
    var         -- variable declaration
```

The following symbols are valid:

```
    {   }       -- block delimeter
    (   )       -- unit type, unit value, group expression
    ;           -- sequence separator
    :           -- variable, type, function declaration
    ->          -- function type signature
    =           -- variable assignment
    ,           -- tuple separator
    .           -- tuple index, type predicate & discriminator
    \           -- pointer type, upref & dnref operation
    $           -- empty subcase
    !           -- type discriminator
    ?           -- type predicate, unknown initialization
```

## Identifiers

A variable identifier starts with a lowercase letter and might contain letters,
digits, and underscores:

```
i    myCounter    x_10          -- variable identifiers
```

A type identifier starts with an uppercase letter and might contain letters,
digits, and underscores:

```
Int    U32    Tree              -- type identifiers
```

## Integer numbers

A number is a sequence of digits:

```
1    20    300                  -- tuple indexes / Int values
```

Numbers are used in values of [type `Int`](TODO) and in [tuple indexes](TODO):

## Native token

A native token starts with an underscore `_` and might contain letters,
digits, and underscores:

```
_char    _printf    _errno      -- native identifiers
```

A native token may also be enclosed by curly braces `{` and `}` or parenthesis
`(` and `)` to contain any other characters:

```
_(1 + 1)     _{2 * (1+1)}
```

# 2. TYPES

## Unit

The unit type `()` only allows the [single value](TODO) `()`.

## Native

A native type holds external [values from the host language](TODO), i.e.,
values which *Ce* do not create or manipulate directly.

Native type identifiers follow the rules for [native tokens](TODO):

```
_char     _int    _{FILE*}
```

## User

A user type is a [new type](TODO) introduced by the programmer.
A user type holds values created from [subcase constructors](TODO) also
introduced by the programmer.

A user type identifier starts with an uppercase letter:

```
List    Int    Tree
```

The type `Int` is a primitive type that holds [integer values](TODO).

## Tuple

A tuple type holds [compound values](TODO) from a fixed number of other types:
A tuple type identifier is a comma-separated list of types, enclosed in
parentheses:

```
((),())     (Int,(Tree,Tree))
```

## Function

A function type holds a [function](TODO) value and is composed of an input and
output types separated by an arrow `->`:

```
() -> Tree
(List,List) -> ()
```

## Pointer

A pointer type can be applied to any other type with the prefix backslash `\`:

```
\Int    \List
```

# 3. EXPRESSIONS

## Unit

The unit value is the single value of the [unit type](TODO):

```
()
```

## Native expression

A native expression holds a value from a [host language type](TODO):

```
_printf    _(2+2)     _{f(x,y)}
```

## Variable

A variable holds a value of its [type](TODO):

```
i    myCounter    x_10
```

## Tuple and Index

A tuple holds a fixed number of values of a compound [tuple type](TODO):

```
((),False)              -- a pair with () and False
(x,(),y)                -- a triple
```

A tuple index suffix a tuple with a dot `.` and holds the value at the given
position:

```
(x,()).2                -- yields ()
```

## Call, Input & Output

A call invokes a [function](TODO) with the given argument:

```
f ()                    -- f   receives unit     ()
call (id) x             -- id  receives variable x
add (x,y)               -- add receives tuple    (x,y)
```

The prefix keyword `call` can appear on assignments and statements, but not in
the middle of expressions.

The special function `clone` copies the contents of its argument.
If the value is of a recursive type, the copy is also recursive:

```
var y: List = Item Item $List
var x: List = clone y           -- `x` becomes "Item Item $List"
```

Just like a `call`, the `input` & `output` keywords also invoke functions, but
with the purpose of communicating with external I/O devices:

```
input libsdl Delay 2000            -- waits 2 seconds
output libsdl Draw Pixel (0,0)     -- draws a pixel on the screen
```

The supported device functions and associated behaviors dependend on the
platform in use.
The special device `std` works for the standard input & output devices and
accepts any value as argument:

```
var x: Int = input std      -- reads an `Int` from stdin
output std x                -- writes the value of `x` to stdout
```

The `input` & `output` expressions can appear on assignments and statements,
but not in the middle of expressions.

`TODO: input_f, output_f`

## Constructor, Discriminator, Predicate

### Constructor

A constructor creates a value of a [user type](TODO) given one subcase and its
argument:

```
True ()                 -- value of type Bool
False                   -- () is optional
Car (True,())           -- subcase Car holds a tuple
```

### Discriminator

A discriminator accesses the value of a [user type](TODO) as one of its
subcases.
It suffixes the value with a dot `.`, a subcase identifier, and an exclamation
mark `!`:

```
(True ()).True!         -- yields ()

x = Node ($Tree,(),$Tree)
x.Node!.2               -- yields ()
x.$True                 -- error: `x` is a `Node`
```

An error occurs during execution if the discriminated subcase does not match
the actual value.

### Predicate

A predicate checks if the value of a [user type](TODO) is of its given subcase.
It suffixes the value with a dot `.`, a subcase identifier, and a question mark
`?`:

```
type Member {
    Student:   ()
    Professor: ()
}
x = Professor
b = x.Professor?    -- yields True
```

## Pointer up-reference and down-reference

A pointer holds the address of a variable with a value.
An *upref* acquires the address of a variable with the prefix backslash `\`.
A *dnref* recovers the value given an address with the sufix backslash `\`:

```
var x: Int = 10
var y: \Int = \x    -- acquires the address of `x`
output std y\       -- recovers the value of `x`
```

# 4. STATEMENTS

## Type declaration

A type declaration creates a new [user type](TODO).
Each declaration inside the type defines a subcase of it:

```
type Bool {
    False: ()       -- subcase False holds unit value
    True:  ()       -- subcase True  holds unit value
}
```

A recursive type uses a `rec` modifier and always contains the implicit base
empty subcase with the prefix `$´:

```
type rec Tree {
    -- $Tree: ()            -- implicit empty subcase always present
    Node: (Tree,(),Tree)    -- subcase Node holds left subtree, unit value, and right subtree
}
```

The prefix `$` yields the empty subcase of all recursive types, e.g., `$Tree`
is the empty subcase of `Tree`.
The empty subcase can be used in
[constructors, discriminators, or predicates](TODO).

## Variable declaration

A variable declaration intoduces a name of a given type and assigns a value to
it:

```
var x : () = ()                  -- `x` of type `()` holds `()`
var y : Bool = True              -- `y` of type `Bool` holds `True`
var z : (Bool,()) = (False,())   -- `z` of given tuple type holds the given tuple
var n : List = Cons(Cons($List)) -- `n` of recursive type `List` holds result of constructor
```

## Assignment

An assignment changes the value of a variable, native identifier, tuple index,
or discriminator:

```
set x = ()
set _n = 1
set tup.1 = n
set x.Student! = ()
```

## Call, Input & Output

The `call`, `input` & `output` statements invoke [functions](#TODO):

```
call f()        -- calls f passing ()
input std       -- input from stdin
output std 10   -- output to stdout
```

## Sequence

Statements execute one after the other and can be separated by semicolons:

```
call f() ; call g()
call h()
```

## Conditional

A conditional tests a `Bool` value and executes one of its true or false
branches depending on the test:

```
if x {
    call f()    -- if x is True
} else {
    call g()    -- if x is False
}
```

## Loop

A `loop` executes a block of statements indefinitely until it reaches a `break`
statement:

```
loop {
    ...         -- repeats this command indefinitely
    if ... {    -- until this condition is met
        break
    }
}
```

## Function, Argument and Return

A function declaration binds a block of statements to a name which can be
[called](TODO) afterwards.
The declaration also determines the [function type](TODO) with the argument and
return types.
The argument can be accessed through the identifier `arg`.
A `return` exits a function with a value:

```
func f : () -> () {
    return arg
}
```

## Block

A block delimits, between curly braces `{` and `}`, the scope and visibility of
[variables](TODO):

```
{
    var x: () = ()
    ... x ...           -- `x` is visible here
}
... x ...               -- `x` is not visible here
```

## Native

A native statement executes a [native token](TODO) in the host language:

```
native _{
    printf("Hello World!");
}
```

The modifier `pre` makes the native block to be included before the main
program:

```
native pre _{
    #include <math.h>
}
```

# 5. SYNTAX

```
Stmt ::= `var´ VAR `:´ [`\´] Type       -- variable declaration     var x: () = ()
            `=´ (Expr | `?´)
      |  `type´ [`rec´] USER `{`        -- user type declaration    type rec List {
            { USER `:´ Type [`;´] }     --    subcases                 Cons: List
         `}´                                                        }
      |  `set´ Expr `=´ Expr            -- assignment               set x = 1
      |  (`call´ | `input´ |` output´)  -- call                     call f()
            (VAR|NATIVE) [Expr]         -- input & output           input std ; output std 10
      |  `if´ Expr `{´ Stmt `}´         -- conditional              if x { call f() } else { call g() }
      |  `loop´ `{´ Stmt `}´            -- loop                     loop { break }
      |  `break´                        -- break                    break
         [`else´ `{´ Stmt `}´]
      |  `func´ VAR `:´ Type `{´        -- function                 func f : ()->() { return () }
            Stmt
         `}´
      |  `return´ Expr                  -- function return          return ()
      |  { Stmt [`;´] }                 -- sequence                 call f() ; call g()
      |  `{´ Stmt `}´                   -- block                    { call f() ; call g() }
      |  `native´ [`pre´] `{´ ... `}´   -- native                   native { printf("hi"); }

Expr ::= `(´ `)´                        -- unit value               ()
      |  NATIVE                         -- native expression        _printf
      |  `$´ USER                       -- empty constructor        $List
      |  VAR                            -- variable identifier      i
      |  `\´ Expr                       -- upref                    \x
      |  Expr `\´                       -- dnref                    x\
      |  `(´ Expr {`,´ Expr} `)´        -- tuple                    (x,())
      |  USER [Expr]                    -- constructor              True ()
      |  [`call´ | `input´ | `output´]  -- call                     f(x)
            (VAR|NATIVE) [Expr]         -- input & output           input std ; output std 10
      |  Expr `.´ NUM                   -- tuple index              x.1
      |  Expr `.´ [`$´] USER `!´        -- discriminator            x.True!
      |  Expr `.´ [`$´] USER `?´        -- predicate                x.False?
      |  `(´ Expr `)´                   -- group                    (x)

Type ::= `(´ `)´                        -- unit                     ()
      |  NATIVE                         -- native type              _char
      |  USER                           -- user type                Bool
      |  `(´ Type {`,´ Type} `)´        -- tuple                    ((),())
      |  Type `->´ Type                 -- function                 () -> ()
```
