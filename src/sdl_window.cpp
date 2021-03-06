#include "sdl_window.hpp"
#include <SDL2/SDL.h>
#include <cstdio>
#include "gl/gl_1_5.h"

static void show_error(const char* message) {
	if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, nullptr) != 0) {
		std::fputs(message, stdout);
	}
}

struct Window::Impl {
	bool sdl_initialized = false;
	SDL_Window* window = nullptr;
	SDL_GLContext context = nullptr;
	bool gl_loaded = false;

	Impl(int window_width, int window_height) {
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			show_error("Failed to initialize SDL");
			return;
		}
		sdl_initialized = true;

		window = SDL_CreateWindow("Super Match 5 DX", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_OPENGL);
		if (window == nullptr) {
			show_error("Failed to create window.");
			return;
		}

		context = SDL_GL_CreateContext(window);
		if (context == nullptr) {
			show_error("Failed to create OpenGL context.");
		}

		if (ogl_LoadFunctions() != ogl_LOAD_SUCCEEDED) {
			show_error("Failed to load OpenGL 1.5 functions.");
			return;
		}
		gl_loaded = true;

		glViewport(0, 0, window_width, window_height);
	}

	~Impl() {
		if (context != nullptr) {
			SDL_GL_DeleteContext(context);
			context = nullptr;
		}
		if (window != nullptr) {
			SDL_DestroyWindow(window);
			window = nullptr;
		}
		if (sdl_initialized) {
			SDL_Quit();
			sdl_initialized = false;
		}
	}

	bool isValid() const {
		return gl_loaded;
	}

	void swapBuffers() {
		SDL_GL_SwapWindow(window);
	}

	bool handleEvents(WindowEventInfo& info) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return false;
			case SDL_MOUSEBUTTONDOWN:
				info.mouse_button = ev.button.button;
				info.mouse_click_x = ev.button.x;
				info.mouse_click_y = ev.button.y;
				break;
			default:
				break;
			}
		}

		return true;
	}
};

Window::Window(int width, int height)
	: impl(std::make_unique<Impl>(width, height))
{}

Window::~Window() {}

bool Window::isValid() const {
	return impl->isValid();
}

void Window::swapBuffers() {
	impl->swapBuffers();
}

uint32_t Window::getTicks() {
	return SDL_GetTicks();
}

void Window::delay(uint32_t duration) {
	SDL_Delay(duration);
}

bool Window::handleEvents(WindowEventInfo& info) {
	return impl->handleEvents(info);
}
