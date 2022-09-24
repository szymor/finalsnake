.PHONY: all clean

TARGET=finalsnake
SRC=main.c game.c
INC=main.h game.h
PKGS = sdl SDL_gfx
CFLAGS = $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS)) -lm

all: $(TARGET)

$(TARGET): $(SRC) $(INC)
	gcc $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -rf $(TARGET)