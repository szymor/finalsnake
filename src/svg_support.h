#ifndef _H_SVG_SUPPORT
#define _H_SVG_SUPPORT
#include <SDL/SDL.h>

int IMG_isSVG(SDL_RWops* src);

// it can return the surface greater than width x height
SDL_Surface* SVG_LoadSizedSVG_RW(const char* src, int width, int height,
	int cr, int cg, int cb, int ca, float angle);
#endif
