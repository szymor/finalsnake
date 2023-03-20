#ifndef _H_GFX
#define _H_GFX

#include <SDL.h>
#include "main.h"

#define HUE_PRECISION		(256)
#define SUIT_COUNT			(4)
#define CONSUMABLE_SIZE		(16)
#define SNAKE_PART_SIZE		(16)
#define ROT_ANGLE_COUNT		(64)
#define BILINEAR_FILTERING	(1)

// radius of the obstacle cannot be equal or greater than this
#define OBS_SHEETS_COUNT	(256)
#define OBS_STYLES_COUNT	(6)
#define OBS_FRAMERATE		(60)

// it MUST be with no parentheses
#define CHECKERBOARD_SIZE				64
//#define CHECKERBOARD_OFF

#define FRUITS_COUNT	(FRUIT_END - FRUIT_START)
#define VEGGIES_COUNT	(VEGE_END - VEGE_START)

extern SDL_Surface *tiles;
extern SDL_Surface *fruits;
extern SDL_Surface *veggies;
extern SDL_Surface *snake_head;
extern SDL_Surface *snake_body;

extern int obstacle_framelimits[];

void tiles_init(void);
void tiles_prepare(int suit, int hue);
void tiles_dispose(void);

void food_init(void);
void food_recolor(int hue);
void food_dispose(void);
enum Food get_random_food(void);
void get_sprite_from_food(enum Food food, SDL_Surface **surface, SDL_Rect *rect);

void parts_init(void);
void parts_recolor(int hue);
void parts_dispose(void);

Uint32 get_wall_color(int hue);

void obstacle_free_surfaces(void);
// get and allocate/generate if needed
SDL_Surface *obstacle_get_surface(int radius, Uint32 color, int style);

#endif
