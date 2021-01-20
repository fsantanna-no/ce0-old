# Ce

A simple language with algebraic data types, ownership semantics, and scoped
memory management (i.e. no garbage collection).

- [Manual](manual.md)
- Memory management
- [Comparison with other languages](other.md)

<!--
# A. Pools and recursive types

## Goals

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
-->

# Dynamic memory management

Values of recursive types, such as lists and trees, require dynamic memory
allocation since their sizes are unbounded.
They also grow and shrink during runtime since they typically represent complex
data structures that evolve over time.
Finally, they are manipulated by different parts of the program, even outside
the scope in which they were originally instantiated.
These three characteristics, (a) dynamic allocation, (b) variable size and (c)
scope portability, need to be addressed somehow.

*Ce* approaches recursive types with algebraic data types with ownership
semantics and scoped memory management.
Allocation is bound to the scope of the assignee, which is the owner of the
value.
A value has exactly one owner at any given time, which implies that recursive
values can only form trees of ownerships (but not graphs with cycles or even
generic DAGs).
Deallocation occurs automatically when the scope of the owner terminates.
Ownership can be transferred by reassigning the value to another assignee,
which can live in another scope.
A value can also be shared with a pointer without transferring ownership, in
which case generic graphs are possible.

*Ce* ensures that deallocation occurs exactly once at the very moment when
there are no more active pointers to the value.
In particular the following cases must be prevented:

- Memory leak: when a value cannot be referenced but is not deallocated and remains in memory.
- Dangling reference: when a value is deallocated but can still be referenced (aka. *use-after-free*).
- Double free: when a value is deallocated multiple times.

## Basics

In *Ce*, a new type declaration supports variants (subcases) with tuples:

```
type Character {
    Warrior: (Int,Int)      -- variant Warrior has strength and stamina
    Ranger:  (Int,Int)      -- variant Ranger has sight and speed
    Wizard:  Int            -- variant Wizard has mana power
}
```

Such composite types are also known as algebraic data types because they are
composed of sums (variants) and products (tuples).

A new type declaration can also be made recursive if it uses itself in one of
its subcases:

```
type rec List {         -- a list is either empty ($List) or
    Item: (Int,List)    -- an item that holds a number and a sublist
}

var l: List = Item (1, Item (2,$List))   -- list `1 -> 2 -> empty`
```

A variable of a recursive type holds a *strong reference* to its value, since
constructors are always dynamically allocated in the heap:

```
var x: List = Item(1, $List)
    |              __|__
   / \            /     \
  | x |--------> |   1   | <-- actual allocated memory with the linked list
   \_/   ref     |  null |
    |             \_____/
    |                |
  stack             heap
```

The assigned variable is the owner of the allocated value, which is
automatically deallocated when the enclosing scope terminates:

```
{
    var x: List = Item(1, $List)
}
-- scope terminates, memory pointed by `x` is deallocated
```

A variable can be pointed or *borrowed* with the prefix backslash `\`.
In this case, both the owner and the pointer refer to the same allocated value:

```
var x: List  = Item(1, $List)
var y: \List = \x    |
    |              __|__
   / \   ref      /     \
  | x |--------> |   1   | <-- actual allocated memory with the linked list
   \_/       /   |  null |
    |       /     \_____/
    |      /         |
  stack   /         heap
    |    / ptr
   / \  /
  | y |-
   \_/
```

We distinguish a strong reference from a pointer in the sense that the former
owns the value and does not use [pointer operations](TODO) to manipulate it.

## Ownership and Borrowing

The ownership and borrowing of dynamically allocated values must follow a set
of rules:

1. Every allocated constructor has a single owner at any given time. Not zero, not two or more.
    - The owner is a variable that lives in the stack and reaches the allocated value.
2. When the owner goes out of scope, the allocated memory is automatically
   deallocated.
3. Ownership can be transferred in three ways:
    - Assigning the owner to another variable, which becomes the new owner (e.g. `new = old`).
    - Passing the owner to a function call argument, which becomes the new owner (e.g. `f(old)`).
    - Returning the owner from a function call to an assignee, which becomes the new owner (e.g. `new = f()`).
4. When transferring ownership, the original owner is assigned its empty subcase.
5. Ownership cannot be transferred to the current owner subtree.
6. Ownership cannot be transferred with an active pointer in scope.
7. A pointer cannot escape or survive outside the scope of its owner.

All rules are verified at compile time, i.e., there are no runtime checks or
extra overheads.

### Ownership transfer

As stated in rule 4, an ownership transfer invalidates the original owner and
rejects further accesses to it:

```
{
    var x: List = Item(1, $List)    -- `x` is the original owner
    var y: List = x                 -- `y` is the new owner
    ... x ...                       -- error: `x` cannot be referred again
    ... y ...                       -- ok
}
```

Ownership transfer ensures that rule 1 is preserved.
If ownership could be shared, deallocation in rule 2 would be ambiguous or
cause a double free, since owners could be in different scopes:

```
{
    var x: List = Item(1, $List)    -- `x` is the original owner
    {
        var y: List = x             -- `y` is the new owner
    }                               -- deallocate here?
}                                   -- or here or both?
```

Ownership transfer is particularly important when the value must survive the
allocation scope, which is typical of constructor functions:

```
func build: () -> List {
    var tmp: List = ...     -- `tmp` is the original owner
    return tmp              -- `return` transfers ownership (we don't want to deallocate it now)
}
var l: List = build()       -- `l` is the new owner
```

As stated in rule 5, the owner of a value cannot be transferred to its own
subtree.
This also takes into account pointers to the subtree.
This rule prevents cycles, which also ensures that rule 1 is preserved:

```
var l: List = Item $List
var p: \List = \l.Item!     -- `p` points to the end of `l`
set p\ = l                  -- error: cannot transfer `l` to the end of itself
```

It is possible to transfer only part of a recursive value.
In this case, the removed part will be automatically reset to the empty subcase:

```
var x: List = Item(1, Item(2, $List))   -- after: Item(1,$List)
var y: List = x.Item!                   -- after: Item(2,$List)
```

Finally, it is also possible to make a "void transfer" through a `set`
statement.
In this case, the value with lost ownership will be deallocated immediately:

```
var x: List = Item(1, $List)    -- previous value
set x = $List                   -- previous value is deallocated
```

## Borrowing

In many situations, transferring ownership is undesirable, such as when passing
a value to a narrower scope for temporary manipulation:

```
var l: List = ...       -- `l` is the owner
... length(\l) ...      -- `l` is borrowed on call and unborrowed on return
... l ...               -- `l` is still the owner

func length: (\List -> Int) {
    ... -- use pointer, which is destroyed on termination
}
```

Rule 7 states that a pointer cannot escape or survive outside the scope of its
owner:

```
func f: () -> \List {
    var l: List = ...       -- `l` is the owner
    return \l               -- error: cannot return pointer to deallocated value
}
```

```
var x: \List = ...          -- outer scope
{
    var l2: List = ...
    set x = \l2             -- error: cannot hold pointer from inner scope
}
... x ...                   -- use-after-free
```

If surviving pointers were allowed, they would refer to deallocated values,
resulting in a dangling reference (i.e, *use-after-free*).

Rule 6 states that if there is an active pointer to a value, then its ownership
cannot be transferred:

```
var l: List = ...       -- owner
var x: \List = \l       -- active pointer
call g(l)               -- error: cannot transfer with active pointer
... x ...               -- use-after-free
```

This rule prevents that a transfer eventually deallocates a value that is still
reachable through an active pointer (i.e, *use-after-free*).
This rule implies that a pointer *dnref* can never be transferred because the
pointer must be pointing to some value, and hence is active:

```
var l: List = ...       -- owner
var x: \List = \l       -- active pointer
var y: List = x\        -- error: cannot transfer with active pointer
```

<!--
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

!--
If the pool is unbouded (e.g. `y[]`), all allocation is made in the heap with
`malloc`.
Then, when the root reference (e.g. `y`) goes out of scope, it is traversed to
`free` all memory.
--

## Details

### Pool allocation

A bounded pool is defined internally as follows:

```
typedef struct {
    void* buf;      // stack-allocated buffer
    int   max;      // maximum size
    int   cur;      // current size
} Pool;
```

Pool allocation depends if the pool is bounded or unbounded:

```
void* pool_alloc (Pool* pool, int n) {
    if (pool == NULL) {                     // pool is unbounded
        return malloc(n);                   // fall back to `malloc`
    } else {
        void* ret = &pool->buf[pool->cur];
        pool->cur += n;                     // nodes are allocated sequentially
        if (pool->cur <= cur->max) {
            return ret;
        } else {
            return NULL;
        }
    }
}
```

--

A dynamic constructor must check if all allocations succeeded.

Illustrative example:

```
func f: () -> Nat {
    var x: Nat = Succ(Succ($Nat))
    return x
}
var y[]: Nat = f()    -- y[] or y[N]
```

Generated code:

```
void f (Pool* pool) {
    int _cur = pool->cur;                       // current pool size
    Nat* _2 = pool_alloc(pool, sizeof(Nat));
    Nat* _1 = pool_alloc(pool, sizeof(Nat));
    if (_2==NULL || _1==NULL) {                 // one of them failed
        if (pool == NULL) {
            free(_1);                           // free both
            free(_2);
        } else {
            pool->cur = _cur;                   // restore pool size
        }
    } else {
        *_2 = (Nat) { Succ, {._Succ=NULL} };    // assign both
        *_1 = (Nat) { Succ, {._Succ=_2} };
        x = _1;                                 // root value
    }
}
```

### Pool deallocation

- stack
- __cleanup__

### Tracking assignments

1. Check the root assignment for dependencies in nested scopes:

```
var y: Nat = Succ(Succ($Nat))   -- same scope: static allocation
```

```
var y[]: Nat = f()              -- body of `f` is nested: pool allocation
```

2. Check `return` of body for dependencies:

```
return x                        -- check `x`
```

```
var x: Nat = Succ(Succ($Nat))   -- constructor must be allocated in the received pool
```

### TODO

```
-- OK
call show(Succ($Nat))     -- ok stack

-- ERR
-- `f` returns `Nat` but have no pool to allocated it
-- if call returns isrec, it must be in an assignment or in a return (to use pool from outside)
func f: () -> Nat {}
call f()                    -- missing pool for return of "f"

-- OK
var three: Nat = ...
func fthree: () -> Nat {
    return three            -- should not use pool b/c defined outside
}
```

-->
