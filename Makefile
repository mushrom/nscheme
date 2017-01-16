-include config.mk

SRC    = $(wildcard src/*.c)
OBJ    = $(SRC:.c=.o)
DEPS   = $(OBJ:.o=.d)
CFLAGS = -Wall -O2 -MD -I./include -g $(CONFIG_OPTS)

nscheme: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

-include $(DEPS)

.PHONY: clean
clean:
	rm -f nscheme
	rm -f $(OBJ)
	rm -f $(DEPS)
