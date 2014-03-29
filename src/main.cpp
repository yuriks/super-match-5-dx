#include "math/vec.hpp"
#include <iostream>
#include <algorithm>
#include <SDL2/SDL.h>

static void show_error(const char* message) {
	if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, nullptr) != 0) {
		std::cerr << message << std::endl;
	}
}

void game_loop(SDL_Window* window) {
	bool running = true;

	Sint32 frame_time_behind = 0;
	static const Sint32 MAX_LAG_TIME = 100;

	const Sint32 fixed_frame_time = 1000 / 60;

	while (running) {
		// The duration we're hoping to have on this frame
		const Sint32 desired_frame_time = fixed_frame_time - frame_time_behind;

		const Uint32 frame_start = SDL_GetTicks();

		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				running = false;
				break;
			default:
				break;
			}
		}

		// do update here

		SDL_GL_SwapWindow(window);

		const Sint32 time_elapsed_in_frame = SDL_GetTicks() - frame_start;

		std::cout << "FRAMETIME: " << time_elapsed_in_frame << '\n';

		if (time_elapsed_in_frame < desired_frame_time) {
			const Sint32 sleep_len = desired_frame_time - time_elapsed_in_frame;
			SDL_Delay(sleep_len);
		}

		frame_time_behind -= fixed_frame_time;
		frame_time_behind += SDL_GetTicks() - frame_start;
		if (frame_time_behind > MAX_LAG_TIME) {
			frame_time_behind = MAX_LAG_TIME;
		}
	}
}

int main(int , char *[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		show_error("Failed to initialize SDL");
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("Super Match 5 DX", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);
	if (window == nullptr) {
		show_error("Failed to create window.");
		return 1;
	}

	game_loop(window);

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
