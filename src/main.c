#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_gfxPrimitives.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "game.h"
#include "gfx.h"

// maximum number of settings per option
#define MENU_SETTINGS_MAX			(3)
#define MENU_SETTING_STR_LEN_MAX	(16)
#define WORD_WRAP_MAX_LINE_LEN		(40)

enum GameState
{
	GS_MENU,
	GS_GAME,
	GS_GAMEOVER,
	GS_QUIT
};

SDL_Surface *screen = NULL;
enum GameState gamestate = GS_MENU;
Mix_Chunk *sfx_crunch = NULL;

int menu_options[MO_NUM];
int menu_options_num[MO_NUM] = {LT_NUM, W_NUM};
const char menu_options_text[MO_NUM][MENU_SETTINGS_MAX][MENU_SETTING_STR_LEN_MAX] = {
	{
		"cage", "polygon", "star"
	},
	{
		"absteiner", "normie", "boozer"
	}
};

void gs_menu_process(void);
void gs_game_process(void);
void gs_gameover_process(void);

bool get_random_proverb(char *proverb, int size);
void wrap_text_lines(char *text);

int main(int argc, char *argv[])
{
	srand(time(NULL));
	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0);
#if defined(MIYOO)
	const Uint32 vflags = SDL_SWSURFACE | SDL_DOUBLEBUF;
#else
	const Uint32 vflags = SDL_HWSURFACE | SDL_DOUBLEBUF;
#endif
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, vflags);
	SDL_CHECK(screen == NULL);
	SDL_WM_SetCaption("Final Snake alpha", NULL);
	SDL_ShowCursor(SDL_DISABLE);

	int img_flags = IMG_INIT_PNG;
	if (img_flags != IMG_Init(img_flags))
	{
		printf("IMG_Init: %s\n", IMG_GetError());
		exit(0);
	}

	int mix_flags = MIX_INIT_MOD;
	if (mix_flags != mix_flags & Mix_Init(mix_flags))
	{
		printf("Mix_Init: %s\n", Mix_GetError());
		exit(0);
	}
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 1, 1024) == -1)
	{
		printf("Mix_OpenAudio: %s\n", Mix_GetError());
		exit(0);
	}
	sfx_crunch = Mix_LoadWAV(SFX_DIR "crunch.wav");
	if (!sfx_crunch)
	{
		printf("Mix_LoadWAV: %s\n", Mix_GetError());
		// it is not a critical error, so let it pass
	}

	tiles_init();
	food_init();
	parts_init();

	while (GS_QUIT != gamestate)
	{
		switch (gamestate)
		{
			case GS_MENU:
				gs_menu_process();
				break;
			case GS_GAME:
				gs_game_process();
				break;
			case GS_GAMEOVER:
				gs_gameover_process();
				break;
			case GS_QUIT:
				// dummy state
				break;
		};
	}

	return 0;
}

void gs_menu_process(void)
{
	char proverb[256] = "If you need a helping hand, you'll find one at the end of your arm.\n";
	get_random_proverb(proverb, 256);
	wrap_text_lines(proverb);

	int selection = 0;
	SDL_Event event;
	bool leave = false;
	while (!leave)
	{
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

		char text[32];

		sprintf(text, "FINAL SNAKE");
		stringRGBA(screen, (SCREEN_WIDTH - 8 * strlen(text)) / 2,
			30, text, 255, 255, 255, 255);

		// proverb display
		const char *ptr = proverb;
		int ypos = 54;
		while (ptr[0] != 0 && ptr[1] != 0)
		{
			stringRGBA(screen, (SCREEN_WIDTH - 8 * strlen(ptr)) / 2,
				ypos, ptr, 255, 255, 255, 255);
			ypos += 12;
			while (*ptr++ != 0);
		}

		sprintf(text, "%c LEVEL TYPE", MO_LEVELTYPE == selection ? '>' : ' ');
		stringRGBA(screen, (SCREEN_WIDTH / 2 - 8 * strlen(text)) / 2,
			SCREEN_HEIGHT - 8 - 77, text, 255, 255, 255, 255);
		sprintf(text, "%s", menu_options_text[MO_LEVELTYPE][menu_options[MO_LEVELTYPE]]);
		stringRGBA(screen, SCREEN_WIDTH / 2 + (SCREEN_WIDTH / 2 - 8 * strlen(text)) / 2,
			SCREEN_HEIGHT - 8 - 77, text, 255, 255, 255, 255);

		sprintf(text, "%c WOBBLINESS", MO_WOBBLINESS == selection ? '>' : ' ');
		stringRGBA(screen, (SCREEN_WIDTH / 2 - 8 * strlen(text)) / 2,
			SCREEN_HEIGHT - 8 - 61, text, 255, 255, 255, 255);
		sprintf(text, "%s", menu_options_text[MO_WOBBLINESS][menu_options[MO_WOBBLINESS]]);
		stringRGBA(screen, SCREEN_WIDTH / 2 + (SCREEN_WIDTH / 2 - 8 * strlen(text)) / 2,
			SCREEN_HEIGHT - 8 - 61, text, 255, 255, 255, 255);

		sprintf(text, "by vamastah aka szymor");
		stringRGBA(screen, (SCREEN_WIDTH - 8 * strlen(text)) / 2,
			SCREEN_HEIGHT - 8 - 24, text, 255, 255, 255, 255);

#ifdef COMMIT_HASH
		sprintf(text, "commit " xstr(COMMIT_HASH));
#else
		sprintf(text, "unofficial build");
#endif
		stringRGBA(screen, (SCREEN_WIDTH - 8 * strlen(text)) / 2,
			SCREEN_HEIGHT - 8 - 12, text, 255, 255, 255, 255);

		SDL_Flip(screen);

		if (SDL_WaitEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_LEFT:
							menu_options[selection] = (menu_options[selection] + menu_options_num[selection] - 1) % menu_options_num[selection];
							break;
						case SDLK_RIGHT:
							menu_options[selection] = (menu_options[selection] + 1) % menu_options_num[selection];
							break;
						case SDLK_UP:
							selection = (selection + MO_NUM - 1) % MO_NUM;
							break;
						case SDLK_DOWN:
							selection = (selection + 1) % MO_NUM;
							break;
						case KEY_START:
							leave = true;
							gamestate = GS_GAME;
							break;
						case KEY_QUIT:
							leave = true;
							gamestate = GS_QUIT;
							break;
					}
					break;
				case SDL_QUIT:
					leave = true;
					gamestate = GS_QUIT;
					break;
			}
		}
		else
		{
			leave = true;
			gamestate = GS_QUIT;
		}
	}
}

void gs_game_process(void)
{
	bool paused = false;
	// game data init
	srand(time(NULL));
	enum CameraMode cm = CM_FIXED;
	struct Room room;
	room_init(&room);

	SDL_Event event;
	bool leave = false;
	Uint32 prevtime = SDL_GetTicks();
	while (!leave)
	{
		if (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_1:
							cm = CM_FIXED;
							camera_prepare(&room.snake, cm);
							break;
						case SDLK_2:
							cm = CM_TRACKING;
							camera_prepare(&room.snake, cm);
							break;
						case SDLK_3:
							cm = CM_TPP;
							camera_prepare(&room.snake, cm);
							break;
						case KEY_PREV_CM:
							cm = (cm + CM_END - 1) % CM_END;
							camera_prepare(&room.snake, cm);
							break;
						case KEY_NEXT_CM:
							cm = (cm + 1) % CM_END;
							camera_prepare(&room.snake, cm);
							break;
						case KEY_START:
							paused = !paused;
							break;
						case KEY_QUIT:
							leave = true;
							gamestate = GS_MENU;
							break;
					}
					break;
				case SDL_QUIT:
					leave = true;
					gamestate = GS_QUIT;
					break;
			}
		}
		else
		{
			// delta time evaluation
			Uint32 currtime = SDL_GetTicks();
			Uint32 delta = currtime - prevtime;
			prevtime = currtime;
			double dt = delta / 1000.0;
#if FPS_LIMIT != 0
			fps_limiter();
#endif
			fps_counter(dt);
			if (!paused)
			{
				room_process(&room, dt);
				camera_process(dt);
			}
			room_draw(&room);
			fps_draw();
			pause_draw(paused);
			SDL_Flip(screen);
			if (room_check_gameover(&room))
			{
				leave = true;
				gamestate = GS_GAMEOVER;
			}
		}
	}
	room_dispose(&room);
	obstacle_free_surfaces();
}

void gs_gameover_process(void)
{
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

	const char text[] = "GAME OVER";
	stringRGBA(screen, (SCREEN_WIDTH - 8 * strlen(text)) / 2,
		(SCREEN_HEIGHT - 8) / 2, text, 255, 255, 255, 255);

	SDL_Flip(screen);

	SDL_Event event;
	bool leave = false;
	while (!leave)
	{
		if (SDL_WaitEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case KEY_START:
						case KEY_QUIT:
							leave = true;
							gamestate = GS_MENU;
							break;
					}
					break;
				case SDL_QUIT:
					leave = true;
					gamestate = GS_QUIT;
					break;
			}
		}
	}
}

bool get_random_proverb(char *proverb, int size)
{
	int lineno = 0;
	FILE *file = NULL;
	file = fopen("proverbs.txt", "r");
	if (NULL == file)
		return false;

	// count number of lines
	int ch;
	while ((ch = fgetc(file)) != EOF)
	{
		if ('\n' == ch)
			++lineno;
	}

	fseek(file, 0, SEEK_SET);
	int proverbno = rand() % lineno + 1;
	for (int i = 0; i < proverbno; ++i)
		fgets(proverb, size, file);
	fclose(file);
	return true;
}

void wrap_text_lines(char *text)
{
	int ix = 0;
	int lastws = -1;
	int lastbr = -1;
	while (text[ix])
	{
		if (' ' == text[ix])
		{
			lastws = ix;
		}
		if ((ix - lastbr) == WORD_WRAP_MAX_LINE_LEN)
		{
			text[lastws] = '\0';
			lastbr = lastws;
		}
		++ix;
	}
	text[ix - 1] = '\0';
}
