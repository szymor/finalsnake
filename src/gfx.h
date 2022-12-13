#ifndef _H_GFX
#define _H_GFX

#include <SDL.h>

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

enum Food
{
	FRUIT_START = 0,
	FRUIT_ACAI = FRUIT_START,
	FRUIT_APPLE,
	FRUIT_APRICOT,
	FRUIT_AVOCADO,
	FRUIT_BANANAS,
	FRUIT_BLACKBERRY,
	FRUIT_BLUEBERRY,
	FRUIT_CACTUS_PEAR,
	FRUIT_CANTALOUPE,
	FRUIT_CHERRY,
	FRUIT_CINDERBERRY,			// fantasy
	FRUIT_COCONUT,
	FRUIT_CRANBERRY,
	FRUIT_DESTINYFRUIT,			// fantasy
	FRUIT_DRAGONFRUIT,
	FRUIT_FIG,
	FRUIT_GRAPES,
	FRUIT_GREEN_APPLE,
	FRUIT_HONEYDEW,
	FRUIT_LEMON,
	FRUIT_LIME,
	FRUIT_LYCHEE,
	FRUIT_MANGO,
	FRUIT_METALBERRY,			// fantasy
	FRUIT_ORANGE,
	FRUIT_OREBERRY,				// fantasy
	FRUIT_PASSIONFRUIT,			// fantasy
	FRUIT_PEACH,
	FRUIT_PEAR,
	FRUIT_PINEAPPLE,
	FRUIT_POMEGRANATE,
	FRUIT_RASPBERRY,
	FRUIT_SOULFRUIT,			// fantasy
	FRUIT_STRAWBERRY,
	FRUIT_TOMATO,
	FRUIT_WATERMELON,
	FRUIT_END,
	VEGE_START = 64,
	VEGE_ARTICHOKE = VEGE_START,
	VEGE_BEETS,
	VEGE_BLACK_BEANS,
	VEGE_BOKCHOY,
	VEGE_BRUSSELS_SPROUTS,
	VEGE_CABBAGE,
	VEGE_CAYENNE,
	VEGE_CORN,
	VEGE_CUCUMBER,
	VEGE_DEVILS_LETTUCE,		// fantasy
	VEGE_GARLIC,
	VEGE_GHOST_PEPPER,			// fantasy
	VEGE_GINGER,
	VEGE_GOLD_MUSHROOM,			// fantasy
	VEGE_GREEN_BEANS,
	VEGE_HABANERO,
	VEGE_JALAPENO,
	VEGE_JELLYROOT,				// fantasy
	VEGE_LEEK,
	VEGE_ONION,
	VEGE_PIXIE_BEANS,			// fantasy
	VEGE_POTATO,
	VEGE_PUMPKIN,
	VEGE_RADDISH,
	VEGE_RED_BEANS,
	VEGE_RED_BELLPEPPER,
	VEGE_RHUBARB,
	VEGE_SHROOM1,
	VEGE_SHROOM2,
	VEGE_SHROOM3,
	VEGE_SPINACH,
	VEGE_TURNIP,
	VEGE_YAM,
	VEGE_YELLOW_BELLPEPPER,
	VEGE_CARROT,
	VEGE_EGGPLANT,
	VEGE_END
};

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
