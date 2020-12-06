# Ce

A simple (functional?) language with algebraic data types and automatic memory
management:

- bounded memory allocation
- deterministic deallocation
- no garbage collection

# 1. Lexical rules

## Comment

A comment starts with `--` and runs until the end of the line:

```
-- this is a single line comment
```

## Symbols and Keywords

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
    !           -- type discriminator
    ?           -- type predicate
```

The following keywords are reserved and cannot be used as identifiers:

```
    arg         -- function argument
    call        -- function call
    else        -- conditional statement
    func        -- function declaration
    if          -- conditional statement
    Nil         -- null subtype
    output      -- output function
    rec         -- type, function recursive declaration
    return      -- function return
    type        -- new type declaration
    val         -- immutable variable declaration
```

## Identifiers

A variable identifier starts with a  lowercase letter and might contain
letters, digits, and underscores:

```
i       myCounter   x_10        -- variable identifiers
```

A type identifier starts with an uppercase letter and might contain letters,
digits, and underscores:

```
Int     A10         Tree        -- type identifiers
```

A native identifier starts with an underscore and might contain letters,
digits, and underscores:

```
_char    _printf    _errno      -- native identifiers
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

## Variable

A variable holds a value of its type:

```
i    myCounter    x_10
```

## Native symbol

A native symbol holds a value from the host language:

```
_printf    _errno
```

## Tuple and Index

A tuple holds a fixed number of values of different types:

```
((),False)              -- a pair with () and False
(x,(),y)                -- a triple
```

A tuple index holds the value at the given position:

```
(x,()).2                -- yields ()
```

## Call

A call invokes an expression as a function with the given argument:

```
f ()                    -- f   receives unit     ()
(id) (x)                -- id  receives variable x
add (x,y)               -- add receives tuple    (x,y)
```

The special function `output` shows the given argument in the screen:

```
x = ((),())
output(x)               -- shows "((),())" in the screen
```

## Constructor, Discriminator and Predicate

A constructor creates a value of a type given one subtype and its argument:

```
True ()                 -- value of type Bool
False                   -- () is optional
Car (True,())           -- subtype Car holds a tuple
```

A discriminator accesses the value of a type as one of its subtypes.
It sufixes the value with a dot `.`, a subtype identifier, and an exclamation
mark `!`:

```
(True ()).True!         -- yields ()

x = Node (Nil,(),Nil)
x.Node!.2               -- yields ()
```

The value `Nil` corresponds to the null subtype of all recursive types.

A predicate checks if the value of a type is of its given subtype.
It sufixes the value with a dot `.`, a subtype identifier, and a question mark
`?`:

```
type Member {
    Student:   ()
    Professor: ()
}
x = Professor
b = x.Professor?        -- yields True
```

# 3. Statements

## Type declaration

A type declaration creates a new type.
Each case in the type declaration defines a subtype of it:

```
type Bool {
    False: ()       -- subtype False holds unit value
    True:  ()       -- subtype True  holds unit value
}
```

A recursive type uses a `rec` declaration and always contains the implicit base
null subtype `Nil´:

```
type rec Tree {
    -- Nil: ()              -- implicit null subtype always present
    Node: (Tree,(),Tree)    -- subtype Node holds left subtree, unit value, and right subtree
}
```

## Variable declaration

A variable declaration binds a value to a name with a specified type:

```
val x : () = ()                 -- assigns `()` to variable `x` of type `()`
val y : Bool = True             -- assigns `True` to variable `y` of type `Bool`
val z : (Bool,()) = (False,())  -- assigns a tuple to variable `z`
val t : Tree = Node(Nil,(),Node(Nil,(),Nil))
```

If the type is recursive and the assignment requires dynamic allocation, the
declaration must also include a [pool](TODO) with brackets:

```
val x[]: Nat = f()      -- the constructors in `f` are allocated in a pool
val y[64]: Nat = f()    -- the pool can be bounded to limit the number of nodes
```

## Call

A call statement invokes a call expression:

```
call f()    -- calls f passing ()
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

## Function, Argument and Return

A function declaration binds a block of statements to a name which can be
[called](TODO) afterwards.
The declaration also determines the types of the argument and return values
separated by an arrow `->`.
The argument can be acessed through the identifier `arg`.
A `return` exits a function with a value:

```
func f : () -> () {
    return arg
}
```

# 5. Pools and recursive types

Recursive types, such as lists and trees, require dynamic memory allocation
since their sizes are unbounded.

Pools are containers for mutually referred values of recursive types.
A pool is associated with the root variable of a recursive type.
When the root goes out of scope, all pool memory is automatically reclaimed.
A pool can be optionally declared with a bounded size, allowing it to be
allocated in the stack.
Pools enable to the following properties for recursive types:

- bounded memory allocation
- deterministic deallocation
- no garbage collection

A recursive type declaration uses itself in one of its subtypes:

```
type rec Nat {  -- a natural number is either zero (`Nil`) or
    Succ: Nat   -- a successor of a natural number (`Succ(Nat)`)
}

val two: Nat = Succ(Succ(Nil))  -- `two` is the successor of the successor of zero (represented as `Nil`)
```

Values of recursive types are always references:

```
val x: Nat = Succ(Succ(Nil))
val y: Nat = x  -- x,y share the same reference, no copy is made
```

If the constructors reside in the scope of the assignee, no dynamic memory
allocation is required.
Internally, the constructors can be references allocated in the stack:

```
-- x = y = Succ(Succ(Nil))
Nat _2 = (Nat) { Succ, {._Succ=NULL} };   -- declares last value first (Succ(Nil)
Nat _1 = (Nat) { Succ, {._Succ=&_2} };    -- points to previous value  (Succ(...))
Nat* x = &_1;                             -- points to last reference
Nat* y = x;                               -- copies reference
```

If the constructors do not survive the scope of the assignee, they are
allocated in a memory pool declared with brackets in the assignee:

```
func f: () -> Nat {
    val x: Nat = Succ(Succ(Nil))    -- constructor inside a function
    return x                        -- `x` does not survive the scope
}
val y[]: Nat = f()                  -- `y` is a memory pool for a Nat tree to be allocated inside `f`
```

All dependencies of an assignment are tracked and all constructors are
allocated in the same pool.
When the pool itself goes out of scope, all allocated memory inside it is
traversed and is automatically reclaimed.

The pool may be declared with bounded size (e.g. `y[64]`), which limits the
number of nodes in the tree.
This allows the pool items to be allocated in the stack (instead of `malloc`).
When the pool goes out of scope, the stack unwinds and all memory is
automatically reclaimed.

Internally, the pool is forwarded to all constructors locations where the
actual allocations takes place:

```
void f (Pool* pool) {
    Nat* _2 = pool_alloc(pool, sizeof(Nat));    // constructors allocate
    Nat* _1 = pool_alloc(pool, sizeof(Nat));    // in the forwarded pool
    *_2 = (Nat) { Succ, {._Succ=NULL} };
    *_1 = (Nat) { Succ, {._Succ=_2} };
    x = _1;
    return x;
}

Nat  _yv[64];               // stack-allocated buffer (if bounded)
Pool _yp = { _yv, 64, 0 };  // buffer, max size, cur size
Nat* y = f(&_yp);           // pool is NULL if unbounded
```

<!--
If the pool is unbouded (e.g. `y[]`), all allocation is made in the heap with
`malloc`.
Then, when the root reference (e.g. `y`) goes out of scope, it is traversed to
`free` all memory.
-->

# 4. Syntax

```
Stmt ::= `val´ VAR `:´ Type `=´ Expr    -- variable declaration     val x: () = ()
      |  `type´ [`rec´] USER `{`        -- user type declaration
            { USER `:´ Type [`;´] }     -- subtypes
         `}´
      |  `call´ Expr                    -- call                     call f()
      |  `if´ Expr `{´ Stmt `}´         -- conditional              if x { call f() } else { call g() }
         [`else´ `{´ Stmt `}´]
      |  `func´ VAR `:´ Type `{´        -- function                 func f : ()->() { return () }
            Stmt
         `}´
      |  `return´ Expr                  -- function return          return ()
      |  { Stmt [`;´] }                 -- sequence                 call f() ; call g()

Expr ::= `(´ `)´                        -- unit value               ()
      |  NATIVE                         -- native identifier        _printf
      |  VAR                            -- variable identifier      i
      |  `arg´                          -- function argument        arg
      |  `Nil´                          -- null recursive subtype   Nil
      |  `(´ Expr {`,´ Expr} `)´        -- tuple                    (x,())
      |  Expr `.´ INDEX                 -- tuple index              x.1
      |  Expr `(´ Expr `)´              -- call                     f(x)
      |  USER [`(´ Expr `)´]            -- constructor              True ()
      |  Expr `.´ USER `!´              -- discriminator            x.True!
      |  Expr `.´ USER `?´              -- predicate                x.False?
      |  `(´ Expr `)´                   -- group                    (x)

Type ::= `(´ `)´                        -- unit                     ()
      |  NATIVE                         -- native                   _char
      |  USER                           -- user type                Bool
      |  `(´ Type {`,´ Type} `)´        -- tuple                    ((),())
      |  Type `->´ Type                 -- function                 () -> ()
```
