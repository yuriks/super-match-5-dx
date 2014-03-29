#include "math/vec.hpp"
#include <iostream>
#include <algorithm>
#include "render/SpriteBuffer.hpp"
#include "sdl_window.hpp"
#include "util.hpp"
#include <cstdint>
#include <algorithm>
#include <array>
#include <numeric>
#include "range_macro.hpp"
#include <cassert>
#include "render/texture.hpp"
#include "gl/gl_1_5.h"
#include "math/MatrixTransform.hpp"

static const unsigned int NUM_CARD_SPRITES = 15;
static const int CARD_SIZE = 64;

static const int WINDOW_WIDTH = 360;
static const int WINDOW_HEIGHT = 480;

struct GameState {
	bool running = true;

	std::vector<int> cards;
	std::vector<float> card_anim_state;
	int playfield_width; // in cards
	int playfield_height;

	GameState(RandomGenerator& rng, int w, int h) {
		playfield_width = w;
		playfield_height = h;
		cards.reserve(w * h);

		const unsigned int num_pairs = w * h / 2;
		assert(num_pairs <= NUM_CARD_SPRITES);

		std::array<int, NUM_CARD_SPRITES> card_sprites;
		std::iota(RANGE(card_sprites), 0);
		std::shuffle(RANGE(card_sprites), rng);

		for (size_t i = 0; i < num_pairs; ++i) {
			cards.push_back(card_sprites[i]);
			cards.push_back(card_sprites[i]);
		}

		std::shuffle(RANGE(cards), rng);

		card_anim_state.resize(cards.size(), 0.0f);
	}
};


// `DrawState` conflicts with a macro in `windows.h`.
struct YksDrawState {
	yks::SpriteBufferIndices sprite_buffer_indices;
	yks::SpriteBuffer card_buffer;

	yks::TextureInfo card_texture;

	YksDrawState()
		: card_texture(yks::loadTexture("data/cards.png"))
	{
		card_buffer.texture_size = yks::mvec2(card_texture.width, card_texture.height);
	}
};

void update_game(GameState& state) {
	(void) state;
}

void draw_game(const GameState& game_state, YksDrawState& draw_state) {
	// Draw cards
	yks::Sprite card_spr;
	card_spr.img = yks::IntRect{ 0, 0, CARD_SIZE, CARD_SIZE };

	for (int y = 0; y < game_state.playfield_height; ++y) {
		for (int x = 0; x < game_state.playfield_width; ++x) {
			card_spr.pos = yks::mvec2(x * (CARD_SIZE + 8), y * (CARD_SIZE + 8));
			draw_state.card_buffer.append(card_spr);
		}
	}

	// Submit everything
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, draw_state.card_texture.handle.name);
	draw_state.card_buffer.draw(draw_state.sprite_buffer_indices);
}

void setup_intial_opengl_state() {
	YKS_CHECK_GL_PARANOID;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	yks::mat4 projection_matrix = yks::orthographic_proj(0, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), 0, -10, 10);
	glLoadTransposeMatrixf(projection_matrix.as_row_major());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

	YKS_CHECK_GL_PARANOID;
}

void game_loop(Window& window) {
	RandomGenerator rng;
	GameState game_state(rng, 4, 4);
	YksDrawState draw_state;

	int32_t frame_time_behind = 0;
	static const int32_t MAX_LAG_TIME = 100;
	const int32_t fixed_frame_time = 1000 / 60;

	setup_intial_opengl_state();

	while (game_state.running) {
		// The duration we're hoping to have on this frame
		const int32_t desired_frame_time = fixed_frame_time - frame_time_behind;

		const uint32_t frame_start = window.getTicks();

		if (!window.handleEvents()) {
			game_state.running = false;
		}

		update_game(game_state);
		draw_game(game_state, draw_state);

		window.swapBuffers();

		const int32_t time_elapsed_in_frame = window.getTicks() - frame_start;

		std::cout << "FRAMETIME: " << time_elapsed_in_frame << '\n';

		if (time_elapsed_in_frame < desired_frame_time) {
			const int32_t sleep_len = desired_frame_time - time_elapsed_in_frame;
			window.delay(sleep_len);
		}

		frame_time_behind -= fixed_frame_time;
		frame_time_behind += window.getTicks() - frame_start;
		if (frame_time_behind > MAX_LAG_TIME) {
			frame_time_behind = MAX_LAG_TIME;
		}
	}
}

int main(int , char *[]) {
	Window window(WINDOW_WIDTH, WINDOW_HEIGHT);
	if (!window.isValid()) {
		return 1;
	}

	game_loop(window);

	return 0;
}
