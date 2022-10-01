#include <SDL_gfxPrimitives.h>
#include "main.h"
#include "game.h"

int fps = 0;

struct Camera camera = {
	.cm = CM_FIXED,
	.center = NULL,
	.angle = NULL
};

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

double vdist(const struct Vec2D *vec1, const struct Vec2D *vec2)
{
	struct Vec2D diff = *vec1;
	vsub(&diff, vec2);
	return vlen(&diff);
}

void snake_init(struct Snake *snake)
{
	snake->base_v = 40.0;
	snake->base_w = M_PI;
	snake->dir = 0.0;
	snake->len = 1;
	snake->segments[0] = (struct Segment)
		{ .pos = { .x = SCREEN_WIDTH/2, .y = SCREEN_HEIGHT/2 },
			.r = HEAD_RADIUS,
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
			SINCOS_FIX_DEC(snake->dir);
			break;
		case TURN_RIGHT:
			snake->dir += snake->w * dt;
			SINCOS_FIX_INC(snake->dir);
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
#ifndef ANTIALIASING_OFF
		aacircleRGBA(screen, x, y, snake->segments[i].r, 64, 64, 64, 255);
#endif
	}
}

void snake_control(struct Snake *snake)
{
	Uint8 *keystate = SDL_GetKeyState(NULL);
	if (keystate[KEY_LEFT] && !keystate[KEY_RIGHT])
	{
		snake->turn = TURN_LEFT;
	}
	else if (keystate[KEY_RIGHT] && !keystate[KEY_LEFT])
	{
		snake->turn = TURN_RIGHT;
	}
	else
	{
		snake->turn = TURN_NONE;
	}
	if (keystate[KEY_ACCELERATE])
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
		snake->segments[i].r = BODY_RADIUS;
		snake->segments[i].pos = snake->segments[start - 1].pos;
		snake->segments[i].color = SDLGFX_COLOR(rand() % 32, rand() % 32, rand() % 32);
	}
}

void snake_eat_collectibles(struct Snake *snake, struct Room *room)
{
	for (int i = 0; i < room->collectibles_num; ++i)
	{
		struct Vec2D diff = snake->segments[0].pos;
		vsub(&diff, &room->collectibles[i].segment.pos);
		if (vlen(&diff) < (snake->segments[0].r + room->collectibles[i].segment.r - EAT_DEPTH))
		{
			snake_add_segments(snake, 1);
			collectible_generate(&room->collectibles[i], room);
		}
	}
}

bool snake_check_selfcollision(const struct Snake *snake)
{
	for (int i = START_LEN + 1; i < snake->len; ++i)
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
		struct Vec2D *vector_distance = wall_dist(&walls[i], &snake->segments[0].pos);
		if (vlen(vector_distance) < (snake->segments[0].r + walls[i].r))
		{
			return true;
		}
	}
	return false;
}

bool snake_check_obstaclecollision(const struct Snake *snake, struct Obstacle obs[], int obnum)
{
	for (int i = 0; i < obnum; ++i)
	{
		struct Vec2D diff = snake->segments[0].pos;
		vsub(&diff, &obs[i].segment.pos);
		if (vlen(&diff) < (snake->segments[0].r + obs[i].segment.r))
		{
			return true;
		}
	}
	return false;
}

bool generate_safe_position(
	const struct Room *room, struct Vec2D *pos,
	double safe_distance, int max_attempts,
	bool snake, bool wall, bool obstacle)
{
	int x_from = room->collectible_limit_upper_left.x;
	int x_to = room->collectible_limit_bottom_right.x;
	int y_from = room->collectible_limit_upper_left.y;
	int y_to = room->collectible_limit_bottom_right.y;
	int attempts = 0;
	bool safe;
	do {
		safe = true;
		pos->x = rand() % (x_to - x_from) + x_from;
		pos->y = rand() % (y_to - y_from) + y_from;
		if (snake)
		{
			if (vdist(pos, &room->snake.segments[0].pos) < safe_distance)
			{
				safe = false;
				++attempts;
				continue;
			}
		}
		if (wall)
		{
			for (int i = 0; i < room->walls_num; ++i)
			{
				if ((vlen(wall_dist(&room->walls[i], pos)) - room->walls[i].r) < safe_distance)
				{
					safe = false;
					++attempts;
					break;
				}
			}
			if (!safe) continue;
		}
		if (obstacle)
		{
			for (int i = 0; i < room->obstacles_num; ++i)
			{
				if ((vdist(pos, &room->obstacles[i].segment.pos) - room->obstacles[i].segment.r) < safe_distance)
				{
					safe = false;
					++attempts;
					break;
				}
			}
			if (!safe) continue;
		}
	} while (!safe && (attempts < max_attempts));
	return safe;
}

void collectible_generate(struct Collectible *col, const struct Room *room)
{
	const int safe_distance = 15;

	col->segment = (struct Segment)
		{ .pos = { .x = 0, .y = 0 },
			.r = COLLECTIBLE_RADIUS,
			.color = SDLGFX_COLOR(rand() % 255, rand() % 255, rand() % 255)
		};

	if (!generate_safe_position(room, &col->segment.pos,
		safe_distance, 100, true, true, true))
	{
		// spawn it on top of the snake :)
		col->segment.pos = room->snake.segments[0].pos;
	}
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
#ifndef ANTIALIASING_OFF
	aacircleRGBA(screen, x, y, col->segment.r, 0, 0, 0, 255);
#endif
}

void camera_prepare(const struct Snake *target, enum CameraMode cm)
{
	camera.cm = cm;
	camera.center = &target->segments[0].pos;
	camera.angle = &target->dir;
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

#if defined(MIYOO)
	/* thickLineRGBA seems to have buggy implementation in MiyooCFW */
	struct Vec2D voff = { .x = x2 - x1, .y = y2 - y1 };
	vmul(&voff, wall->r / vlen(&voff));
	voff = (struct Vec2D){ .x = -voff.y, .y = voff.x };
	lineRGBA(screen, x1 + voff.x, y1 + voff.y, x2 + voff.x, y2 + voff.y, 0, 0, 0, 255);
	lineRGBA(screen, x1 - voff.x, y1 - voff.y, x2 - voff.x, y2 - voff.y, 0, 0, 0, 255);
	circleRGBA(screen, x1, y1, wall->r, 0, 0, 0, 255);
	circleRGBA(screen, x2, y2, wall->r, 0, 0, 0, 255);
#else
	thickLineRGBA(screen, x1, y1, x2, y2, 2 * wall->r + 1, 0, 0, 0, 255);
	filledCircleRGBA(screen, x1, y1, wall->r, 0, 0, 0, 255);
	filledCircleRGBA(screen, x2, y2, wall->r, 0, 0, 0, 255);
#endif
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

void obstacle_init(struct Obstacle *obstacle, double x, double y, double r)
{
	obstacle->segment.pos = (struct Vec2D){ .x = x, .y = y };
	obstacle->segment.r = r;
	obstacle->segment.color = 0;
}

void obstacle_draw(const struct Obstacle *obstacle)
{
	double x = obstacle->segment.pos.x;
	double y = obstacle->segment.pos.y;
	camera_convert(&x, &y);

#if defined(MIYOO)
	circleRGBA(screen, x, y, obstacle->segment.r, 0, 0, 0, 255);
#else
	filledCircleRGBA(screen, x, y, obstacle->segment.r, 0, 0, 0, 255);
#ifndef ANTIALIASING_OFF
	aacircleRGBA(screen, x, y, obstacle->segment.r, 0, 0, 0, 255);
#endif
#endif
}

void room_init(struct Room *room)
{
	struct Vec2D pos;

	room->collectibles_num = 0;
	room->collectibles = NULL;
	room->walls_num = 0;
	room->walls = NULL;
	room->obstacles_num = 0;
	room->obstacles = NULL;

	switch (menu_options[MO_LEVELSIZE])
	{
		case LS_SMALL:
		{
			const int min_obstacle_size = 12;
			const int max_obstacle_size = 24;
			room->collectibles_num = 3;
			room->collectibles = (struct Collectible *)malloc(room->collectibles_num * sizeof(struct Collectible));
			room->collectible_limit_upper_left = (struct Vec2D){ .x = 0, .y = 0};
			room->collectible_limit_bottom_right = (struct Vec2D){ .x = SCREEN_WIDTH, .y = SCREEN_HEIGHT};
			snake_init(&room->snake);
			snake_add_segments(&room->snake, START_LEN - 1);
			camera_prepare(&room->snake, CM_FIXED);
			room->walls_num = 4;
			room->walls = (struct Wall *)malloc(room->walls_num * sizeof(struct Wall));
			wall_init(&room->walls[0], 0, 0, SCREEN_WIDTH, 0, 10);
			wall_init(&room->walls[1], 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, 10);
			wall_init(&room->walls[2], 0, 0, 0, SCREEN_HEIGHT, 10);
			wall_init(&room->walls[3], SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10);
			room->obstacles_num = 16;
			room->obstacles = (struct Obstacle *)malloc(room->obstacles_num * sizeof(struct Obstacle));
			for (int i = 0; i < room->obstacles_num; ++i)
			{
				if (!generate_safe_position(room, &pos,
					48, 100, true, false, true))
				{
					/* out of the game field
					 * we cannot break the loop because
					 * we need to fill whole the allocated memory
					 */
					pos.x = -100;
					pos.y = -100;
				}
				obstacle_init(&room->obstacles[i], pos.x, pos.y,
					rand() % (max_obstacle_size - min_obstacle_size) + min_obstacle_size);
			}
		} break;
		case LS_REGULAR:
		{
			const int min_obstacle_size = 8;
			const int max_obstacle_size = 12;
			room->collectibles_num = 16;
			room->collectibles = (struct Collectible *)malloc(room->collectibles_num * sizeof(struct Collectible));
			room->collectible_limit_upper_left = (struct Vec2D){ .x = -SCREEN_WIDTH * 0.6, .y = -SCREEN_WIDTH * 0.6};
			room->collectible_limit_bottom_right = (struct Vec2D){ .x = SCREEN_WIDTH * 0.6, .y = SCREEN_WIDTH * 0.6};
			snake_init(&room->snake);
			room->snake.segments[0].pos = (struct Vec2D){ .x = 0, .y = 0 };
			snake_add_segments(&room->snake, START_LEN - 1);
			camera_prepare(&room->snake, CM_TRACKING);
			int outer_wall_num = rand() % 10 + 5;
			room->walls_num = outer_wall_num * 2;
			room->walls = (struct Wall *)malloc(room->walls_num * sizeof(struct Wall));
			double angle = 2 * M_PI / outer_wall_num;
			double cosfi = cos(angle);
			double sinfi = sin(angle);

			// generation of outer walls
			pos.x = -SCREEN_WIDTH;
			pos.y = 0;
			for (int i = 0; i < outer_wall_num; ++i)
			{
				struct Vec2D pos2;
				pos2.x = pos.x * cosfi - pos.y * sinfi;
				pos2.y = pos.x * sinfi + pos.y * cosfi;
				wall_init(&room->walls[i], pos.x, pos.y, pos2.x, pos2.y, 10);
				pos = pos2;
			}

			// generation of inner walls
			for (int i = 0; i < outer_wall_num; ++i)
			{
				struct Vec2D pos2;
				pos.x = -(rand() % (SCREEN_WIDTH / 4) + 3 * SCREEN_WIDTH / 4);
				pos.y = 0;
				pos2.x = -(rand() % (SCREEN_WIDTH / 4) + SCREEN_WIDTH / 4);
				pos2.y = 0;
				for (int j = 0; j < i; ++j)
				{
					struct Vec2D tmp;
					tmp = pos;
					pos.x = tmp.x * cosfi - tmp.y * sinfi;
					pos.y = tmp.x * sinfi + tmp.y * cosfi;
					tmp = pos2;
					pos2.x = tmp.x * cosfi - tmp.y * sinfi;
					pos2.y = tmp.x * sinfi + tmp.y * cosfi;
				}
				wall_init(&room->walls[outer_wall_num + i], pos.x, pos.y, pos2.x, pos2.y, 6);
			}

			// generation of round obstacles
			room->obstacles_num = 64;
			room->obstacles = (struct Obstacle *)malloc(room->obstacles_num * sizeof(struct Obstacle));
			for (int i = 0; i < room->obstacles_num; ++i)
			{
				if (!generate_safe_position(room, &pos,
					32.0, 100, true, true, true))
				{
					/* out of the game field
					 * we cannot break the loop because
					 * we need to fill whole the allocated memory
					 */
					pos.x = SCREEN_WIDTH * 3;
					pos.y = SCREEN_WIDTH * 3;
				}
				obstacle_init(&room->obstacles[i], pos.x, pos.y,
					rand() % (max_obstacle_size - min_obstacle_size) + min_obstacle_size);
			}
		} break;
		case LS_LARGE:
			camera_prepare(&room->snake, CM_TPP);
			break;
	}

	for (int i = 0; i < room->collectibles_num; ++i)
	{
		// needs to be done after wall init
		collectible_generate(&room->collectibles[i], room);
	}
}

void room_dispose(struct Room *room)
{
	if (room->collectibles)
	{
		free(room->collectibles);
		room->collectibles = NULL;
		room->collectibles_num = 0;
	}
	if (room->walls)
	{
		free(room->walls);
		room->walls = NULL;
		room->walls_num = 0;
	}
	if (room->obstacles)
	{
		free(room->obstacles);
		room->obstacles = NULL;
		room->obstacles_num = 0;
	}
}

void room_process(struct Room *room, double dt)
{
	snake_control(&room->snake);

	for (int i = 0; i < room->collectibles_num; ++i)
	{
		collectible_process(&room->collectibles[i], dt);
	}
	snake_process(&room->snake, dt);
	snake_eat_collectibles(&room->snake, room);
}

void room_draw(const struct Room *room)
{
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 128, 128, 128));
	for (int i = 0; i < room->collectibles_num; ++i)
	{
		collectible_draw(&room->collectibles[i]);
	}
	for (int i = 0; i < room->walls_num; ++i)
	{
		wall_draw(&room->walls[i]);
	}
	for (int i = 0; i < room->obstacles_num; ++i)
	{
		obstacle_draw(&room->obstacles[i]);
	}
	snake_draw(&room->snake);
}

bool room_check_gameover(const struct Room *room)
{
	return snake_check_selfcollision(&room->snake) ||
		snake_check_wallcollision(&room->snake, room->walls, room->walls_num) ||
		snake_check_obstaclecollision(&room->snake, room->obstacles, room->obstacles_num);
}
