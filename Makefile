BASE=triangle-lock
SRC=$(BASE).c
CFLAGS=-Ofast
LFLAGS=-lm -lX11 -L/usr/lib
BIN=$(BASE)

all: screensaver

screensaver:
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LFLAGS)

clean:
	rm -f $(BIN)

install:
	sudo cp -i $(BIN) /usr/bin/

.PHONY: screensaver
