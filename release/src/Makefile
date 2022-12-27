CC = gcc
CFLAGS = -I. -g
DEPS = PL0.h set.h
OBJ = pl0.o set.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

pl0: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

all: pl0

.PHONY: clean

clean:
	rm *.o