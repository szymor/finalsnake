.PHONY: all clean

TARGET=finalsnake
SRC=$(addprefix src/,main.c game.c gfx.c svg_support.c)
INC=$(addprefix src/,main.h game.h gfx.h svg_support.h nanosvg.h nanosvgrast.h)
PKGS = sdl SDL_gfx SDL_image SDL_mixer

COMMIT_HASH != git rev-parse --short=7 HEAD
$(shell git diff-index --quiet HEAD)
ifneq ($(.SHELLSTATUS),0)
COMMIT_HASH := $(COMMIT_HASH)-dirty
endif

CFLAGS = -Werror --pedantic $(shell pkg-config --cflags $(PKGS)) -DCOMMIT_HASH=$(COMMIT_HASH)
LDFLAGS = $(shell pkg-config --libs $(PKGS)) -lm

all: $(TARGET)

$(TARGET): $(SRC) $(INC)
	gcc $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -rf $(TARGET)