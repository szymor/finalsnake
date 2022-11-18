#ifndef _H_SVG_SUPPORT
#define _H_SVG_SUPPORT
#include <SDL/SDL.h>

int IMG_isSVG(SDL_RWops* src);
SDL_Surface* SVG_LoadSizedSVG_RW(const char* src, int width, int height,
	int cr, int cg, int cb, int ca);
#endif
