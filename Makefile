#LIBS  = -lkernel32 -luser32 -lgdi32 -lopengl32
CFLAGS = -Wall #-Wno-switch #-Wno-return-local-addr

SRC=$(wildcard src/*.c)

tests: $(SRC) src/main/tests.c
	gcc -g -o $@ $^ $(CFLAGS) $(LIBS)

main: $(SRC) src/main/main.c
	gcc -g -o ce $^ $(CFLAGS) $(LIBS)
