#ifndef _H_GFX
#define _H_GFX

#include <SDL.h>

#define HUE_PRECISION		(256)
#define SUIT_COUNT			(4)
#define FOOD_SIZE			(16)
#define SNAKE_PART_SIZE		(16)
#define ROT_ANGLE_COUNT		(8)

// it MUST be with no parentheses
#define CHECKERBOARD_SIZE				64
//#define CHECKERBOARD_OFF

extern SDL_Surface *tiles;
extern SDL_Surface *fruits;
extern SDL_Surface *veggies;
extern SDL_Surface *snake_head;
extern SDL_Surface *snake_body;

void tiles_init(void);
void tiles_prepare(int suit, int hue);
void tiles_dispose(void);

void food_init(void);
void food_recolor(int hue);
SDL_Rect *food_get_random_rect(void);
void food_dispose(void);

void parts_init(void);
void parts_recolor(int hue);
void parts_dispose(void);

#endif
