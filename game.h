#ifndef _H_GAME
#define _H_GAME

#include <stdbool.h>
#include <SDL.h>

#define MAX_SNAKE_LEN					(1024)
#define START_LEN						(5)
#define HEAD_RADIUS						(5.0)
#define BODY_RADIUS						(4.0)
#define SEGMENT_DISTANCE				(6.0)
#define COLLECTIBLE_RADIUS				(4.0)
#define COLLECTIBLE_COUNT				(3)
#define COLLECTIBLE_SAFE_DISTANCE		(25.0)
#define EAT_DEPTH						(2.0)
#define SNAKE_V_MULTIPLIER				(2.0)
#define SNAKE_W_MULTIPLIER				(1.5)

#define SDL_CHECK(x) if (x) { printf("SDL: %s\n", SDL_GetError()); exit(0); }
#define SDLGFX_COLOR(r, g, b) (((r) << 24) | ((g) << 16) | ((b) << 8) | 0xff)

#define SINCOS_FIX_INC(x)		if ((x) >  M_PI) (x) -= 2 * M_PI;
#define SINCOS_FIX_DEC(x)		if ((x) < -M_PI) (x) += 2 * M_PI;

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
	CM_TPP,
	CM_END
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

struct Obstacle
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
	const struct Vec2D *center;
	const double *angle;
};

struct Room
{
	struct Snake snake;
	struct Collectible col[COLLECTIBLE_COUNT];
	struct Wall *walls;
	int wallnum;
	struct Obstacle *obs;
	int obnum;
};

void fps_counter(double dt);
void fps_draw(void);
#if FPS_LIMIT != 0
void fps_limiter(void);
#endif

struct Vec2D* vadd(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vsub(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vmul(struct Vec2D *dst, double scalar);
double vdot(const struct Vec2D *vec1, const struct Vec2D *vec2);
double vlen(const struct Vec2D *vec);
double vdist(const struct Vec2D *vec1, const struct Vec2D *vec2);

void camera_prepare(const struct Snake *target, enum CameraMode cm);
void camera_convert(double *x, double *y);

void snake_init(struct Snake *snake);
void snake_process(struct Snake *snake, double dt);
void snake_draw(const struct Snake *snake);
void snake_control(struct Snake *snake);
void snake_add_segments(struct Snake *snake, int count);
void snake_eat_collectibles(struct Snake *snake, struct Room *room);
bool snake_check_selfcollision(const struct Snake *snake);
bool snake_check_wallcollision(const struct Snake *snake, struct Wall walls[], int wallnum);
bool snake_check_obstaclecollision(const struct Snake *snake, struct Obstacle obs[], int obnum);

void collectible_generate(struct Collectible *col, const struct Room *room);
void collectible_process(struct Collectible *col, double dt);
void collectible_draw(const struct Collectible *col);

void wall_init(struct Wall *wall, double x1, double y1, double x2, double y2, double r);
void wall_draw(const struct Wall *wall);
struct Vec2D* wall_dist(const struct Wall *wall, const struct Vec2D *pos);

void obstacle_init(struct Obstacle *obstacle, double x, double y, double r);
void obstacle_draw(const struct Obstacle *obstacle);

void room_init(struct Room *room, int level);
void room_dispose(struct Room *room);
void room_process(struct Room *room, double dt);
void room_draw(const struct Room *room);
bool room_check_gameover(const struct Room *room);

#endif
