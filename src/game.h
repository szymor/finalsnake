#ifndef _H_GAME
#define _H_GAME

#include <stdbool.h>
#include <SDL.h>
#include "gfx.h"

#define MAX_SNAKE_LEN					(10240)
#define START_LEN						(60)
#define HEAD_RADIUS						(5.0)
#define BODY_RADIUS						(4.0)
#define PIECE_DISTANCE					(0.5)
#define PIECE_DRAW_INCREMENT			(12)
#define CONSUMABLE_RADIUS				(6.0)
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
};

enum CameraMode
{
	CM_FIXED,
	CM_TRACKING,
	CM_TPP,
	CM_TPP_DELAYED,
	CM_END
};

enum Turn
{
	TURN_NONE,
	TURN_LEFT,
	TURN_RIGHT
};

enum ConsumableGenerationMode
{
	CGM_CARTESIAN,
	CGM_POLAR
};

struct Snake
{
	double v;	// linear speed
	double base_v;
	double w;	// angular speed
	double base_w;
	double dir;	// angular position
	double wobbly_freq;
	double wobbly_phase;	// wobbly phase
	int len;
	struct Vec2D pieces[MAX_SNAKE_LEN];
	enum Turn turn;
};

struct Consumable
{
	struct Segment segment;
	double phase;
	enum Food type;
	SDL_Surface *food_surface;
	SDL_Rect src_rect;
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
	double angle_store;
	const double *target_angle;
};

struct Room
{
	struct Snake snake;
	enum ConsumableGenerationMode cg_mode;
	union
	{
		struct
		{
			// origin at (0,0)
			double radius;
		} cg_polar;
		struct
		{
			struct Vec2D upper_left;
			struct Vec2D bottom_right;
		} cg_cartesian;
	};
	struct Consumable *consumables;
	int consumables_num;
	struct Wall *walls;
	int walls_num;
	struct Obstacle *obstacles;
	int obstacles_num;
	Uint32 wall_color;
	int obstacle_style;
};

void fps_counter(double dt);
void fps_draw(void);
#if FPS_LIMIT != 0
void fps_limiter(void);
#endif
void pause_draw(bool paused);

struct Vec2D* vadd(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vsub(struct Vec2D *dst, const struct Vec2D *elem);
struct Vec2D* vmul(struct Vec2D *dst, double scalar);
double vdot(const struct Vec2D *vec1, const struct Vec2D *vec2);
double vlen(const struct Vec2D *vec);
double vdist(const struct Vec2D *vec1, const struct Vec2D *vec2);

bool generate_safe_position(
	const struct Room *room, struct Vec2D *pos,
	double safe_distance, int max_attempts,
	bool snake, bool wall, bool obstacle);

void camera_prepare(const struct Snake *target, enum CameraMode cm);
void camera_convert(double *x, double *y);
void camera_process(double dt);

void snake_init(struct Snake *snake);
void snake_process(struct Snake *snake, double dt);
void snake_draw(const struct Snake *snake);
void snake_control(struct Snake *snake);
void snake_add_segments(struct Snake *snake, int count);
void snake_eat_consumables(struct Snake *snake, struct Room *room);
bool snake_check_selfcollision(const struct Snake *snake);
bool snake_check_wallcollision(const struct Snake *snake, struct Wall walls[], int wallnum);
bool snake_check_obstaclecollision(const struct Snake *snake, struct Obstacle obs[], int obnum);

void consumable_generate(struct Consumable *col, const struct Room *room);
void consumable_process(struct Consumable *col, double dt);
void consumable_draw(struct Consumable *col);

void wall_init(struct Wall *wall, double x1, double y1, double x2, double y2, double r);
void wall_draw(const struct Wall *wall, Uint32 color);
struct Vec2D* wall_dist(const struct Wall *wall, const struct Vec2D *pos);

void obstacle_init(struct Obstacle *obstacle, double x, double y, double r);
void obstacle_draw(const struct Obstacle *obstacle, Uint32 color, int style);

void room_init(struct Room *room);
void room_dispose(struct Room *room);
void room_process(struct Room *room, double dt);
void room_draw(const struct Room *room);
bool room_check_gameover(const struct Room *room);

#endif
