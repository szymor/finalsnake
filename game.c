#include <SDL_gfxPrimitives.h>
#include "main.h"
#include "game.h"
#include "gfx.h"

int fps = 0;

struct Camera camera = {
	.cm = CM_FIXED,
	.center = NULL,
	.angle = NULL,
	.target_angle = NULL
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
	snake->pieces[0] = (struct Vec2D) {
			.x = SCREEN_WIDTH/2,
			.y = SCREEN_HEIGHT/2
		};
	snake->turn = TURN_NONE;
	snake->wobbly_freq = 0;
	snake->wobbly_phase = 0;
	switch (menu_options[MO_WOBBLINESS])
	{
		case W_NORMIE:
			snake->wobbly_freq = 2.4;
			break;
		case W_BOOZER:
			snake->wobbly_freq = 0.8;
			break;
	}
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

	// wobbliness calculation
	snake->wobbly_phase += dt;
	SINCOS_FIX_INC(snake->wobbly_phase);
	double wobbly = 0.5 * sin(2 * M_PI * snake->wobbly_freq * snake->wobbly_phase);

	// head calculation
	struct Vec2D offset = {
		.x = snake->v * sin(snake->dir + wobbly) * dt,
		.y = -snake->v * cos(snake->dir + wobbly) * dt
	};
	vadd(&snake->pieces[0], &offset);

	// tail calculation
	for (int i = 1; i < snake->len; ++i)
	{
		struct Vec2D diff;
		diff = snake->pieces[i-1];
		vsub(&diff, &snake->pieces[i]);
		double dlen = vlen(&diff);
		if (dlen > PIECE_DISTANCE)
		{
			vmul(&diff, (dlen - PIECE_DISTANCE) / dlen);
			vadd(&snake->pieces[i], &diff);
		}
	}
}

void snake_draw(const struct Snake *snake)
{
	Uint32 black = SDLGFX_COLOR(0, 0, 0);

	// body
	for (int i = snake->len - 1; i > 0; i -= PIECE_DRAW_INCREMENT)
	{
		double x = snake->pieces[i].x;
		double y = snake->pieces[i].y;
		camera_convert(&x, &y);
		filledCircleColor(screen, x, y, BODY_RADIUS, black);
		circleRGBA(screen, x, y, BODY_RADIUS, 64, 64, 64, 255);
	}

	// head
	double x = snake->pieces[0].x;
	double y = snake->pieces[0].y;
	camera_convert(&x, &y);
	filledCircleColor(screen, x, y, HEAD_RADIUS, black);
	circleRGBA(screen, x, y, HEAD_RADIUS, 64, 64, 64, 255);
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
	if (snake->len > MAX_SNAKE_LEN)
	{
		snake->len = MAX_SNAKE_LEN;
	}
	for (int i = start; i < snake->len; ++i)
	{
		snake->pieces[i] = snake->pieces[start - 1];
	}
}

void snake_eat_consumables(struct Snake *snake, struct Room *room)
{
	for (int i = 0; i < room->consumables_num; ++i)
	{
		struct Vec2D diff = snake->pieces[0];
		vsub(&diff, &room->consumables[i].segment.pos);
		if (vlen(&diff) < (HEAD_RADIUS + room->consumables[i].segment.r - EAT_DEPTH))
		{
			snake_add_segments(snake, PIECE_DRAW_INCREMENT);
			consumable_generate(&room->consumables[i], room);
		}
	}
}

bool snake_check_selfcollision(const struct Snake *snake)
{
	for (int i = START_LEN + 1; i < snake->len; ++i)
	{
		struct Vec2D diff = snake->pieces[0];
		vsub(&diff, &snake->pieces[i]);
		if (vlen(&diff) < (HEAD_RADIUS + BODY_RADIUS))
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
		struct Vec2D *vector_distance = wall_dist(&walls[i], &snake->pieces[0]);
		if (vlen(vector_distance) < (HEAD_RADIUS + walls[i].r))
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
		struct Vec2D diff = snake->pieces[0];
		vsub(&diff, &obs[i].segment.pos);
		if (vlen(&diff) < (HEAD_RADIUS + obs[i].segment.r))
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
	int x_from = room->cg_cartesian.upper_left.x;
	int x_to = room->cg_cartesian.bottom_right.x;
	int y_from = room->cg_cartesian.upper_left.y;
	int y_to = room->cg_cartesian.bottom_right.y;
	int attempts = 0;
	bool safe;
	do {
		safe = true;
		switch(room->cg_mode)
		{
			case CGM_CARTESIAN:
				pos->x = rand() % (x_to - x_from) + x_from;
				pos->y = rand() % (y_to - y_from) + y_from;
				pos->x = round(pos->x);
				pos->y = round(pos->y);
				break;
			case CGM_POLAR:
				double r = rand() % (int) room->cg_polar.radius;
				double fi = 2 * M_PI * (rand() % 1024) / 1024;
				pos->x = r * cos(fi);
				pos->y = r * sin(fi);
				pos->x = round(pos->x);
				pos->y = round(pos->y);
				break;
		}
		if (snake)
		{
			if (vdist(pos, &room->snake.pieces[0]) < safe_distance)
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

void consumable_generate(struct Consumable *col, const struct Room *room)
{
	const int safe_distance = 15;

	col->segment = (struct Segment)
		{ .pos = { .x = 0, .y = 0 },
			.r = CONSUMABLE_RADIUS,
		};
	col->color = SDLGFX_COLOR(rand() % 255, rand() % 255, rand() % 255);

	if (!generate_safe_position(room, &col->segment.pos,
		safe_distance, 100, true, true, true))
	{
		// spawn it on top of the snake :)
		col->segment.pos = room->snake.pieces[0];
	}
}

void consumable_process(struct Consumable *col, double dt)
{
	// placeholder for moving consumables
}

void consumable_draw(const struct Consumable *col)
{
	double x = col->segment.pos.x;
	double y = col->segment.pos.y;
	camera_convert(&x, &y);
	filledCircleColor(screen, x, y, col->segment.r, col->color);
	circleRGBA(screen, x, y, col->segment.r, 0, 0, 0, 255);
}

void camera_prepare(const struct Snake *target, enum CameraMode cm)
{
	camera.cm = cm;
	camera.center = &target->pieces[0];
	if (CM_TPP == cm)
	{
		camera.angle = &target->dir;
	}
	else if (CM_TPP_DELAYED == cm)
	{
		camera.angle_store = target->dir;
		camera.angle = &camera.angle_store;
		camera.target_angle = &target->dir;
	}
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
		case CM_TPP_DELAYED:
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

void camera_process(double dt)
{
	if (CM_TPP_DELAYED == camera.cm)
	{
		double diff = *camera.target_angle - camera.angle_store;
		SINCOS_FIX_INC(diff);
		SINCOS_FIX_DEC(diff);
		camera.angle_store += 0.6 * diff * dt;
		SINCOS_FIX_INC(camera.angle_store);
		SINCOS_FIX_DEC(camera.angle_store);
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

void pause_draw(bool paused)
{
	if (paused)
	{
		char string[8] = "PAUSE";
		int x = (SCREEN_WIDTH - 8 * strlen(string)) / 2;
		int y = (SCREEN_HEIGHT - 8) / 2;
		stringRGBA(screen, x, y, string, 0, 0, 0, 255);
		stringRGBA(screen, x - 1, y - 1, string, 255, 255, 255, 255);
	}
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
	wall->start.x = round(x1);
	wall->start.y = round(y1);
	wall->end.x = round(x2);
	wall->end.y = round(y2);
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

	struct Vec2D voff = { .x = x2 - x1, .y = y2 - y1 };
	vmul(&voff, wall->r / vlen(&voff));
	voff = (struct Vec2D){ .x = -voff.y, .y = voff.x };
	Sint16 vx[4] = {x1 + voff.x, x2 + voff.x, x2 - voff.x, x1 - voff.x};
	Sint16 vy[4] = {y1 + voff.y, y2 + voff.y, y2 - voff.y, y1 - voff.y};
	filledPolygonRGBA(screen, vx, vy, 4, 0, 0, 0, 255);
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

void obstacle_init(struct Obstacle *obstacle, double x, double y, double r)
{
	obstacle->segment.pos = (struct Vec2D){ .x = round(x), .y = round(y) };
	obstacle->segment.r = r;
}

void obstacle_draw(const struct Obstacle *obstacle)
{
	double x = obstacle->segment.pos.x;
	double y = obstacle->segment.pos.y;
	camera_convert(&x, &y);
	filledCircleRGBA(screen, x, y, obstacle->segment.r, 0, 0, 0, 255);
}

void room_init(struct Room *room)
{
	struct Vec2D pos;

	room->consumables_num = 0;
	room->consumables = NULL;
	room->walls_num = 0;
	room->walls = NULL;
	room->obstacles_num = 0;
	room->obstacles = NULL;

	switch (menu_options[MO_LEVELTYPE])
	{
		case LT_CAGE:
		{
			const int min_obstacle_size = 12;
			const int max_obstacle_size = 24;
			room->consumables_num = 3;
			room->consumables = (struct Consumable *)malloc(room->consumables_num * sizeof(struct Consumable));
			room->cg_mode = CGM_CARTESIAN;
			room->cg_cartesian.upper_left = (struct Vec2D){ .x = 0, .y = 0};
			room->cg_cartesian.bottom_right = (struct Vec2D){ .x = SCREEN_WIDTH, .y = SCREEN_HEIGHT};
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
		case LT_POLYGON:
		{
			const int outer_wall_num = rand() % 6 + 3;
			const int circumradius = SCREEN_HEIGHT;
			room->consumables_num = 4;
			room->consumables = (struct Consumable *)malloc(room->consumables_num * sizeof(struct Consumable));
			room->cg_mode = CGM_POLAR;
			room->cg_polar.radius = circumradius * cos(M_PI / outer_wall_num);
			room->walls_num = outer_wall_num;
			room->walls = (struct Wall *)malloc(room->walls_num * sizeof(struct Wall));
			room->obstacles_num = outer_wall_num + 1;
			room->obstacles = (struct Obstacle *)malloc(room->obstacles_num * sizeof(struct Obstacle));
			obstacle_init(&room->obstacles[0], 0, 0, room->cg_polar.radius / 2);
			const double angle = 2 * M_PI / outer_wall_num;
			const double cosfi = cos(angle);
			const double sinfi = sin(angle);

			// generation of outer walls and obstacles
			// radius of the circumscribed circle of the polygon
			pos.x = circumradius;
			pos.y = 0;
			struct Vec2D obstacle_pos;
			obstacle_pos.x = (circumradius - room->obstacles[0].segment.r) / 2 + room->obstacles[0].segment.r;
			obstacle_pos.y = 0;
			for (int i = 0; i < outer_wall_num; ++i)
			{
				// calculation of the endpoint of the wall
				struct Vec2D pos2;
				pos2.x = pos.x * cosfi - pos.y * sinfi;
				pos2.y = pos.x * sinfi + pos.y * cosfi;
				wall_init(&room->walls[i], pos.x, pos.y, pos2.x, pos2.y, 10);
				// calculation of the obstacle
				obstacle_init(&room->obstacles[i + 1], obstacle_pos.x, obstacle_pos.y, 17);
				pos.x = obstacle_pos.x * cosfi - obstacle_pos.y * sinfi;
				pos.y = obstacle_pos.x * sinfi + obstacle_pos.y * cosfi;
				obstacle_pos = pos;
				// endpoint assignment for the next iteration
				pos = pos2;
			}

			// snake initialization
			snake_init(&room->snake);
			room->snake.pieces[0] = (struct Vec2D){ .x = 0, .y = -room->cg_polar.radius / 2 - HEAD_RADIUS - 1 };
			snake_add_segments(&room->snake, START_LEN - 1);
			camera_prepare(&room->snake, CM_TRACKING);
		} break;
		case LT_STAR:
		{
			const int wall_thickness = 10;
			room->consumables_num = 5;
			room->consumables = (struct Consumable *)malloc(room->consumables_num * sizeof(struct Consumable));
			int points_no = rand() % 5 + 5;
			room->walls_num = points_no * 3;
			room->walls = (struct Wall *)malloc(room->walls_num * sizeof(struct Wall));
			// radius of the circumscribed circle of the star
			const double radius = SCREEN_WIDTH;
			const double alpha = M_PI * (points_no - 2) / points_no;
			const double fi = M_PI - alpha;
			const double cosfi = cos(fi);
			const double sinfi = sin(fi);
			const double cos2fi = cos(2 * fi);
			const double sin2fi = sin(2 * fi);
			pos.x = 0;
			pos.y = -radius;
			struct Vec2D star_side;
			star_side.x = radius * sin(M_PI / points_no) / cos(fi / 2);
			star_side.y = 0;
			// rotate the star side vector to maintain a nice origin
			struct Vec2D tmp = star_side;
			star_side.x = tmp.x * cos(alpha) - tmp.y * sin(alpha);
			star_side.y = tmp.x * sin(alpha) + tmp.y * cos(alpha);
			double inner_coef = 0;
			for (int i = 0; i < points_no; ++i)
			{
				// draw a side
				struct Vec2D pos2 = pos;
				vadd(&pos2, &star_side);
				//--- quick'n'dirty way to get inradius of the star
				if (0 == i)
				{
					const double inradius = vlen(&pos2);
					room->cg_mode = CGM_POLAR;
					room->cg_polar.radius = inradius;
					inner_coef = inradius / radius;
				}
				//--- here it ends
				wall_init(&room->walls[i * 3 + 1], pos.x, pos.y, pos2.x, pos2.y, wall_thickness);
				// draw an inner wall
				wall_init(&room->walls[i * 3], pos.x * inner_coef, pos.y * inner_coef, 0, 0, 6);
				// turn counterclockwise
				tmp = star_side;
				star_side.x = tmp.x * cosfi - tmp.y * sinfi;
				star_side.y = tmp.x * sinfi + tmp.y * cosfi;
				// draw another side
				pos = pos2;
				vadd(&pos2, &star_side);
				wall_init(&room->walls[i * 3 + 2], pos.x, pos.y, pos2.x, pos2.y, wall_thickness);
				// turn clockwise twice as much
				tmp = star_side;
				star_side.x = tmp.x * cos2fi + tmp.y * sin2fi;
				star_side.y = -tmp.x * sin2fi + tmp.y * cos2fi;
				pos = pos2;
			}

			// snake initialization
			snake_init(&room->snake);
			room->snake.pieces[0] = (struct Vec2D){ .x = 15, .y = -radius / 3 };
			snake_add_segments(&room->snake, START_LEN - 1);
			camera_prepare(&room->snake, CM_TPP_DELAYED);
		} break;
	}

	for (int i = 0; i < room->consumables_num; ++i)
	{
		// needs to be done after wall init
		consumable_generate(&room->consumables[i], room);
	}

	tiles_recolor(rand() % 100);
}

void room_dispose(struct Room *room)
{
	if (room->consumables)
	{
		free(room->consumables);
		room->consumables = NULL;
		room->consumables_num = 0;
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

	for (int i = 0; i < room->consumables_num; ++i)
	{
		consumable_process(&room->consumables[i], dt);
	}
	snake_process(&room->snake, dt);
	snake_eat_consumables(&room->snake, room);
}

void room_draw(const struct Room *room)
{
	// draw background
#ifdef CHECKERBOARD_OFF
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 128, 128, 128));
#else
	int ox = 0;
	int oy = 0;
	switch (camera.cm)
	{
		case CM_TRACKING:
			ox = (int)(camera.center->x - SCREEN_WIDTH / 2) % (CHECKERBOARD_SIZE * 2);
			if (ox < 0) ox = (CHECKERBOARD_SIZE * 2) + ox;
			oy = (int)(camera.center->y - SCREEN_HEIGHT / 2) % (CHECKERBOARD_SIZE * 2);
			if (oy < 0) oy = (CHECKERBOARD_SIZE * 2) + oy;
		case CM_FIXED:
			const int bpp = screen->format->BytesPerPixel;
			SDL_LockSurface(screen);
			for (int y = 0; y < SCREEN_HEIGHT; ++y)
			{
				const int fox = ox;
				for (int x = 0; x < SCREEN_WIDTH; ++x)
				{
					int tf = (ox / CHECKERBOARD_SIZE) ^ (oy / CHECKERBOARD_SIZE) ? CHECKERBOARD_SIZE : 0;
					int tx = (ox % CHECKERBOARD_SIZE) + tf;
					int ty = oy % CHECKERBOARD_SIZE;
					Uint8 *p = (Uint8 *)screen->pixels + y * screen->pitch + x * bpp;
					const int tbpp = tiles->format->BytesPerPixel;
					const Uint16 tp = *(Uint16 *)((Uint8 *)tiles->pixels + ty * tiles->pitch + tx * tbpp);
					*(Uint16 *)p = tp;
					++ox;
					ox %= 2 * CHECKERBOARD_SIZE;
				}
				++oy;
				oy %= 2 * CHECKERBOARD_SIZE;
				ox = fox;
			}
			SDL_UnlockSurface(screen);
			break;
		case CM_TPP:
		case CM_TPP_DELAYED:
		{
			SDL_LockSurface(screen);
			const int bpp = screen->format->BytesPerPixel;
			const double sinfi = sin(*camera.angle);
			const double cosfi = cos(*camera.angle);
			const int delta_x = cosfi * 65536;
			const int delta_y = sinfi * 65536;
			const double bx = -SCREEN_WIDTH / 2;
			const double by = -SCREEN_HEIGHT / 2;
			const double ax = bx * cosfi - by * sinfi;
			const double ay = bx * sinfi + by * cosfi;
			// fixed-point representation
			int xx = (ax + camera.center->x) * 65536;
			int yy = (ay + camera.center->y) * 65536;
			for (int y = 0; y < SCREEN_HEIGHT; ++y)
			{
				int fx = xx;
				int fy = yy;
				for (int x = 0; x < SCREEN_WIDTH; ++x)
				{
					int ix = xx % ((2 * CHECKERBOARD_SIZE) << 16);
					if (ix < 0) ix += ((2 * CHECKERBOARD_SIZE) << 16);
					int iy = yy % ((2 * CHECKERBOARD_SIZE) << 16);
					if (iy < 0) iy += ((2 * CHECKERBOARD_SIZE) << 16);
					ix >>= 16;	// div by 65536
					iy >>= 16;
					int modx = ix % CHECKERBOARD_SIZE;
					int mody = iy % CHECKERBOARD_SIZE;
					if (((ix / CHECKERBOARD_SIZE) ^ (iy / CHECKERBOARD_SIZE)) & 1)
						modx += CHECKERBOARD_SIZE;
					Uint8 *p = (Uint8 *)screen->pixels + y * screen->pitch + x * bpp;
					const int tbpp = tiles->format->BytesPerPixel;
					const Uint16 tp = *(Uint16 *)((Uint8 *)tiles->pixels + mody * tiles->pitch + modx * tbpp);
					*(Uint16 *)p = tp;
					xx += delta_x;
					yy += delta_y;
				}
				xx = fx - delta_y;
				yy = fy + delta_x;
			}
			SDL_UnlockSurface(screen);
		} break;
	}
#endif
	for (int i = 0; i < room->consumables_num; ++i)
	{
		consumable_draw(&room->consumables[i]);
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
