#include <stdbool.h>
#include <string.h>
#include <cstring>
#include <string>
#include <cassert>
#include "pong.hpp"

#include "../common/logging.hpp"
#include "../common/maths_utils.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../platform/interface/display.hpp"
#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "../menu.hpp"
#include "../common/common_transitions.hpp"
#include "../apps/settings.hpp"

#define TAG "Pong"

PongConfiguration DEFAULT_PONG_GAME_CONFIG = {
    .header = ConfigurationHeader(),
};

const char *Pong::get_game_name() const { return "Pong"; }
const char *Pong::get_help_text() const { return "TODO"; }

void draw_pong_canvas(const Platform &p,
                      const SquareCellGridDimensions &dimensions,
                      const UserInterfaceCustomization &customization)

{
        p.display->initialize();
        p.display->clear(Black);

        if (customization.rendering_mode == Detailed)
                p.display->draw_rounded_border(customization.accent_color);

        int x_margin = dimensions.left_horizontal_margin;
        int y_margin = dimensions.top_vertical_margin;

        int actual_width = dimensions.actual_width;
        int actual_height = dimensions.actual_height;

        int border_width = 2;
        // We need to make the border rectangle and the canvas slightly
        // bigger to ensure that it does not overlap with the game area.
        // Otherwise the caret rendering erases parts of the border as
        // it moves around (as the caret intersects with the border
        // partially)
        int border_offset = 1;

        /* We don't draw the individual rectangles to make rendering
           faster on the physical Arduino LCD display. */
        p.display->clear_region(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            {.x = x_margin + actual_width + border_offset,
             .y = y_margin + actual_height + border_offset},
            Black);

        p.display->draw_rectangle(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            actual_width + 2 * border_offset, actual_height + 2 * border_offset,
            customization.accent_color, border_width, false);

        if (customization.show_help_text) {
                std::map<Action, std::string> button_hints;
                button_hints[BACK_ACTION] = "Quit";
                button_hints[CONFIRM_ACTION] = "Continue";
                button_hints[FORWARD_ACTION] = "Pause";
                button_hints[HELP_ACTION] = "Help";
                render_controls_explanations(*p.display,
                                             p.capabilities.action_button_kind,
                                             button_hints);
        }
}

struct Ball {
        Circle circle;
        Point velocity;
};

struct Paddle {
        Rectangle body;
        Point velocity;
        Point acceleration;
};

int calculate_impact_position(double ball_center_y, Point ball_velocity,
                              int paddle_displacement, Point top_left,
                              Point border_dimensions)
{

        auto [vx, vy] = ball_velocity;
        double travel_time = paddle_displacement / vx;
        double total_y_travel = abs(travel_time * vy);

        // figure out if we need to do mod
        double final_y_with_no_border = ball_center_y + total_y_travel;
        double y_lo_bound = top_left.y;
        double y_up_bound = top_left.y + border_dimensions.y;
        if (y_lo_bound <= final_y_with_no_border &&
            final_y_with_no_border <= y_up_bound) {
                return final_y_with_no_border;
        }

        double towards_wall;
        if (vy > 0) {
                towards_wall = top_left.y + border_dimensions.y - ball_center_y;
        } else {
                towards_wall = ball_center_y - top_left.y;
        }

        LOG_DEBUG(TAG, "Ball center y %f", ball_center_y);
        LOG_DEBUG(TAG, "Total y travel before the wall %f", total_y_travel);
        total_y_travel -= towards_wall;
        // now we cancel out all complete bounces
        int bounces = 1 + total_y_travel / border_dimensions.y;
        int remaining = (int)total_y_travel % (int)border_dimensions.y;

        LOG_DEBUG(TAG, "Total y travel after hitting the wall %f",
                  total_y_travel);
        LOG_DEBUG(TAG, "Total y space %f", border_dimensions.y);
        LOG_DEBUG(TAG, "Total y travel after bouncing %d", remaining);
        LOG_DEBUG(TAG, "Expecting %d bounces", bounces);
        LOG_DEBUG(TAG, "Ball travelling %s ", vy > 0 ? "downwards" : "upwards");
        if (vy > 0) {
                if (bounces % 2 == 0) {
                        return top_left.y + remaining;
                } else {
                        return top_left.y + border_dimensions.y - remaining;
                }
        } else {
                if (bounces % 2 == 0) {
                        return top_left.y + border_dimensions.y - remaining;
                } else {
                        return top_left.y + remaining;
                }
        }
}

/**
 * When managing position of the player / CPU paddles, we need to ensure that
 * they don't go out of bounds of the game area. To test for this, we try to
 * move the paddle by the displacement and check if it went outside. If it did,
 * the caller code is responsible for preventing the paddle from making this
 * move.
 */
bool next_step_outside(const Paddle &paddle, double displacement,
                       const LineSegment &top_wall,
                       const LineSegment &bottom_wall);
UserAction Pong::app_loop(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          const PongConfiguration &config) const
{

        int game_cell_width = 2;
        auto gd =
            std::unique_ptr<SquareCellGridDimensions>(calculate_grid_dimensions(
                p.display->get_width(), p.display->get_height(),
                p.display->get_display_corner_radius(), game_cell_width));
        int rows = gd->rows;
        int cols = gd->cols;

        draw_pong_canvas(p, *gd, customization);

        // We need to locate the grid vertices to assemble wall segments.
        int radius = 3;
        // this padding is needed so that the ball doesn't clip the walls of
        // the game grid.
        int padding = 1;
        int game_area_width = gd->actual_width - 2 * padding;
        int game_area_height = gd->actual_height - 2 * padding;
        Point top_left = {(double)gd->left_horizontal_margin + padding,
                          (double)gd->top_vertical_margin + padding};
        Point top_right =
            top_left + Point{(double)gd->actual_width - 2 * padding, 0};
        Point bottom_left =
            top_left + Point{0, (double)gd->actual_height - 2 * padding};
        Point bottom_right =
            top_right + Point{0, (double)gd->actual_height - 2 * padding};

        LineSegment top_wall{top_left, top_right};
        LineSegment bottom_wall{bottom_left, bottom_right};
        LineSegment left_wall{top_left, bottom_left};
        LineSegment right_wall{top_right, bottom_right};

        std::vector<LineSegment *> walls = {&top_wall, &bottom_wall, &left_wall,
                                            &right_wall};

        int paddle_len = gd->actual_height / 4;
        int paddle_w = 5;
        // some intense maths here to make the paddle centered.
        Point paddle_start = {top_left.x + padding + paddle_w,
                              top_left.y +
                                  (gd->actual_height - 2 * padding) / 2.0 -
                                  paddle_len / 2.0};
        Point paddle_end = paddle_start + Point{0, (double)paddle_len};

        Paddle paddle{
            .body = {paddle_start, (double)paddle_w, (double)paddle_len},
            .velocity = {0, 0},
            .acceleration = {0.1, 0.1},
        };

        Point cpu_paddle_offset{(double)gd->actual_width -
                                    2 * ((double)paddle_w + padding) - paddle_w,
                                0};

        Paddle cpu_paddle{
            .body = {paddle_start + cpu_paddle_offset, (double)paddle_w,
                     (double)paddle_len},
            .velocity = {0, 0},
            .acceleration = {0.1, 0.1},
        };

        Point pos = {paddle_end.x + 10, gd->actual_height / 2.0};
        double initial_v = 1.0;
        Point v = {initial_v, initial_v};
        double friction = 0.25;
        Ball ball{Circle{pos, (double)radius}, v};
        int time_delta = 1000 / config.initial_speed; // ms

        // Rendering lambdas to make the logic code more readable.
        auto erase_paddle = [&](Rectangle paddle) {
                p.display->draw_rectangle(paddle.top_left.cast(), paddle_w,
                                          paddle_len, Black, 1, false);
        };
        auto render_paddle = [&](Rectangle paddle) {
                p.display->draw_rectangle(paddle.top_left.cast(), paddle_w,
                                          paddle_len,
                                          customization.accent_color, 1, false);
        };
        auto erase_ball = [&](Ball ball) {
                p.display->draw_circle(ball.circle.center.cast(), radius, Black,
                                       1, true);
        };
        auto render_ball = [&](Ball ball) {
                p.display->draw_circle(ball.circle.center.cast(), radius, Red,
                                       1, true);
        };

        render_paddle(paddle.body);
        render_paddle(cpu_paddle.body);

        Point game_area_dimensions = {(double)game_area_width,
                                      (double)game_area_height};
        int distance_between_paddles =
            game_area_width - 2 * (paddle_w + padding);
        int ball_impact_y = calculate_impact_position(
            ball.circle.center.y, ball.velocity, distance_between_paddles,
            top_left, game_area_dimensions);

        bool game_over = false;
        bool game_paused = false;
        bool action_input_on_last_iteration = false;
        while (!game_over) {
                auto maybe_action = poll_action_input(p.action_controllers);
                if (maybe_action.has_value() &&
                    maybe_action.value() == BACK_ACTION) {
                        break;
                }
                if (maybe_action.has_value() &&
                    maybe_action.value() == FORWARD_ACTION &&
                    !action_input_on_last_iteration) {
                        game_paused = !game_paused;
                        action_input_on_last_iteration = true;
                        p.time_provider->delay_ms(INPUT_POLLING_DELAY);
                }
                if (!maybe_action.has_value())
                        action_input_on_last_iteration = false;
                if (game_paused) {
                        p.time_provider->delay_ms(INPUT_POLLING_DELAY);
                        continue;
                }

                auto maybe_direction =
                    poll_directional_input(p.directional_controllers);
                if (maybe_direction.has_value()) {
                        auto dir = maybe_direction.value();
                        if (dir == Direction::UP || dir == Direction::DOWN) {
                                int dir_sign = dir == Direction::UP ? -1 : 1;
                                paddle.velocity.y +=
                                    dir_sign * paddle.acceleration.y;
                                Point off = {0, paddle.velocity.y};

                                if (!next_step_outside(paddle, off.y, top_wall,
                                                       bottom_wall)) {
                                        erase_paddle(paddle.body);
                                        paddle.body.top_left =
                                            paddle.body.top_left + off;
                                        render_paddle(paddle.body);
                                } else {
                                        paddle.velocity.y = 0;
                                }
                        }
                } else {
                        // For now we do no deceleration.
                        paddle.velocity = {0, 0};
                }

                // Handle cpu paddle. We move it until the paddle covers the
                // calculated ball impact position. We also ensure that if the
                // ball impact y is to close to the top/bottom border, the
                // paddle doesn't clip through that wall.
                erase_paddle(cpu_paddle.body);
                double center_y =
                    cpu_paddle.body.top_left.y + (double)paddle_len / 2;
                bool hits_top_wall = next_step_outside(cpu_paddle, -initial_v,
                                                       top_wall, bottom_wall);
                bool hits_bottom_wall = next_step_outside(
                    cpu_paddle, initial_v, top_wall, bottom_wall);
                if (center_y > ball_impact_y && !hits_top_wall) {
                        cpu_paddle.body.top_left.y -= initial_v;
                } else if (center_y < ball_impact_y && !hits_bottom_wall) {
                        cpu_paddle.body.top_left.y += initial_v;
                }
                render_paddle(cpu_paddle.body);

                erase_ball(ball);
                ball.circle.center = ball.circle.center + ball.velocity;
                render_ball(ball);

                // collision detection and handling
                for (const auto &seg : walls) {
                        if (!collides(ball.circle, *seg))
                                continue;
                        if (seg == &left_wall || seg == &right_wall)
                                game_over = true;
                        if (seg->is_horizontal())
                                ball.velocity.y *= -1;
                        if (seg->is_vertical())
                                ball.velocity.x *= -1;
                }
                if (collides(ball.circle, paddle.body)) {
                        if (collides(ball.circle, paddle.body.get_top_edge()) ||
                            collides(ball.circle,
                                     paddle.body.get_bottom_edge())) {
                                // This prevents the ball from clipping into the
                                // paddle when it hits the top or bottom edge of
                                // the paddle.
                                ball.velocity.y = -ball.velocity.y;

                                // we re-render the paddle just in case the ball
                                // has erased a part of its top/bottom edge.
                                erase_paddle(paddle.body);
                                render_paddle(paddle.body);
                        } else {
                                ball.velocity.x = -ball.velocity.x;
                                // the velocity of the paddle is partially
                                // tranferred to the vertical velocity of the
                                // ball. This is controlled by the friction
                                // coefficient.
                                ball.velocity.y += paddle.velocity.y * friction;
                                ball_impact_y = calculate_impact_position(
                                    ball.circle.center.y, ball.velocity,
                                    distance_between_paddles, top_left,
                                    game_area_dimensions);
                        }
                }
                if (collides(ball.circle, cpu_paddle.body)) {
                        ball.velocity.x = -ball.velocity.x;
                        // here the impact is likely 0 as the CPU paddle will
                        // have precomputed the required position and so it will
                        // be fully stationary by the time the ball reaches it.
                        ball.velocity.y += cpu_paddle.velocity.y * friction;
                }

                if (!p.display->refresh())
                        return UserAction::CloseWindow;
                p.time_provider->delay_ms(time_delta);
        }
        wait_until_green_pressed(p);

        return UserAction::PlayAgain;
}

bool next_step_outside(const Paddle &paddle, double displacement,
                       const LineSegment &top_wall,
                       const LineSegment &bottom_wall)
{
        bool outside = false;
        double new_top, new_bottom;
        new_top = paddle.body.top_left.y + displacement;
        new_bottom = paddle.body.top_left.y + paddle.body.height + displacement;
        outside |= new_top <= top_wall.start.y;
        outside |= new_bottom >= bottom_wall.end.y;
        return outside;
}

PongConfiguration *load_initial_pong_config(const PersistentStorage &storage)
{
        int storage_offset = get_settings_storage_offset(Game::Pong);

        PongConfiguration config;
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent storage "
                  "at offset %d",
                  storage_offset);
        storage.get(storage_offset, config);

        PongConfiguration *output = new PongConfiguration();

        if (!config.header.validate_against(DEFAULT_PONG_GAME_CONFIG)) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "pong game configuration, using default values.");
                memcpy(output, &DEFAULT_PONG_GAME_CONFIG,
                       sizeof(PongConfiguration));
                storage.put(storage_offset, DEFAULT_PONG_GAME_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(PongConfiguration));
        }

        return output;
}

/**
 * Assembles the generic configuration struct that is needed to collect
 * user defined game configuration for pong. Note that this is a
 * declarative way of defining what can be configured and the UI code
 * then dynamically renders selectors and handles switching between
 * option values.
 *
 * WARNING: This is tightly coupled with the
 * `extract_game_config` function. If you change the
 * structure of this config, make sure to make a corresponding update to
 * that function below to ensure that the specific game config can be
 * successfully extracted from the generic config struct.
 */
Configuration *assemble_pong_configuration(PersistentStorage *storage,
                                           PongConfiguration *initial_config)
{
        auto *initial_speed = ConfigurationOption::of_integers(
            "Speed (px/s)", {100, 150, 200, 250},
            initial_config->initial_speed);

        std::vector<ConfigurationOption *> options = {initial_speed};

        return new Configuration("Pong", options);
}
void extract_game_config(PongConfiguration &game_config,
                         const PongConfiguration &initial_config,
                         const Configuration &config)
{
        ConfigurationOption initial_speed = *config.options[0];
        game_config.initial_speed = initial_speed.get_curr_int_value();
}

std::optional<UserAction>
Pong::collect_config(const Platform &p,
                     const UserInterfaceCustomization &customization,
                     PongConfiguration &game_config) const
{
        auto initial_cfg = std::unique_ptr<PongConfiguration>(
            load_initial_pong_config(*p.persistent_storage));
        auto cfg = std::unique_ptr<Configuration>(assemble_pong_configuration(
            p.persistent_storage, initial_cfg.get()));

        auto interrupt = collect_configuration(p, *cfg, customization);
        if (interrupt)
                return interrupt;
        extract_game_config(game_config, *initial_cfg, *cfg);
        return std::nullopt;
}
