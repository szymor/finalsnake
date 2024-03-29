#include "gfx.h"
#include "main.h"
#include "svg_support.h"
#include <SDL_image.h>
#include <SDL_gfxPrimitives.h>
#include <math.h>
#include <stdbool.h>

#define MAX(a,b)		((a) > (b) ? (a) : (b))
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#define MAX3(a,b,c)		MAX((a), MAX((b), (c)))
#define MIN3(a,b,c)		MIN((a), MIN((b), (c)))

SDL_Surface *tiles = NULL;
SDL_Surface *fruits = NULL;
SDL_Surface *veggies = NULL;
SDL_Surface *snake_head[SKILL_END] = { NULL };
SDL_Surface *snake_body[SKILL_END] = { NULL };

// each obstacle size can have different number of frames
int obstacle_framelimits[OBS_SHEETS_COUNT];

static SDL_Surface *tiles_orig = NULL;
static SDL_Surface *obstacle_surfaces[OBS_SHEETS_COUNT];
static const int obstacle_gears_num[OBS_STYLES_COUNT] = {
	4, 40, 12, 9, 24, 8
};

static void rgb_to_hsv(double *rh, double *gs, double *bv);
static void hsv_to_rgb(double *hr, double *sg, double *vb);
static void surface_recolor(SDL_Surface *s, int hue);

// r,g,b - range 0..1
static void rgb_to_hsv(double *rh, double *gs, double *bv)
{
	double r = *rh;
	double g = *gs;
	double b = *bv;
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

	*rh = h;
	*gs = s;
	*bv = vmax;
}

static void hsv_to_rgb(double *hr, double *sg, double *vb)
{
	double vmax = *vb;
	double s = *sg;
	double h = *hr;
	double r, g, b;

	double chroma = vmax * s;
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

	*hr = r;
	*sg = g;
	*vb = b;
}

Uint32 get_wall_color(int hue)
{
	Uint8 r, g, b, a;
	double rh = 2 * M_PI * hue / HUE_PRECISION;
	double gs = 0.7;
	double bv = 0.2;
	hsv_to_rgb(&rh, &gs, &bv);
	r = rh * 255.0;
	g = gs * 255.0;
	b = bv * 255.0;
	a = 255;
	return (r << 24) | (g << 16) | (b << 8) | a;
}

// use for 32-bit surfaces only (DisplayFormatAlpha surfaces also count)
static void surface_recolor(SDL_Surface *s, int hue)
{
	Uint32 temp;
	const SDL_PixelFormat *fmt = s->format;
	double rh = 2 * M_PI * hue / HUE_PRECISION;
	double gs = 0.1;
	double bv = 1.0;
	hsv_to_rgb(&rh, &gs, &bv);

	SDL_LockSurface(s);
	for (int y = 0; y < s->h; ++y)
		for (int x = 0; x < s->w; ++x)
		{
			Uint32 *pixel = (Uint32 *)((Uint8 *)s->pixels + y * s->pitch + x * s->format->BytesPerPixel);

			Uint8 red, green, blue, alpha;
			temp = *pixel & fmt->Rmask;
			temp = temp >> fmt->Rshift;
			temp = temp << fmt->Rloss;
			red = (Uint8)temp;
			temp = *pixel & fmt->Gmask;
			temp = temp >> fmt->Gshift;
			temp = temp << fmt->Gloss;
			green = (Uint8)temp;
			temp = *pixel & fmt->Bmask;
			temp = temp >> fmt->Bshift;
			temp = temp << fmt->Bloss;
			blue = (Uint8)temp;
			temp = *pixel & fmt->Amask;
			temp = temp >> fmt->Ashift;
			temp = temp << fmt->Aloss;
			alpha = (Uint8)temp;

			double r = rh * (red / 255.0);
			double g = gs * (green / 255.0);
			double b = bv * (blue / 255.0);

			red = r * 255;
			green = g * 255;
			blue = b * 255;

			*pixel = ((red >> fmt->Rloss) << fmt->Rshift) |
				((green >> fmt->Gloss) << fmt->Gshift) |
				((blue >> fmt->Bloss) << fmt->Bshift) |
				((alpha >> fmt->Aloss) << fmt->Ashift);
		}
	SDL_UnlockSurface(s);
}

void tiles_init(void)
{
	SDL_Surface *tmp = IMG_Load(GFX_DIR "tiles" xstr(CHECKERBOARD_SIZE) ".png");
	tiles_orig = SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);

	tiles = SDL_CreateRGBSurface(0, CHECKERBOARD_SIZE * 2, CHECKERBOARD_SIZE,
		tiles_orig->format->BitsPerPixel,
		tiles_orig->format->Rmask,
		tiles_orig->format->Gmask,
		tiles_orig->format->Bmask,
		tiles_orig->format->Amask);
	SDL_BlitSurface(tiles_orig, NULL, tiles, NULL);
}

void tiles_prepare(int suit, int hue)
{
	const SDL_PixelFormat *fmt = tiles_orig->format;
	const Uint8 bpp = fmt->BytesPerPixel;
	Uint32 temp;

	SDL_Rect src = {.x = CHECKERBOARD_SIZE * suit, .y = 0,
		.w = CHECKERBOARD_SIZE, .h = CHECKERBOARD_SIZE};
	SDL_Rect dst = {.x = 0, .y = 0, .w = 0, .h = 0};
	SDL_BlitSurface(tiles_orig, &src, tiles, &dst);
	dst.x = CHECKERBOARD_SIZE;
	SDL_BlitSurface(tiles_orig, &src, tiles, &dst);

	// invert the second tile
	SDL_LockSurface(tiles);
	for (int y = 0; y < tiles->h; ++y)
		for (int x = CHECKERBOARD_SIZE; x < tiles->w; ++x)
		{
			Uint16 *pixel = (Uint16 *)((Uint8 *)tiles->pixels + y * tiles->pitch + x * bpp);

			Uint8 red, green, blue;
			temp = *pixel & fmt->Rmask;
			temp = temp >> fmt->Rshift;
			temp = temp << fmt->Rloss;
			red = (Uint8)temp;
			temp = *pixel & fmt->Gmask;
			temp = temp >> fmt->Gshift;
			temp = temp << fmt->Gloss;
			green = (Uint8)temp;
			temp = *pixel & fmt->Bmask;
			temp = temp >> fmt->Bshift;
			temp = temp << fmt->Bloss;
			blue = (Uint8)temp;

			red = 255 - red;
			green = 255 - green;
			blue = 255 - blue;

			*pixel = (red >> fmt->Rloss) << fmt->Rshift |
				(green >> fmt->Gloss) << fmt->Gshift |
				(blue >> fmt->Bloss) << fmt->Bshift;
		}
	SDL_UnlockSurface(tiles);

	SDL_LockSurface(tiles);
	for (int y = 0; y < tiles->h; ++y)
		for (int x = 0; x < tiles->w; ++x)
		{
			Uint16 pixel = *(Uint16 *)((Uint8 *)tiles->pixels + y * tiles->pitch + x * bpp);

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

			double rh = red / 255.0;
			double gs = green / 255.0;
			double bv = blue / 255.0;
			rgb_to_hsv(&rh, &gs, &bv);

			// change contrast and recolor
			bv = 0.5 + bv * 0.25;
			rh = 2 * M_PI * hue / HUE_PRECISION;
			gs = 0.2;

			hsv_to_rgb(&rh, &gs, &bv);

			// back to Uint8
			red = rh * 255;
			green = gs * 255;
			blue = bv * 255;

			Uint16 *dst_px = (Uint16 *)((Uint8 *)tiles->pixels + y * tiles->pitch + x * bpp);
			*dst_px = (red >> fmt->Rloss) << fmt->Rshift |
				(green >> fmt->Gloss) << fmt->Gshift |
				(blue >> fmt->Bloss) << fmt->Bshift;
		}
	SDL_UnlockSurface(tiles);
}

void tiles_dispose(void)
{
	SDL_FreeSurface(tiles);
	tiles = NULL;
	SDL_FreeSurface(tiles_orig);
	tiles_orig = NULL;
}

void food_init(void)
{
	SDL_Surface *tmp = IMG_Load(GFX_DIR "fruits-16.png");
	fruits = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);

	tmp = IMG_Load(GFX_DIR "veggies-16.png");
	veggies = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);
}

// it does not need init before as a side effect
void food_recolor(int hue)
{
	food_dispose();
	food_init();
	surface_recolor(fruits, hue);
	surface_recolor(veggies, hue);
}

void food_dispose(void)
{
	if (fruits)
	{
		SDL_FreeSurface(fruits);
		fruits = NULL;
	}
	if (veggies)
	{
		SDL_FreeSurface(veggies);
		veggies = NULL;
	}
}

void get_sprite_from_food(enum Food food, SDL_Surface **surface, SDL_Rect *rect)
{
	const bool is_fruit = food < FRUIT_END;
	const int index = food - (is_fruit ? FRUIT_START : VEGE_START);
	*surface = is_fruit ? fruits : veggies;
	rect->w = rect->h = CONSUMABLE_SIZE;
	rect->x = CONSUMABLE_SIZE * (index % 6);	// spritesheet is 6x6
	rect->y = CONSUMABLE_SIZE * (index / 6);
}

static void get_rgba_values_32(Uint32 px, SDL_PixelFormat *fmt, Uint8 *red, Uint8 *green, Uint8 *blue, Uint8 *alpha)
{
	Uint32 temp;

	temp = px & fmt->Rmask;
	temp = temp >> fmt->Rshift;
	temp = temp << fmt->Rloss;
	*red = (Uint8)temp;
	temp = px & fmt->Gmask;
	temp = temp >> fmt->Gshift;
	temp = temp << fmt->Gloss;
	*green = (Uint8)temp;
	temp = px & fmt->Bmask;
	temp = temp >> fmt->Bshift;
	temp = temp << fmt->Bloss;
	*blue = (Uint8)temp;
	temp = px & fmt->Amask;
	temp = temp >> fmt->Ashift;
	temp = temp << fmt->Aloss;
	*alpha = (Uint8)temp;
}

static Uint8 mix_color_channel(const Uint8 color[4], const double frac[4])
{
	double result = (color[0] * frac[2] + color[2] * frac[0]) * frac[3] +
		(color[1] * frac[2] + color[3] * frac[0]) * frac[1];
	return (Uint8)result;
}

static void parts_generate_rotated(SDL_Surface **orig)
{
	SDL_Surface *tmp = *orig;
	*orig = SDL_CreateRGBSurface(0,
		SNAKE_PART_SIZE * ROT_ANGLE_COUNT, SNAKE_PART_SIZE,
		tmp->format->BitsPerPixel,
		tmp->format->Rmask,
		tmp->format->Gmask,
		tmp->format->Bmask,
		tmp->format->Amask);
	SDL_LockSurface(*orig);
	for (int i = 0; i < ROT_ANGLE_COUNT; ++i)
	{
		double angle = i * 2 * M_PI / ROT_ANGLE_COUNT;
		double sina = sin(angle);
		double cosa = cos(angle);
		int offset = i * SNAKE_PART_SIZE;
		for (int y = 0; y < SNAKE_PART_SIZE; ++y)
			for (int x = 0; x < SNAKE_PART_SIZE; ++x)
			{
#if BILINEAR_FILTERING
				Uint32 *p = (Uint32 *)((Uint8 *)(*orig)->pixels + y * (*orig)->pitch + (x + offset) * (*orig)->format->BytesPerPixel);
				int ox = x - SNAKE_PART_SIZE / 2;
				int oy = y - SNAKE_PART_SIZE / 2;
				double tx = ox * cosa + oy * sina;
				double ty = -ox * sina + oy * cosa;
				tx += SNAKE_PART_SIZE / 2;
				ty += SNAKE_PART_SIZE / 2;
				const int ix = tx;
				const int iy = ty;
				const double u_ratio = tx - ix;
				const double v_ratio = ty - iy;
				const double u_opposite = 1 - u_ratio;
				const double v_opposite = 1 - v_ratio;
				const double frac[4] = {u_ratio, v_ratio, u_opposite, v_opposite};

				// right and bottom boundaries are not handled correctly
				// needs to be done _in_the_future_ :)
				if (ix >= 0 && ix < (SNAKE_PART_SIZE - 1) &&
					iy >= 0 && iy < (SNAKE_PART_SIZE - 1))
				{
					Uint32 tp00 = *(Uint32 *)((Uint8 *)tmp->pixels + iy * tmp->pitch + ix * tmp->format->BytesPerPixel);
					Uint32 tp01 = *(Uint32 *)((Uint8 *)tmp->pixels + (iy + 1) * tmp->pitch + ix * tmp->format->BytesPerPixel);
					Uint32 tp10 = *(Uint32 *)((Uint8 *)tmp->pixels + iy * tmp->pitch + (ix + 1) * tmp->format->BytesPerPixel);
					Uint32 tp11 = *(Uint32 *)((Uint8 *)tmp->pixels + (iy + 1) * tmp->pitch + (ix + 1) * tmp->format->BytesPerPixel);

					Uint8 red[4], green[4], blue[4], alpha[4];
					get_rgba_values_32(tp00, tmp->format, &red[0], &green[0], &blue[0], &alpha[0]);
					get_rgba_values_32(tp01, tmp->format, &red[1], &green[1], &blue[1], &alpha[1]);
					get_rgba_values_32(tp10, tmp->format, &red[2], &green[2], &blue[2], &alpha[2]);
					get_rgba_values_32(tp11, tmp->format, &red[3], &green[3], &blue[3], &alpha[3]);

					Uint8 rred, rgreen, rblue, ralpha;
					rred = mix_color_channel(red, frac);
					rgreen = mix_color_channel(green, frac);
					rblue = mix_color_channel(blue, frac);
					ralpha = mix_color_channel(alpha, frac);

					const SDL_PixelFormat *fmt = tmp->format;
					*p = (rred >> fmt->Rloss) << fmt->Rshift |
						(rgreen >> fmt->Gloss) << fmt->Gshift |
						(rblue >> fmt->Bloss) << fmt->Bshift |
						(ralpha >> fmt->Aloss) << fmt->Ashift;
				}
				else
				{
					// transparent
					*p = 0;
				}
#else
				Uint32 *p = (Uint32 *)((Uint8 *)(*orig)->pixels + y * (*orig)->pitch + (x + offset) * (*orig)->format->BytesPerPixel);
				int ox = x - SNAKE_PART_SIZE / 2;
				int oy = y - SNAKE_PART_SIZE / 2;
				double tx = ox * cosa + oy * sina;
				double ty = -ox * sina + oy * cosa;
				int ix = tx + SNAKE_PART_SIZE / 2;
				int iy = ty + SNAKE_PART_SIZE / 2;
				if (ix >= 0 && ix < SNAKE_PART_SIZE &&
					iy >= 0 && iy < SNAKE_PART_SIZE)
				{
					Uint32 *tp = (Uint32 *)((Uint8 *)tmp->pixels + iy * tmp->pitch + ix * tmp->format->BytesPerPixel);
					*p = *tp;
				}
				else
				{
					// transparent
					*p = 0;
				}
#endif
			}
	}
	SDL_UnlockSurface(*orig);

	SDL_FreeSurface(tmp);
}

void parts_init(void)
{
	SDL_Surface *tmp = IMG_Load(GFX_DIR "snake-head.png");
	snake_head[SKILL_NONE] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load(GFX_DIR "snake-head-ghost.png");
	snake_head[SKILL_GHOST] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load(GFX_DIR "snake-head-onix.png");
	snake_head[SKILL_ONIX] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load(GFX_DIR "snake-head-uroboros.png");
	snake_head[SKILL_UROBOROS] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);

	tmp = IMG_Load(GFX_DIR "snake-body.png");
	snake_body[SKILL_NONE] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load(GFX_DIR "snake-body-ghost.png");
	snake_body[SKILL_GHOST] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load(GFX_DIR "snake-body-onix.png");
	snake_body[SKILL_ONIX] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load(GFX_DIR "snake-body-uroboros.png");
	snake_body[SKILL_UROBOROS] = SDL_DisplayFormatAlpha(tmp);
	SDL_FreeSurface(tmp);

	for (int i = SKILL_NONE; i < SKILL_END; ++i)
	{
		parts_generate_rotated(&snake_head[i]);
	}
}

// it does not need init before as a side effect
void parts_recolor(int hue)
{
	parts_dispose();
	parts_init();
	for (int i = SKILL_NONE; i < SKILL_END; ++i)
	{
		surface_recolor(snake_head[i], hue);
		surface_recolor(snake_body[i], hue);
	}
}

void parts_dispose(void)
{
	for (int i = SKILL_NONE; i < SKILL_END; ++i)
	{
		if (snake_head[i])
		{
			SDL_FreeSurface(snake_head[i]);
			snake_head[i] = NULL;
		}
		if (snake_body[i])
		{
			SDL_FreeSurface(snake_body[i]);
			snake_body[i] = NULL;
		}
	}
}

void obstacle_free_surfaces(void)
{
	for (int i = 0; i < OBS_SHEETS_COUNT; ++i)
	{
		if (obstacle_surfaces[i])
		{
			SDL_FreeSurface(obstacle_surfaces[i]);
			obstacle_surfaces[i] = NULL;
		}
	}
}

SDL_Surface *obstacle_get_surface(int radius, Uint32 color, int style)
{
	if (NULL == obstacle_surfaces[radius])
	{
		char path[64];
		int ssize = radius * 2 + 4;
		int cr = (color >> 24) & 0xff;
		int cg = (color >> 16) & 0xff;
		int cb = (color >> 8) & 0xff;
		int ca = color & 0xff;
		int gears = obstacle_gears_num[style - 1];
		float angle_delta = 5 * M_PI / (OBS_FRAMERATE * gears);

		int frame_num = ceilf(2 * M_PI / (angle_delta * gears)) + 1;
		SDL_Surface *temp = SDL_CreateRGBSurface(0,
			ssize * frame_num, ssize, 32,
			0xff, 0xff00, 0xff0000, 0xff000000);
		obstacle_surfaces[radius] = SDL_DisplayFormatAlpha(temp);
		SDL_FreeSurface(temp);
		SDL_FillRect(obstacle_surfaces[radius], NULL, 0);

		sprintf(path, GFX_DIR "saw%d.svg", style);

		float angle = 0;
		int i = 0;
		for (angle = 0, i = 0; angle < (2 * M_PI / gears); angle += angle_delta, ++i)
		{
			SDL_Surface *temp2 = SVG_LoadSizedSVG_RW(path, ssize, ssize,
				cr, cg, cb, ca, angle);
			temp = SDL_DisplayFormatAlpha(temp2);
			SDL_FreeSurface(temp2);

			int bytes_num = obstacle_surfaces[radius]->pitch;
			if (bytes_num > temp->pitch)
				bytes_num = temp->pitch;
			// funny thing, the heights below may be unequal
			for (int y = 0; y < obstacle_surfaces[radius]->h && y < temp->h; ++y)
			{
				char *to = obstacle_surfaces[radius]->pixels;
				char *from = temp->pixels;
				int offset_to = y * obstacle_surfaces[radius]->pitch + (i * ssize * 4);
				int offset_from = y * temp->pitch;
				memcpy(to + offset_to, from + offset_from, bytes_num);
			}
			// I could not make the function below do simple copying
			// SDL_BlitSurface(temp, NULL, obstacle_surfaces[radius], &dst);
			SDL_FreeSurface(temp);
		}

		// number of generated frames
		obstacle_framelimits[radius] = i;
		//printf("r=%d, limit=%d, frames=%d\n", radius, i, frame_num);
	}
	return obstacle_surfaces[radius];
}
