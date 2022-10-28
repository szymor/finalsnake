#include "gfx.h"
#include <SDL_image.h>
#include <math.h>

#define MAX(a,b)		((a) > (b) ? (a) : (b))
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#define MAX3(a,b,c)		MAX((a), MAX((b), (c)))
#define MIN3(a,b,c)		MIN((a), MIN((b), (c)))

SDL_Surface *tiles = NULL;

static SDL_Surface *tiles_orig = NULL;

void tiles_init(void)
{
	SDL_Surface *tmp = IMG_Load("tiles.png");
	tiles_orig = SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);

	tiles = SDL_CreateRGBSurface(0, tiles_orig->w, tiles_orig->h,
		tiles_orig->format->BitsPerPixel,
		tiles_orig->format->Rmask,
		tiles_orig->format->Gmask,
		tiles_orig->format->Bmask,
		tiles_orig->format->Amask);
	SDL_BlitSurface(tiles_orig, NULL, tiles, NULL);
}

void tiles_recolor(int hue)
{
	const SDL_PixelFormat *fmt = tiles_orig->format;
	const Uint8 bpp = fmt->BytesPerPixel;
	Uint32 temp;

	SDL_LockSurface(tiles_orig);
	SDL_LockSurface(tiles);

	for (int y = 0; y < tiles_orig->h; ++y)
		for (int x = 0; x < tiles_orig->w; ++x)
		{
			Uint32 pixel = *(Uint16 *)((Uint8 *)tiles_orig->pixels + y * tiles_orig->pitch + x * bpp);

			Uint8 red, green, blue;

			temp = pixel & fmt->Rmask;
			temp = temp >> fmt->Rshift;
			temp = temp << fmt->Rloss;
			red = (Uint8)temp;
			temp = pixel & fmt->Gmask;
			temp = temp >> fmt->Gshift;
			temp = temp << fmt->Gloss;
			green = (Uint8)temp;
			temp = pixel & fmt->Bmask;
			temp = temp >> fmt->Bshift;
			temp = temp << fmt->Bloss;
			blue = (Uint8)temp;

			// RGB to HSV
			double r = red / 255.0;
			double g = green / 255.0;
			double b = blue / 255.0;
			double vmax = MAX3(r, g, b);	// range 0..1
			double vmin = MIN3(r, g, b);
			double chroma = vmax - vmin;
			double s = vmax > 0 ? chroma / vmax : 0;	// range 0..1
			double h = 0;	// range 0..2pi
			if (chroma > 0)
			{
				if (vmax == r)
				{
					h = (g - b) / chroma;
				}
				else if (vmax == g)
				{
					h = (b - r) / chroma + 2 * M_PI / 3;
				}
				else if (vmax == b)
				{
					h = (r - g) / chroma + 4 * M_PI / 3;
				}
			}

			// change contrast and recolor
			vmax = 0.5 + vmax * 0.25;
			h = 2 * M_PI * hue / 100;
			s = 0.5;

			// HSV to RGB
			chroma = vmax * s;
			h = 3 * h / M_PI;
			double hmod = h;
			while (hmod >= 2.0) hmod -= 2.0;
			double xcol = chroma * (1.0 - fabs(hmod - 1.0));
			if (h < 1)
			{
				r = chroma;
				g = xcol;
				b = 0;
			}
			else if (h < 2)
			{
				r = xcol;
				g = chroma;
				b = 0;
			}
			else if (h < 3)
			{
				r = 0;
				g = chroma;
				b = xcol;
			}
			else if (h < 4)
			{
				r = 0;
				g = xcol;
				b = chroma;
			}
			else if (h < 5)
			{
				r = xcol;
				g = 0;
				b = chroma;
			}
			else // if (h < 6)
			{
				r = chroma;
				g = 0;
				b = xcol;
			}
			const double m = vmax - chroma;
			r += m;
			g += m;
			b += m;

			// back to Uint8
			red = r * 255;
			green = g * 255;
			blue = b * 255;

			Uint16 *dst_px = (Uint16 *)((Uint8 *)tiles->pixels + y * tiles->pitch + x * bpp);
			*dst_px = (red >> fmt->Rloss) << fmt->Rshift |
				(green >> fmt->Gloss) << fmt->Gshift | 
				(blue >> fmt->Bloss) << fmt->Bshift;
		}

	SDL_UnlockSurface(tiles);
	SDL_UnlockSurface(tiles_orig);
}

void tiles_dispose(void)
{
	SDL_FreeSurface(tiles);
	tiles = NULL;
	SDL_FreeSurface(tiles_orig);
	tiles_orig = NULL;
}
