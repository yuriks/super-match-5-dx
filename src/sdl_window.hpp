#pragma once
#include <cstdint>
#include <tuple>
#include <memory>

struct WindowEventInfo {
	int mouse_button = 0;
	int mouse_click_x = -1;
	int mouse_click_y = -1;
};

struct Window {
	Window(int width, int height);
	~Window();
	bool isValid() const;

	void swapBuffers();

	uint32_t getTicks();
	void delay(uint32_t duration);

	bool handleEvents(WindowEventInfo& info);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};
