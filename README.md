# ce0

# Lexical rules

## Comments

A comment starts with `--` and runs until the end of the line:

```
-- this is a single line comment
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
