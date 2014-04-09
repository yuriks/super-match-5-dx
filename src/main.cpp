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
#include "math/misc.hpp"
#include "srgb.hpp"

static const unsigned int NUM_CARD_SPRITES = 13;
static const int CARD_SIZE = 64;

static const int WINDOW_WIDTH = 360;
static const int WINDOW_HEIGHT = 480;

enum class CardState {
	HIDDEN,
	SELECTED,
	MATCHED
};

struct GameState {
	bool running = true;

	std::vector<int> cards;
	std::vector<CardState> card_states;
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

		card_states.resize(cards.size(), CardState::HIDDEN);
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

yks::IntRect getRectForCard(const GameState& state, const int card_i) {
	const int card_x = card_i % state.playfield_width;
	const int card_y = card_i / state.playfield_width;
	return yks::IntRect{ card_x * (CARD_SIZE + 8), card_y * (CARD_SIZE + 8), CARD_SIZE, CARD_SIZE };
}

bool isPointInRect(const int x, const int y, const yks::IntRect& rect) {
	return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

void update_game(GameState& state, WindowEventInfo& event_info) {
	for (size_t i = 0; i < state.cards.size(); ++i) {
		const yks::IntRect card_rect = getRectForCard(state, i);
		if (event_info.mouse_button == 1 && isPointInRect(event_info.mouse_click_x, event_info.mouse_click_y, card_rect)) {
			state.card_states[i] = CardState::SELECTED;
		}

		const float anim_target = state.card_states[i] == CardState::HIDDEN ? 0.0f : 1.0f;
		state.card_anim_state[i] = stepTowards(state.card_anim_state[i], anim_target, 0.02f);
		//state.card_anim_state[i] += 0.002f * i;
	}
}

void draw_game(const GameState& game_state, YksDrawState& draw_state) {
	// Draw cards
	yks::Sprite card_spr;

	for (int y = 0; y < game_state.playfield_height; ++y) {
		for (int x = 0; x < game_state.playfield_width; ++x) {
			const size_t card_index = y * game_state.playfield_width + x;
			const float card_hscale = std::cos(game_state.card_anim_state[card_index] * yks::pi);
			const yks::vec2 half_card = yks::mvec2(0.5f * CARD_SIZE, 0.5f * CARD_SIZE);

			uint8_t col = yks::byte_from_linear(std::abs(card_hscale));
			card_spr.color = yks::Color{ col, col, col, 255 };
			if (card_hscale < 0.0f) {
				const int tile_i = game_state.cards[card_index] + 1;
				const int tile_x = tile_i % 4;
				const int tile_y = tile_i / 4;
				card_spr.img = yks::IntRect{ tile_x * CARD_SIZE, tile_y * CARD_SIZE, CARD_SIZE, CARD_SIZE };
			} else {
				card_spr.img = yks::IntRect{ 0, 0, CARD_SIZE, CARD_SIZE };
			}
			card_spr.mat.identity()
				.translate(-half_card)
				.scale(yks::mvec2(std::abs(card_hscale), 1.0f))
				.translate(half_card + yks::mvec2(x * (CARD_SIZE + 8), y * (CARD_SIZE + 8)).typecast<float>());
			draw_state.card_buffer.append(card_spr);
		}
	}

	// Submit everything
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, draw_state.card_texture.handle.name);
	draw_state.card_buffer.draw(draw_state.sprite_buffer_indices);
	draw_state.card_buffer.clear();

	YKS_CHECK_GL_PARANOID;
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

static std::random_device::result_type get_seed() {
	std::random_device rd;
	return rd();
}

void game_loop(Window& window) {
	RandomGenerator rng(get_seed());
	GameState game_state(rng, 4, 4);
	YksDrawState draw_state;

	float frame_time_behind = 0.0f;
	static const float MAX_LAG_TIME = 100;
	const float fixed_frame_time = 1000.0f / 60;

	std::array<int32_t, 60> frametimes;
	unsigned int frametimes_pos = 0;
	frametimes.fill(static_cast<int32_t>(fixed_frame_time));

	setup_intial_opengl_state();

	while (game_state.running) {
		const uint32_t frame_start = window.getTicks();

		frame_time_behind += fixed_frame_time;

		// The duration we're hoping to have on this frame
		const int32_t desired_frame_time = static_cast<int32_t>(frame_time_behind);

		WindowEventInfo event_info;
		if (!window.handleEvents(event_info)) {
			game_state.running = false;
		}

		update_game(game_state, event_info);
		draw_game(game_state, draw_state);

		window.swapBuffers();

		const double frametime_avg = std::accumulate(frametimes.cbegin(), frametimes.cend(), 0.0) / frametimes.size();
		std::cout << "FRAMETIME: " << frametime_avg << '\n';

		const int32_t time_elapsed_in_frame = window.getTicks() - frame_start;

		if (time_elapsed_in_frame < desired_frame_time) {
			const int32_t sleep_len = desired_frame_time - time_elapsed_in_frame;
			window.delay(sleep_len);
		}

		const int32_t final_frame_len = window.getTicks() - frame_start;

		frametimes[frametimes_pos] = final_frame_len;
		if (++frametimes_pos >= frametimes.size()) frametimes_pos = 0;

		frame_time_behind -= final_frame_len;
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
