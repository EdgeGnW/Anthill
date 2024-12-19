
#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <limits.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

#define WORLD_WIDTH 640
#define WORLD_HEIGHT 400


//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
Camera camera = { 0 };
Vector3 cubePosition = { 0 };

char world_pixels[WORLD_WIDTH * WORLD_HEIGHT * 4];

int food_collected = 0;
bool show_pheromone = false;

const Vector2 ant_colony_position = {0, 0};
const int ant_colony_size = 50;
#define ANTS_AMOUNT 500
const int ant_width = 2;
const int ant_height = 2;
const float ant_speed = 2;
const float ant_rotation = 0.15;
const Color ant_color = BLUE;
const Color ant_food_color = GREEN;

int pheromone_radius = 20;
int pheromone_counter = 0;
const int pheromone_intervall = 1;
const int pheromone_size = 1;
#define PHEROMONE_LIVETIME 9999

const Vector2 food_position = {400.f, 300.f};
const int food_amount = 50;
const Color food_color = RED;
const int food_spread = 0;
const int food_size = 1;

enum Objects {
	PHEROMONE_PURPLE = PHEROMONE_LIVETIME,
	PHEROMONE_ORANGE = PHEROMONE_LIVETIME * 2,
	FOOD = PHEROMONE_LIVETIME * 2 + 1,
	WALL = PHEROMONE_LIVETIME * 2 + 2,
	COLONY = PHEROMONE_LIVETIME * 2 + 3,
};

struct Ant {
	Vector2 position;
	Vector2 direction;
	bool has_food;
};

struct Ant ants[ANTS_AMOUNT];
enum Objects world[WORLD_WIDTH][WORLD_HEIGHT];

static Texture2D screen_texture;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);          // Update and draw one frame
static void SetPixels(char*, int, int, Color);

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------


    // Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ants");

    Image screen_image = GenImageCellular(WORLD_WIDTH, WORLD_HEIGHT, 1);
	screen_texture = LoadTextureFromImage(screen_image);
	UnloadImage(screen_image);

    for (int i = 0; i < ANTS_AMOUNT; ++i) {
		ants[i].position = ant_colony_position;
		ants[i].direction = Vector2One();
		ants[i].has_food = false;
	}

	for (int i = 0; i < food_amount; ++i) {
		for (int j = 0; j < food_amount; ++j) {
			world[(int) food_position.x + i][(int) food_position.y + j] = FOOD;
		}
	}

	for (int i = 0; i < ant_colony_size; ++i) {
		for (int j = 0; j < ant_colony_size; ++j) {
			world[(int) ant_colony_position.x + i][(int) ant_colony_position.y + j] = COLONY;
		}
	}

    //--------------------------------------------------------------------------------------

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
		if (IsKeyPressed(KEY_ENTER)) show_pheromone = !show_pheromone;
        UpdateDrawFrame();
    }


    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(BLANK);

        bool pheromone_now = false;
		++pheromone_counter;
		if (pheromone_counter >= pheromone_intervall) {
			pheromone_now = true;
			pheromone_counter %= pheromone_intervall;
		}
		

		for (int i = 0; i < WORLD_WIDTH; ++i) {
			for (int j = 0; j < WORLD_HEIGHT; ++j) {
				if (!world[i][j]) {
					SetPixels(world_pixels, i, j, BLANK);
					continue;
				}
				Color color;
				int size;
				enum Objects *world_ptr = &world[i][j];
				switch (*world_ptr) {
					case FOOD: color = food_color; break;
					case COLONY: color = LIME; break;
					case 1 ... PHEROMONE_PURPLE: 
						color = show_pheromone ? ColorBrightness(PURPLE, ((float)(*world_ptr % PHEROMONE_LIVETIME)) / PHEROMONE_LIVETIME - 1) : BLACK;
						size = pheromone_size;
						if (pheromone_now) --*world_ptr;
						break;
					case PHEROMONE_PURPLE + 1 ... PHEROMONE_ORANGE: 
						color = show_pheromone ? ColorBrightness(ORANGE, ((float)(*world_ptr % PHEROMONE_LIVETIME)) / PHEROMONE_LIVETIME - 1) : BLACK;
						size = pheromone_size;
						if (pheromone_now && --*world_ptr <= PHEROMONE_PURPLE) {
							*world_ptr = 0;
						}
						break;
					default: color = GRAY;
				} 
				SetPixels(world_pixels, i, j, color);
			}
		}

		for (int i = 0; i < ANTS_AMOUNT; ++i) {
            struct Ant *ant_ptr = &ants[i];
			enum Objects *world_ptr = &world[(int) ant_ptr->position.x][(int) ant_ptr->position.y];
			if (pheromone_now && *world_ptr <= PHEROMONE_ORANGE) {
				int current_pheromone = ant_ptr->has_food ? PHEROMONE_ORANGE : PHEROMONE_PURPLE;
				int a = (*world_ptr <= current_pheromone && *world_ptr > current_pheromone - PHEROMONE_LIVETIME) ? *world_ptr : current_pheromone - PHEROMONE_LIVETIME;
				*world_ptr = a + PHEROMONE_LIVETIME / 20;
				if (*world_ptr > current_pheromone) *world_ptr = current_pheromone;
			}

			if (GetRandomValue(0, 1)) {
				ant_ptr->direction = Vector2Normalize(Vector2Rotate(ant_ptr->direction, GetRandomValue(-1, 1) * ant_rotation));
			}

            int half_radius = pheromone_radius / 2;

			bool grab_food = false;
            Vector2 pheromone_vector = Vector2Zero();
            for (int x = (int)ant_ptr->position.x - half_radius; x < (int)ant_ptr->position.x + half_radius && !grab_food; ++x) {
                if (x < 0 || x >= WORLD_WIDTH) continue;
                for (int y = (int)ant_ptr->position.y - half_radius; y < (int)ant_ptr->position.y + half_radius; ++y) {
					if ((world[x][y] == FOOD && !ant_ptr->has_food || world[x][y] == COLONY && ant_ptr->has_food) && Vector2LengthSqr((Vector2) {x, y}) <= 3) {
						grab_food = true;
						ant_ptr->position = (Vector2) {x, y};
						*world_ptr = world[x][y];
						break;
					}
                    if (y < 0 || y >= WORLD_HEIGHT || (x == ant_ptr->position.x && y == ant_ptr->position.y) || !world[x][y]) continue;
                    if ((((world[x][y] >PHEROMONE_PURPLE && world[x][y] <= PHEROMONE_ORANGE) || world[x][y] == FOOD) && !ant_ptr->has_food)
                    || ((world[x][y] <= PHEROMONE_PURPLE || world[x][y] == COLONY) && ant_ptr->has_food)) {
                        Vector2 dist_vec = Vector2Subtract((Vector2) {.x = x, .y = y}, ant_ptr->position);
						//(if (Vector2LengthSqr(dist_vec) > pheromone_radius) continue;
                        if (Vector2DotProduct(ant_ptr->direction, dist_vec) < -0.5) continue; 
						int factor = world[x][y] < FOOD ? world[x][y] % PHEROMONE_LIVETIME : PHEROMONE_LIVETIME * pheromone_radius;
                        pheromone_vector = Vector2Add(pheromone_vector, Vector2Scale(Vector2Normalize(dist_vec), factor));
                    }
                }
            }

            if (Vector2LengthSqr(pheromone_vector) > pheromone_radius * 0.5) {
                ant_ptr->direction = Vector2Normalize(Vector2Add(ant_ptr->direction, Vector2Normalize(pheromone_vector)));
            }
            

			if (*world_ptr == FOOD && !ant_ptr->has_food) {
				ant_ptr->has_food = true;
                ant_ptr->direction = Vector2Negate(ant_ptr->direction);
				*world_ptr = 0;
			} else if (*world_ptr == COLONY && ant_ptr->has_food) {
				ant_ptr->has_food = false;
				++food_collected;
                ant_ptr->direction = Vector2Negate(ant_ptr->direction);
			}
			ant_ptr->position = Vector2Clamp(Vector2Add(ant_ptr->position, Vector2Scale(ant_ptr->direction, ant_speed)),
							Vector2Zero(),
							(Vector2) {WORLD_WIDTH - 1, WORLD_HEIGHT - 1});

            if (ant_ptr->position.x <= 0 || ant_ptr->position.x >= WORLD_WIDTH - 1) ant_ptr->direction.x *= -1;
            if (ant_ptr->position.y <= 0 || ant_ptr->position.y >= WORLD_HEIGHT - 1) ant_ptr->direction.y *= -1;

			SetPixels(world_pixels, ant_ptr->position.x, ant_ptr->position.y, ant_ptr->has_food ? ant_food_color : ant_color);
		}

		UpdateTexture(screen_texture, world_pixels);
		DrawTextureEx(screen_texture, Vector2Zero(), 0, 2, WHITE);

		DrawText(TextFormat("Food Collected: %i", food_collected), 20 , 20, 20, RAYWHITE);

		DrawFPS(SCREEN_WIDTH - 200, 20);

    EndDrawing();
    //----------------------------------------------------------------------------------
}

static void SetPixels(char *pixels, int x, int y, Color color) {
	int index = (x + y * WORLD_WIDTH) * 4;
	pixels[index] = color.r;
	pixels[index + 1] = color.g;
	pixels[index + 2] = color.b;
	pixels[index + 3] = color.a;
}
