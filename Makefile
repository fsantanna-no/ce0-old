#LIBS  = -lkernel32 -luser32 -lgdi32 -lopengl32
CFLAGS = -Wall #-Wno-switch #-Wno-return-local-addr

SRC=$(wildcard src/*.c)

tests: $(SRC) src/main/tests.c
	gcc -g -o $@ $^ $(CFLAGS) $(LIBS)

book: $(SRC) src/main/book.c
	gcc -g -o $@ $^ $(CFLAGS) $(LIBS)

cei: $(SRC) src/main/cei.c
	gcc -g -o cei $^ $(CFLAGS) $(LIBS)

main: $(SRC) src/main/main.c
	gcc -g -o ce $^ $(CFLAGS) $(LIBS)
