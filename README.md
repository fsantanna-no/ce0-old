# ce0

# 1. Lexical rules

## Comments

A comment starts with `--` and runs until the end of the line:

```
-- this is a single line comment
```

## Symbols

The following symbols are valid:

```
    {   }       -- block delimeter
    (   )       -- unit type, unit value, group expression
    ;           -- sequence separator
    :           -- variable & type declaration
    =           -- variable assignment
    ,           -- tuple separator
    .           -- tuple index, type constructor & destructor
    !           -- type destructor
    ?           -- type predicate
```

## Keywords

The following keywords are reserved and cannot be used as identifiers:

```
    call        -- function call
    else        -- conditional statement
    if          -- conditional statement
    type        -- new type declaration
    val         -- immutable variable declaration
```

## Identifiers

A variable identifier starts with a  lowercase letter and might contain
letters, digits, and underscores:

```
i       myCounter   x10         -- variable identifiers
```

A type identifier starts with an uppercase letter and might contain letters,
digits, and underscores:

```
Int     A10         Tree        -- type identifiers
```

A native identifier starts with an underscore and might contain letters,
digits, and underscores:

```
_char    _printf    _i99        -- native identifiers
```

A tuple index is a numeric value:

```
1       2       3               -- tuple indexes
```

# 2. Expressions

## Unit

The unit value is the unique value of the [unit type](TODO):

```
()
```

## Variables

A variable holds a value of its type:

```
i    myCounter    x10
```

## Native symbols

A native symbol holds a value from the host language:

```
_printf    _i99
```

## Tuples

A tuple holds a fixed number of values of different types:

```
((),())     -- a pair of units
(x,(),y)    -- a triple
```

A tuple index holds the value at the given position:

```
(x,()).2    -- returns ()
```

## Calls

A call invokes an expression as a function with the argument:

```
f()         -- f   receives unit     ()
(id)(x)     -- id  receives variable x
add(x,y)    -- add receives tuple    (x,y)
```

## Constructors, Destructors, Predicates

A constructor creates a value of a type from a given subtype and argument:

```
Bool.True ()
```

A destructor acesses the value of a type as one of its subtypes:

```
(Bool.True ()).True!        -- yields ()

x = Tree.Node ($,10,$)
x.Tree.Node!.2              -- yields 10
```

A subtype predicate evaluates to a `Bool`:

```
type Member {
    Student:   ()
    Professor: ()
}
x = Member.Professor ()
b = x.Professor?            -- yields Bool.True()
```

# 3. Statements

## Type declarations

A type declaration creates a new type.
Each case in the type declaration defines a subtype of it:

```
type Bool {
    False: ()       -- subtype False holds unit value
    True:  ()       -- subtype True  holds unit value
}

type Tree {
    Node: (Tree,(),Tree)    -- subtype Node holds left subtree, unit value, and right subtree
}
```

Every recursive type includes an implicit void subtype `$´.

## Variable declarations

```
val x : () = ()
val x : Bool = Bool.True ()
val y : (Bool,()) = (Bool.False (),())
```

## Calls

A call invokes a call expression:

```
call f()    -- calls f passing ()
```

## Sequences

Statements execute one after the other and can be separated by semicolons:

```
call f() ; call g()
call h()
```

## Conditionals

A conditional tests a `Bool` value and executes one of its true or false
branches depending on the test:

```
if x {
    call f()    -- if x is Bool.True
} else {
    call g()    -- if x is Bool.False
}
```

# 4. Syntax

```
Stmt ::= `val´ VAR `:´ Type `=´ Expr    -- variable declaration     val x: () = ()
      |  `type´ TYPE `{`                -- type declaration
            { TYPE `:´ Type [`;´] }     -- subtypes
         `}´
      |  `call´ Expr                    -- call                     call f()
      |  { Stmt [`;´] }                 -- sequence                 call f() ; call g()
      |  `if´ Expr `{´ Stmt `}´         -- conditional              if x { call f() } else { call g() }
         [`else´ `{´ Stmt `}´]

Expr ::= `(´ `)´                        -- unit value               ()
      |  NATIVE                         -- native identifier        _printf
      |  VAR                            -- variable identifier      i
      |  `(´ Expr {`,´ Expr} `)´        -- tuple                    (x,())
      |  Expr `.´ INDEX                 -- tuple index              x.1
      |  Expr `(´ Expr `)´              -- call                     f(x)
      |  TYPE `.´ TYPE `(´ Expr `)´     -- constructor              Bool.True ()
      |  Expr `.´ TYPE `!´              -- destructor               x.True!
      |  Expr `.´ TYPE `?´              -- predicate                x.False?
      |  `(´ Expr `)´                   -- group                    (x)
```
