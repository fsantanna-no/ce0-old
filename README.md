# Ce

A simple language with algebraic data types, ownership semantics, and scoped
memory management.

- Install & Use
- [Manual](doc/manual.md)
- [Memory management](doc/memory.md)
- [Comparison with Rust](doc/rust.md)

# Install

```
$ git clone https://github.com/fsantanna/ce0
$ cd ce0/
$ make tests
$ ./tests       # assert that `OK` appears (ignore apparent error messages)
$ make main
$ sudo make install
```

# Use

```
$ vi /tmp/test.ce
output std 42
$ ce -o test /tmp/test.ce
$ ./test        # assert that `42` appears
```
