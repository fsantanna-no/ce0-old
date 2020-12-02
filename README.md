# ce0

# Lexical rules

## Comments

A comment starts with `--` and runs until the end of the line:

```
-- this is a single line comment
```

## Symbols

The following symbols are used:

```
    {   }       -- block delimeter
    (   )       -- unit type, unit value, group expression
    ;           -- sequence separator
    :           -- variable & type declaration
    ,           -- tuple separator
    .           -- tuple index
```

## Keywords

The following keywords are reserved and cannot be used as identifiers:

```
    type        -- declare new types
    call        -- call functions
    val         -- declare immutable variables
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

# Expressions

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

## Constructors

A constructor creates a value of a type from a given subtype and argument:

```
True.Bool ()
```

# Statements

## Variable declarations

```
val x : ()
val y : ((),()) = ((),())
```

## Type declarations

```
type Bool {
    False: ()
    True:  ()
}
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

# Syntax

```
Stmt ::= `val´ VAR `:´ Type `=´ Expr    -- variable declaration     val x: () = ()
      |  `type´ TYPE `{`                -- type declaration
            { TYPE `:´ Type [`;´] }     -- subtypes
         `}´
      |  `call´ Expr                    -- call                     call f()
      |  { Stmt [`;´] }                 -- sequence                 call f() ; call g()

Expr ::= `(´ `)´                        -- unit value               ()
      |  NATIVE                         -- native identifier        _printf
      |  VAR                            -- variable identifier      i
      |  `(´ Expr {`,´ Expr} `)´        -- tuple                    (x,())
      |  Expr INDEX                     -- tuple index              x.1
      |  Expr `(´ Expr `)´              -- call                     f(x)
      |  TYPE `.´ TYPE `(´ Expr `)´     -- constructor              Bool.True ()
      |  `(´ Expr `)´                   -- group                    (x)
```
