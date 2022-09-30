#ifndef _H_MAIN
#define _H_MAIN

#include <SDL.h>

#define SCREEN_WIDTH					(320)
#define SCREEN_HEIGHT					(240)
#if defined(MIYOO)
#define SCREEN_BPP						(16)
#define FPS_LIMIT						(60)
#define ANTIALIASING_OFF
#else
#define SCREEN_BPP						(32)
#define FPS_LIMIT						(60)
//#define ANTIALIASING_OFF
#endif

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

enum LevelSize
{
	LS_SMALL,
	LS_REGULAR,
	LS_LARGE,
	LS_NUM
};

enum Wobbliness
{
	W_NONE,
	W_LITTLE,
	W_MUCH,
	W_NUM
};

enum MenuOptions
{
	MO_LEVELSIZE,
	MO_WOBBLINESS,
	MO_NUM
};

extern int menu_options[];
extern SDL_Surface *screen;
extern enum GameState gamestate;

#endif
