#include <stdbool.h>
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <time.h>

#define SCREEN_WIDTH		(640)
#define SCREEN_HEIGHT		(480)
#define SCREEN_BPP			(32)

#define MAX_SNAKE_LEN			(1024)
#define SEGMENT_DISTANCE		(10.0)

#define SDL_CHECK(x) if (x) { printf("SDL: %s\n", SDL_GetError()); exit(0); }

struct Vec2D
{
	double x;
	double y;
};

struct Segment
{
	struct Vec2D pos;
	double r;
};

enum Turn
{
	TURN_NONE,
	TURN_LEFT,
	TURN_RIGHT
};

struct Snake
{
	double v;
	double dir;
	int len;
	struct Segment segments[MAX_SNAKE_LEN];
	enum Turn turn;
};

struct Collectible
{
	struct Segment segment;
	Uint32 color;
};

SDL_Surface *screen = NULL;

struct Vec2D* vadd(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vsub(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vmul(struct Vec2D *dst, double scalar);
double vlen(const struct Vec2D *vec);

void snake_init(struct Snake *snake);
void snake_process(struct Snake *snake, double dt);
void snake_draw(const struct Snake *snake);
void snake_control(struct Snake *snake);
void snake_add_segments(struct Snake *snake, int count);
void snake_eat_collectibles(struct Snake *snake, struct Collectible cols[], int colnum);

void collectible_init(struct Collectible *col)
{
	col->segment = (struct Segment)
		{ .pos = { .x = rand() % SCREEN_WIDTH, .y = rand() % SCREEN_HEIGHT },
			.r = 5.0
		};
	col->color = SDL_MapRGB(screen->format, rand() % 255, rand() % 255, rand() % 255);
}

void collectible_process(struct Collectible *col)
{
	// placeholder for moving collectibles
}

void collectible_draw(struct Collectible *col)
{
	filledCircleColor(screen, col->segment.pos.x, col->segment.pos.y, col->segment.r, col->color);
}

int main(int argc, char *argv[])
{
	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0);
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
	SDL_CHECK(screen == NULL);
	SDL_WM_SetCaption("Final Snake prealpha", NULL);

	// game data init
	srand(time(NULL));
	struct Snake snake;
	snake_init(&snake);
	snake_add_segments(&snake, 5);
	struct Collectible col;
	collectible_init(&col);

	SDL_Event event;
	bool quit = false;
	Uint32 prevtime = SDL_GetTicks();
	while (!quit)
	{
		if (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							quit = true;
							break;
					}
					break;
				case SDL_QUIT:
					quit = true;
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

			snake_control(&snake);

			collectible_process(&col);
			snake_process(&snake, dt);
			snake_eat_collectibles(&snake, &col, 1);

			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 255, 255));
			collectible_draw(&col);
			snake_draw(&snake);
			SDL_Flip(screen);
		}
	}
	return 0;
}

struct Vec2D* vadd(struct Vec2D *dst, const struct Vec2D *elem)
{
	dst->x += elem->x;
	dst->y += elem->y;
	return dst;
}

struct Vec2D* vsub(struct Vec2D *dst, const struct Vec2D *elem)
{
	dst->x -= elem->x;
	dst->y -= elem->y;
	return dst;
}

struct Vec2D* vmul(struct Vec2D *dst, double scalar)
{
	dst->x *= scalar;
	dst->y *= scalar;
	return dst;
}

double vlen(const struct Vec2D *vec)
{
	return sqrt(vec->x * vec->x + vec->y * vec->y);
}

void snake_init(struct Snake *snake)
{
	snake->v = 70.0;
	snake->dir = 0.0;
	snake->len = 1;
	snake->segments[0] = (struct Segment)
		{ .pos = { .x = SCREEN_WIDTH/2, .y = SCREEN_HEIGHT/2 },
			.r = 10.0
		};
	snake->turn = TURN_NONE;
}

void snake_process(struct Snake *snake, double dt)
{
	// control processing
	switch (snake->turn)
	{
		case TURN_LEFT:
			snake->dir -= M_PI * dt;
			break;
		case TURN_RIGHT:
			snake->dir += M_PI * dt;
			break;
	}

	// head calculation
	struct Vec2D offset = {
		.x = snake->v * sin(snake->dir) * dt,
		.y = -snake->v * cos(snake->dir) * dt
	};
	vadd(&snake->segments[0].pos, &offset);

	// tail calculation
	for (int i = 1; i < snake->len; ++i)
	{
		struct Vec2D diff;
		diff = snake->segments[i-1].pos;
		vsub(&diff, &snake->segments[i].pos);
		double dlen = vlen(&diff);
		if (dlen > SEGMENT_DISTANCE)
		{
			vmul(&diff, (dlen - SEGMENT_DISTANCE) / dlen);
			vadd(&snake->segments[i].pos, &diff);
		}
	}
}

void snake_draw(const struct Snake *snake)
{
	for (int i = 0; i < snake->len; ++i)
	{
		aacircleRGBA(screen, snake->segments[i].pos.x, snake->segments[i].pos.y, snake->segments[i].r, 0, 0, 0, 255);
	}
}

void snake_control(struct Snake *snake)
{
	Uint8 *keystate = SDL_GetKeyState(NULL);
	if (keystate[SDLK_LEFT] && !keystate[SDLK_RIGHT])
	{
		snake->turn = TURN_LEFT;
	}
	else if (keystate[SDLK_RIGHT] && !keystate[SDLK_LEFT])
	{
		snake->turn = TURN_RIGHT;
	}
	else
	{
		snake->turn = TURN_NONE;
	}
}

void snake_add_segments(struct Snake *snake, int count)
{
	int start = snake->len;
	snake->len += count;
	for (int i = start; i < snake->len; ++i)
	{
		snake->segments[i].r = 7.0;
		snake->segments[i].pos = snake->segments[start - 1].pos;
	}
}

void snake_eat_collectibles(struct Snake *snake, struct Collectible cols[], int colnum)
{
	struct Vec2D diff = snake->segments[0].pos;
	vsub(&diff, &cols[0].segment.pos);
	if (vlen(&diff) < (snake->segments[0].r + cols[0].segment.r))
	{
		snake_add_segments(snake, 1);
		collectible_init(&cols[0]);
	}
}
