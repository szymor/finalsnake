.PHONY: all clean

TARGET=finalsnake
SRC=main.c
PKGS = sdl SDL_gfx
CFLAGS = $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS)) -lm

all: $(TARGET)

$(TARGET): $(SRC)
	gcc $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -rf $(TARGET)