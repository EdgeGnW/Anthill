/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

For a C++ project simply rename the file to .cpp and re-run the build script 

-- Copyright (c) 2020-2024 Jeffery Myers
--
--This software is provided "as-is", without any express or implied warranty. In no event 
--will the authors be held liable for any damages arising from the use of this software.

--Permission is granted to anyone to use this software for any purpose, including commercial 
--applications, and to alter it and redistribute it freely, subject to the following restrictions:

--  1. The origin of this software must not be misrepresented; you must not claim that you 
--  wrote the original software. If you use this software in a product, an acknowledgment 
--  in the product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented
--  as being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.

*/

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <limits.h>


#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

#define WORLD_WIDTH 640
#define WORLD_HEIGHT 400



static void set_pixels(char *pixels, int x, int y, Color color) {
	int index = (x + y * WORLD_WIDTH) * 4;
	pixels[index] = color.r;
	pixels[index + 1] = color.g;
	pixels[index + 2] = color.b;
	pixels[index + 3] = color.a;
}

int main ()
{

	char world_pixels[WORLD_WIDTH * WORLD_HEIGHT * 4];

	Vector2 ant_colony_position = {100, 100};
	int ant_colony_size = 20;
	int ants_amount = 50;
	int ant_width = 2;
	int ant_height = 2;
	float ant_speed = 2;
	float ant_rotation = 0.3;
	Color ant_color = BLUE;
	Color ant_food_color = GREEN;

	int pheromone_counter = 0;
	int pheromone_intervall = 1;
	int pheromone_size = 1;
	#define PHEROMONE_LIFETIME 1000000

	Vector2 food_position = {400.f, 300.f};
	int food_amount = 50;
	Color food_color = RED;
	int food_spread = 0;
	int food_size = 1;

	enum Objects {
		PHEROMONE_PURPLE = PHEROMONE_LIFETIME,
		PHEROMONE_ORANGE = PHEROMONE_LIFETIME * 2,
		FOOD = PHEROMONE_LIFETIME * 2 + 1,
		WALL = PHEROMONE_LIFETIME * 2 + 2,
		COLONY = PHEROMONE_LIFETIME * 2 + 3,
	};

	struct Ant {
		Vector2 position;
		Vector2 direction;
		bool has_food;
	};

	struct Ant ants[ants_amount];

	for (int i = 0; i < ants_amount; ++i) {
		ants[i].position = ant_colony_position;
		ants[i].direction = Vector2One();
		ants[i].has_food = false;
	}

	enum Objects world[WORLD_WIDTH][WORLD_HEIGHT];
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

	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	// Create the window and OpenGL context
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ants");

	Image screen_image = GenImageCellular(WORLD_WIDTH, WORLD_HEIGHT, 1);
	Texture2D screen_texture = LoadTextureFromImage(screen_image);
	UnloadImage(screen_image);
	
	// game loop
	while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

		bool pheromone_now = false;
		++pheromone_counter;
		if (pheromone_counter >= pheromone_intervall) {
			pheromone_now = true;
			pheromone_counter %= pheromone_intervall;
		}
		

		for (int i = 0; i < WORLD_WIDTH; ++i) {
			for (int j = 0; j < WORLD_HEIGHT; ++j) {
				if (!world[i][j]) {
					set_pixels(world_pixels, i, j, BLACK);
					continue;
				}
				Color color;
				int size;
				enum Objects *world_ptr = &world[i][j];
				switch (*world_ptr) {
					case FOOD: color = food_color; break;
					case COLONY: color = LIME; break;
					case 1 ... PHEROMONE_PURPLE: 
						color = PURPLE;
						size = pheromone_size;
						if (pheromone_now) *world_ptr -= 1;
						break;
					case PHEROMONE_PURPLE + 1 ... PHEROMONE_ORANGE: 
						color = ORANGE;
						size = pheromone_size;
						if (pheromone_now) {
							*world_ptr = *world_ptr <= PHEROMONE_PURPLE + 1 ? 0 : *world_ptr - 1;
						}
						break;
					default: color = GRAY;
				} 
				set_pixels(world_pixels, i, j, color);
			}
		}

		for (int i = 0; i < ants_amount; ++i) {
			enum Objects *world_ptr = &world[(int) ants[i].position.x][(int) ants[i].position.y];
			if (pheromone_now && *world_ptr <= PHEROMONE_ORANGE) {
				*world_ptr = ants[i].has_food ? PHEROMONE_ORANGE : PHEROMONE_PURPLE;
			}
			if (GetRandomValue(0, 1)) {
				ants[i].direction = Vector2Normalize(Vector2Rotate(ants[i].direction, GetRandomValue(-1, 1) * ant_rotation));
			}
			if (*world_ptr == FOOD && !ants[i].has_food) {
				ants[i].has_food = true;
				*world_ptr = 0;
			}
			ants[i].position = Vector2Clamp(Vector2Add(ants[i].position, Vector2Scale(ants[i].direction, ant_speed)),
							Vector2Zero(),
							(Vector2) {WORLD_WIDTH - 1, WORLD_HEIGHT - 1});
			set_pixels(world_pixels, ants[i].position.x, ants[i].position.y, ants[i].has_food ? ant_food_color : ant_color);
		}

		UpdateTexture(screen_texture, world_pixels);
		DrawTextureEx(screen_texture, Vector2Zero(), 0, 2, WHITE);

		// draw some text using the default font
		DrawText("Fucking Ants", 20,20,20,WHITE);

		DrawFPS(SCREEN_WIDTH - 200, 20);  
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}


	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
