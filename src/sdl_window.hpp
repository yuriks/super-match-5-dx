#pragma once
#include <cstdint>
#include <tuple>
#include <memory>

struct Window {
	Window();
	~Window();
	bool isValid() const;

	void swapBuffers();

	uint32_t getTicks();
	void delay(uint32_t duration);

	bool handleEvents();

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};
