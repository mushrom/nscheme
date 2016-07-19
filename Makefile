SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
CFLAGS = -Wall -std=c11

nscheme: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

.PHONY: clean
clean:
	rm -f nscheme
	rm -f $(OBJ)
