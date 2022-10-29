#ifndef _H_GFX
#define _H_GFX

#include <SDL.h>

#define HUE_PRECISION		(256)
#define SUIT_COUNT			(4)

extern SDL_Surface *tiles;

void tiles_init(void);
void tiles_prepare(int suit, int hue);
void tiles_dispose(void);

#endif
