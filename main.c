#include <stdbool.h>
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <time.h>

#define SCREEN_WIDTH			(640)
#define SCREEN_HEIGHT			(480)
#define SCREEN_BPP				(32)
#define FPS_LIMIT				(60)

#define MAX_SNAKE_LEN			(1024)
#define START_LEN				(5)
#define SEGMENT_DISTANCE		(10.0)
#define COLLECTIBLE_COUNT		(3)
#define SNAKE_V_MULTIPLIER		(2.0)
#define SNAKE_W_MULTIPLIER		(1.5)
#define SNAKE_EAT_MULTIPLIER	(1.08)

#define SDL_CHECK(x) if (x) { printf("SDL: %s\n", SDL_GetError()); exit(0); }
#define SDLGFX_COLOR(r, g, b) (((r) << 24) | ((g) << 16) | ((b) << 8) | 0xff)

struct Vec2D
{
	double x;
	double y;
};

struct Segment
{
	struct Vec2D pos;
	double r;
	Uint32 color;
};

enum CameraMode
{
	CM_FIXED,
	CM_TRACKING,
	CM_TPP
};

enum Turn
{
	TURN_NONE,
	TURN_LEFT,
	TURN_RIGHT
};

struct Snake
{
	double v;	// linear speed
	double base_v;
	double w;	// angular speed
	double base_w;
	double dir;	// angular position
	int len;
	struct Segment segments[MAX_SNAKE_LEN];
	enum Turn turn;
};

struct Collectible
{
	struct Segment segment;
};

struct Wall
{
	struct Vec2D start;
	struct Vec2D end;
	double r;
};

struct Camera
{
	enum CameraMode cm;
	struct Vec2D *center;
	double *angle;
};

SDL_Surface *screen = NULL;

struct Vec2D* vadd(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vsub(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vmul(struct Vec2D *dst, double scalar);
double vdot(const struct Vec2D *vec1, const struct Vec2D *vec2);
double vlen(const struct Vec2D *vec);

struct Camera camera = {
	.cm = CM_FIXED,
	.center = NULL,
	.angle = NULL
};
void camera_convert(double *x, double *y);

void snake_init(struct Snake *snake);
void snake_process(struct Snake *snake, double dt);
void snake_draw(const struct Snake *snake);
void snake_control(struct Snake *snake);
void snake_add_segments(struct Snake *snake, int count);
void snake_eat_collectibles(struct Snake *snake, struct Collectible cols[], int colnum);
bool snake_check_selfcollision(const struct Snake *snake);
bool snake_check_wallcollision(const struct Snake *snake, struct Wall walls[], int wallnum);

void collectible_init(struct Collectible *col);
void collectible_process(struct Collectible *col, double dt);
void collectible_draw(const struct Collectible *col);

void wall_init(struct Wall *wall, double x1, double y1, double x2, double y2, double r);
void wall_draw(const struct Wall *wall);
struct Vec2D* wall_dist(const struct Wall *wall, const struct Vec2D *pos);

int fps = 0;
void fps_counter(double dt);
void fps_draw(void);
#if FPS_LIMIT != 0
void fps_limiter(void);
#endif

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
	snake_add_segments(&snake, START_LEN - 1);
	struct Collectible col[COLLECTIBLE_COUNT];
	for (int i = 0; i < COLLECTIBLE_COUNT; ++i)
	{
		collectible_init(&col[i]);
	}
	struct Wall wallie;
	wall_init(&wallie, 50, 50, 100, 150, 10);

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
						case SDLK_1:
							camera.cm = CM_FIXED;
							break;
						case SDLK_2:
							camera.cm = CM_TRACKING;
							camera.center = &snake.segments[0].pos;
							break;
						case SDLK_3:
							camera.cm = CM_TPP;
							camera.center = &snake.segments[0].pos;
							camera.angle = &snake.dir;
							break;
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
#if FPS_LIMIT != 0
			fps_limiter();
#endif
			fps_counter(dt);

			snake_control(&snake);

			for (int i = 0; i < COLLECTIBLE_COUNT; ++i)
			{
				collectible_process(&col[i], dt);
			}
			snake_process(&snake, dt);
			wall_dist(&wallie, &snake.segments[0].pos);
			snake_eat_collectibles(&snake, col, COLLECTIBLE_COUNT);

			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 128, 128, 128));
			for (int i = 0; i < COLLECTIBLE_COUNT; ++i)
			{
				collectible_draw(&col[i]);
			}
			wall_draw(&wallie);
			snake_draw(&snake);
			fps_draw();
			SDL_Flip(screen);
			if (snake_check_selfcollision(&snake) ||
				snake_check_wallcollision(&snake, &wallie, 1))
			{
				SDL_Delay(1000);
				quit = true;
			}
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

double vdot(const struct Vec2D *vec1, const struct Vec2D *vec2)
{
	return vec1->x * vec2->x + vec1->y * vec2->y;
}

double vlen(const struct Vec2D *vec)
{
	return sqrt(vec->x * vec->x + vec->y * vec->y);
}

void snake_init(struct Snake *snake)
{
	snake->base_v = 70.0;
	snake->base_w = M_PI;
	snake->dir = 0.0;
	snake->len = 1;
	snake->segments[0] = (struct Segment)
		{ .pos = { .x = SCREEN_WIDTH/2, .y = SCREEN_HEIGHT/2 },
			.r = 10.0,
			.color = SDLGFX_COLOR(0, 0, 0)
		};
	snake->turn = TURN_NONE;
}

void snake_process(struct Snake *snake, double dt)
{
	// control processing
	switch (snake->turn)
	{
		case TURN_LEFT:
			snake->dir -= snake->w * dt;
			break;
		case TURN_RIGHT:
			snake->dir += snake->w * dt;
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
	for (int i = snake->len - 1; i >= 0; --i)
	{
		double x = snake->segments[i].pos.x;
		double y = snake->segments[i].pos.y;
		camera_convert(&x, &y);
		filledCircleColor(screen, x, y, snake->segments[i].r, snake->segments[i].color);
		aacircleRGBA(screen, x, y, snake->segments[i].r, 64, 64, 64, 255);
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
	if (keystate[SDLK_UP])
	{
		snake->v = snake->base_v * SNAKE_V_MULTIPLIER;
		snake->w = snake->base_w * SNAKE_W_MULTIPLIER;
	}
	else
	{
		snake->v = snake->base_v;
		snake->w = snake->base_w;
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
		snake->segments[i].color = SDLGFX_COLOR(rand() % 32, rand() % 32, rand() % 32);
	}
}

void snake_eat_collectibles(struct Snake *snake, struct Collectible cols[], int colnum)
{
	for (int i = 0; i < COLLECTIBLE_COUNT; ++i)
	{
		struct Vec2D diff = snake->segments[0].pos;
		vsub(&diff, &cols[i].segment.pos);
		if (vlen(&diff) < (snake->segments[0].r + cols[i].segment.r))
		{
			snake_add_segments(snake, 1);
			//snake->base_v *= SNAKE_EAT_MULTIPLIER;
			collectible_init(&cols[i]);
		}
	}
}

bool snake_check_selfcollision(const struct Snake *snake)
{
	for (int i = START_LEN; i < snake->len; ++i)
	{
		struct Vec2D diff = snake->segments[0].pos;
		vsub(&diff, &snake->segments[i].pos);
		if (vlen(&diff) < (snake->segments[0].r + snake->segments[i].r))
		{
			return true;
		}
	}
	return false;
}

bool snake_check_wallcollision(const struct Snake *snake, struct Wall walls[], int wallnum)
{
	for (int i = 0; i < wallnum; ++i)
	{
		struct Vec2D *vdist = wall_dist(&walls[i], &snake->segments[0].pos);
		if (vlen(vdist) < (snake->segments[0].r + walls[i].r))
		{
			return true;
		}
	}
	return false;
}

void collectible_init(struct Collectible *col)
{
	col->segment = (struct Segment)
		{ .pos = { .x = rand() % SCREEN_WIDTH, .y = rand() % SCREEN_HEIGHT },
			.r = 5.0,
			.color = SDLGFX_COLOR(rand() % 255, rand() % 255, rand() % 255)
		};
}

void collectible_process(struct Collectible *col, double dt)
{
	// placeholder for moving collectibles
}

void collectible_draw(const struct Collectible *col)
{
	double x = col->segment.pos.x;
	double y = col->segment.pos.y;
	camera_convert(&x, &y);
	filledCircleColor(screen, x, y, col->segment.r, col->segment.color);
	aacircleRGBA(screen, x, y, col->segment.r, 0, 0, 0, 255);
}

void camera_convert(double *x, double *y)
{
	switch (camera.cm)
	{
		case CM_FIXED:
			// nothing to do
			break;
		case CM_TRACKING:
			*x = *x - camera.center->x + SCREEN_WIDTH / 2;
			*y = *y - camera.center->y + SCREEN_HEIGHT / 2;
			break;
		case CM_TPP:
			*x -= camera.center->x;
			*y -= camera.center->y;
			double oldx = *x;
			double oldy = *y;
			double sinfi = sin(*camera.angle);
			double cosfi = cos(*camera.angle);
			*x = oldx * cosfi + oldy * sinfi;
			*y = -oldx * sinfi + oldy * cosfi;
			*x += SCREEN_WIDTH / 2;
			*y += SCREEN_HEIGHT / 2;
			break;
	}
}

void fps_counter(double dt)
{
	static double total = 0;
	static int count = 0;
	total += dt;
	++count;
	if (total > 1.0)
	{
		fps = count;
		total -= 1.0;
		count = 0;
	}
}

void fps_draw(void)
{
	char string[8] = "";
	sprintf(string, "%d", fps);
	stringRGBA(screen, 0, 0, string, 255, 255, 255, 255);
}

#if FPS_LIMIT != 0
void fps_limiter(void)
{
	static const Uint32 target_frame_duration = 1000 / FPS_LIMIT;
	static Uint32 prev = 0;
	Uint32 curr = SDL_GetTicks();
	if (0 == prev)
	{
		prev = curr;
	}
	while (curr < prev + target_frame_duration)
	{
		SDL_Delay(1);
		curr = SDL_GetTicks();
	}
	prev = curr;
}
#endif

void wall_init(struct Wall *wall, double x1, double y1, double x2, double y2, double r)
{
	wall->start.x = x1;
	wall->start.y = y1;
	wall->end.x = x2;
	wall->end.y = y2;
	wall->r = r;
}

void wall_draw(const struct Wall *wall)
{
	double x1 = wall->start.x;
	double y1 = wall->start.y;
	double x2 = wall->end.x;
	double y2 = wall->end.y;
	camera_convert(&x1, &y1);
	camera_convert(&x2, &y2);
	thickLineRGBA(screen, x1, y1, x2, y2, 2 * wall->r + 1, 0, 0, 0, 255);
	filledCircleRGBA(screen, x1, y1, wall->r, 0, 0, 0, 255);
	filledCircleRGBA(screen, x2, y2, wall->r, 0, 0, 0, 255);
}

struct Vec2D* wall_dist(const struct Wall *wall, const struct Vec2D *pos)
{
	static struct Vec2D dist_vector;

	struct Vec2D wall_vector = wall->end;
	vsub(&wall_vector, &wall->start);
	struct Vec2D pos_vector = *pos;
	vsub(&pos_vector, &wall->start);
	double walllen = vlen(&wall_vector);
	double projlen = vdot(&wall_vector, &pos_vector) / walllen;
	double ratio = projlen / walllen;

	if (ratio < 0.0)		// distance to the wall start
	{
		dist_vector = pos_vector;
	}
	else if (ratio > 1.0)	// distance to the wall end
	{
		dist_vector = *pos;
		vsub(&dist_vector, &wall->end);
	}
	else					// perpendicular distance
	{
		dist_vector = pos_vector;
		struct Vec2D projection = wall_vector;
		vmul(&projection, ratio);
		vsub(&dist_vector, &projection);
	}

	return &dist_vector;
}
