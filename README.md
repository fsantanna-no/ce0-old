# Ce

A simple language with algebraic data types, ownership semantics, and scoped
memory management (i.e. no garbage collection).

- Install
- [Manual](manual.md)
- [Memory management](memory.md)
- [Comparison with Rust](rust.md)

# INSTALL

```
$ git clone https://github.com/fsantanna/ce0
$ cd ce0/
$ make tests
$ ./tests       # assert that `OK` appears (ignore apparent error messages)
$ make main
$ sudo make install
```

# USE

```
$ vi /tmp/test.ce
output std 42
$ ce -o test /tmp/test.ce
$ ./test        # assert that `42` appears
```
