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
    :           -- type declaration
    ,           -- tuple separator
    .           -- tuple index
```

## Keywords

The following keywords are reserved and cannot be used as identifiers:

```
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

A call invokes an expression as a function with the argument starting with
parenthesis:

```
f()         -- f   receives unit     ()
(id)(x)     -- id  receives variable x
add(x,y)    -- add receives tuple    (x,y)
```

# Statements

## Declarations

```
val x : ()
val y : ((),()) = ((),())
```

## Calls

A call invokes a call expression:

```
call f()    -- calls f passing ()
```

# Syntax

```
Stmt ::= `val´ VAR `:´ Type         -- declaration          val x: ()
      |  `call´ Expr                -- call                 call f()

Expr ::= `(´ `)´                    -- unit value           ()
      |  NATIVE                     -- native identifier    _printf
      |  VAR                        -- variable identifier  i
      |  `(´ Expr {`,´ Expr} `)´    -- tuple                (x,())
      |  Expr INDEX                 -- tuple index          x.1
      |  Expr `(´ Expr `)´          -- call                 f(x)
      |  `(´ Expr `)´               -- group                (x)
```
