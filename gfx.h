#ifndef _H_GFX
#define _H_GFX

#include <SDL.h>

extern SDL_Surface *tiles;

void tiles_init(void);
void tiles_recolor(int hue);
void tiles_dispose(void);

#endif
