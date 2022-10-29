#ifndef _H_MAIN
#define _H_MAIN

#include <SDL.h>

#define xstr(s) str(s)
#define str(s) #s

#define SCREEN_WIDTH					(320)
#define SCREEN_HEIGHT					(240)
#define SCREEN_BPP						(16)
#define FPS_LIMIT						(60)

// it MUST be with no parentheses
#define CHECKERBOARD_SIZE				64
//#define CHECKERBOARD_OFF

#define KEY_LEFT						SDLK_LEFT
#define KEY_RIGHT						SDLK_RIGHT
#if defined(MIYOO)
#define KEY_ACCELERATE					SDLK_LALT
#else
#define KEY_ACCELERATE					SDLK_UP
#endif
#define KEY_PREV_CM						SDLK_TAB
#define KEY_NEXT_CM						SDLK_BACKSPACE
#define KEY_START						SDLK_RETURN
#define KEY_QUIT						SDLK_ESCAPE

enum LevelType
{
	LT_CAGE,
	LT_POLYGON,
	LT_STAR,
	LT_NUM
};

enum Wobbliness
{
	W_ABSTEINER,
	W_NORMIE,
	W_BOOZER,
	W_NUM
};

enum MenuOptions
{
	MO_LEVELTYPE,
	MO_WOBBLINESS,
	MO_NUM
};

extern int menu_options[];
extern SDL_Surface *screen;
extern enum GameState gamestate;

#endif
