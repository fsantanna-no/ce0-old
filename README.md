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
    {   }       -- block delimeters
    (   )       -- unit type, unit value, group expressions
    :           -- declarations
    ,           -- tuple separator
```

## Keywords

The following keywords are reserved and cannot be used as identifiers:

```
    val
```

## Identifiers

Identifiers for variables and types must start with a letter and might contain
letters, numbers and underscores.
A variable starts with a lowercase letter and a type starts with an uppercase
letter:

```
i       myCounter   x10         -- variables
Int     A           Tree        -- types
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

## Tuples

A tuple holds a fixed number of values of different types:

```
((),())     -- a pair of units
(x,(),y)    -- a triple
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

## Calls

A call invokes a call expression:

```
call f()    -- calls f passing ()
```
