#include <cstring>
#include <optional>
#include "../platform/interface/platform.hpp"
#include "../menu.hpp"

#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "../common/constants.hpp"

#include "../common/common_transitions.hpp"
#include "../apps/settings.hpp"
#include "minesweeper.hpp"

#define TAG "minesweeper"

MinesweeperConfiguration DEFAULT_MINESWEEPER_CONFIG = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 1}, .mines_num = 25};

struct MinesweeperGridDimensions {
        int rows;
        int cols;
        int top_vertical_margin;
        int left_horizontal_margin;
        int actual_width;
        int actual_height;

        MinesweeperGridDimensions(int r, int c, int tvm, int lhm, int aw,
                                  int ah)
            : rows(r), cols(c), top_vertical_margin(tvm),
              left_horizontal_margin(lhm), actual_width(aw), actual_height(ah)
        {
        }
};

struct MinesweeperGridCell {
        bool is_bomb;
        bool is_flagged;
        bool is_uncovered;
        int adjacent_bombs;

        MinesweeperGridCell()
            : is_bomb(false), is_flagged(false), is_uncovered(false),
              adjacent_bombs(0)
        {
        }
};

static MinesweeperGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius);
static void draw_game_canvas(const Platform &p,
                             const MinesweeperGridDimensions &dimensions,
                             const UserInterfaceCustomization &customization);

static void erase_caret(const Display &display, const Point &grid_position,
                        const MinesweeperGridDimensions &dimensions,
                        Color grid_background_color);
static void draw_caret(const Display &display, const Point &grid_position,
                       const MinesweeperGridDimensions &dimensions);
/**
 * Performs a recursive uncovering waterfall: tries to uncover the current
 * cell, if the cell has 0 adjacent mines it uncovers all of its neighbours.
 */
static std::optional<UserAction> uncover_grid_cells_starting_from(
    const Display &display, const Point &grid_position,
    const MinesweeperGridDimensions &dimensions,
    std::vector<std::vector<MinesweeperGridCell>> &grid, int &total_uncovered);
static void
uncover_grid_cell(const Display &display, const Point &grid_position,
                  const MinesweeperGridDimensions &dimensions,
                  std::vector<std::vector<MinesweeperGridCell>> &grid,
                  int &total_uncovered);
static void flag_grid_cell(const Display &display, const Point &grid_position,
                           const MinesweeperGridDimensions &dimensions,
                           std::vector<std::vector<MinesweeperGridCell>> &grid,
                           const UserInterfaceCustomization &customization);
static void
unflag_grid_cell(const Display &display, const Point &grid_position,
                 const MinesweeperGridDimensions &dimensions,
                 std::vector<std::vector<MinesweeperGridCell>> &grid,
                 Color grid_background_color);

void place_bombs(std::vector<std::vector<MinesweeperGridCell>> &grid,
                 int bomb_number, const Point &caret_position);

const char *Minesweeper::get_game_name() const { return "Minesweeper"; }
const char *Minesweeper::get_help_text() const
{
        return "Use the joystick to move the caret around the grid. Press "
               "green "
               "to uncover a cell, red to place a flag. The aim "
               "is to uncover all cells with no mines. Digits in the grid tell "
               "you"
               " the number of mines around the cell.";
}
UserAction
Minesweeper::app_loop(const Platform &p,
                      const UserInterfaceCustomization &customization,
                      const MinesweeperConfiguration &config) const
{
        LOG_DEBUG(TAG, "Entering Minesweeper game loop");

        auto gd = std::unique_ptr<MinesweeperGridDimensions>(
            calculate_grid_dimensions(p.display->get_width(),
                                      p.display->get_height(),
                                      p.display->get_display_corner_radius()));
        int rows = gd->rows;
        int cols = gd->cols;

        draw_game_canvas(p, *gd, customization);
        LOG_DEBUG(TAG, "Minesweeper game canvas drawn.");

        if (!p.display->refresh()) {
                return UserAction::CloseWindow;
        }

        std::vector<std::vector<MinesweeperGridCell>> grid(
            rows, std::vector<MinesweeperGridCell>(cols));

        auto grid_at = [&](const Point &location) -> MinesweeperGridCell {
                return grid[location.y][location.x];
        };

        /* We only place bombs after the user selects the cell to
           uncover. This avoids situations where the first selected cell
           is a bomb and the game is immediately over without user's
           logical error. */
        bool bombs_placed = false;

        Point caret_position = {.x = 0, .y = 0};
        draw_caret(*p.display, caret_position, *gd);
        LOG_DEBUG(TAG, "Caret rendered at initial position.");

        int total_uncovered = 0;

        // To avoid button debounce issues, we only process action input if
        // it wasn't processed on the last iteration. This is to avoid
        // situations where the user holds the 'spawn' button for too long and
        // the cell flickers instead of getting spawned properly. We implement
        // this using this flag.
        bool action_input_on_last_iteration = false;
        bool is_game_over = false;
        while (!is_game_over &&
               !(total_uncovered == cols * rows - config.mines_num)) {
                auto maybe_direction =
                    poll_directional_input(p.directional_controllers);
                if (maybe_direction) {
                        Direction dir = maybe_direction.value();
                        LOG_DEBUG(TAG, "Directional input received: %s",
                                  direction_to_str(dir));

                        if (!bombs_placed) {
                                /* Before the bombs are placed, we spin the
                                   random number generator on each step to
                                   ensure that we don't generate the same grid
                                   every time we start the game console. */
                                srand(rand());
                        }

                        /* Once the cells become uncovered, the background is
                        set to black. Because of this, we need to change the
                        erase color */
                        if (grid_at(caret_position).is_uncovered) {
                                erase_caret(*p.display, caret_position, *gd,
                                            Black);
                                // We need to 'uncover' the cell again to ensure
                                // that the numbers don't get cropped after the
                                // caret overlaps with them.
                                uncover_grid_cell(*p.display, caret_position,
                                                  *gd, grid, total_uncovered);
                        } else if (grid_at(caret_position).is_flagged) {
                                erase_caret(*p.display, caret_position, *gd,
                                            customization.accent_color);
                                // We need to unflag and flag the cell again to
                                // ensure that the flag indicator doesn't get
                                // cropped after the caret overlaps with them.
                                unflag_grid_cell(*p.display, caret_position,
                                                 *gd, grid,
                                                 customization.accent_color);
                                flag_grid_cell(*p.display, caret_position, *gd,
                                               grid, customization);
                        } else {
                                erase_caret(*p.display, caret_position, *gd,
                                            customization.accent_color);
                        }
                        translate_within_bounds(caret_position, dir, gd->rows,
                                                gd->cols);
                        draw_caret(*p.display, caret_position, *gd);

                        // We need to manually refresh each time we draw
                        // something. This is required because if the window is
                        // closed, we need to handle this accordingly and free
                        // all resources.
                        if (!p.display->refresh()) {
                                return UserAction::CloseWindow;
                        }
                        p.time_provider->delay_ms(MOVE_REGISTERED_DELAY);
                        /* We continue here to skip the additional input
                           polling delay at the end of the loop and make
                           the input snappy. */
                        continue;
                }
                auto maybe_action = poll_action_input(p.action_controllers);
                if (maybe_action.has_value() &&
                    !action_input_on_last_iteration) {
                        Action act = maybe_action.value();
                        action_input_on_last_iteration = true;
                        LOG_DEBUG(TAG, "Action input received: %s",
                                  action_to_str(act));

                        MinesweeperGridCell cell =
                            grid[caret_position.y][caret_position.x];
                        switch (act) {
                        case Action::RED:
                                if (!cell.is_uncovered) {
                                        if (!cell.is_flagged) {
                                                flag_grid_cell(
                                                    *p.display, caret_position,
                                                    *gd, grid, customization);

                                        } else {
                                                unflag_grid_cell(
                                                    *p.display, caret_position,
                                                    *gd, grid,
                                                    customization.accent_color);
                                                draw_caret(*p.display,
                                                           caret_position, *gd);
                                        }
                                }
                                break;
                        case Action::GREEN:
                                /* We place bombs only after the first
                                   cell is uncovered. This is done to
                                   avoid the situation where the first
                                   cell is a bomb and we are getting an
                                   instant game-over. */
                                if (!bombs_placed) {
                                        place_bombs(grid, config.mines_num,
                                                    caret_position);
                                        bombs_placed = true;
                                        LOG_DEBUG(TAG, "Bombs placed.");
                                }
                                if (cell.is_bomb) {
                                        is_game_over = true;
                                }
                                if (!cell.is_flagged) {
                                        auto maybe_interrupt =
                                            uncover_grid_cells_starting_from(
                                                *p.display, caret_position, *gd,
                                                grid, total_uncovered);

                                        draw_caret(*p.display, caret_position,
                                                   *gd);
                                        if (maybe_interrupt) {
                                                return maybe_interrupt.value();
                                        }
                                        break;
                                }

                        case Action::BLUE:
                                p.time_provider->delay_ms(INPUT_POLLING_DELAY);
                                return UserAction::Exit;
                        default:
                                LOG_DEBUG(TAG, "Irrelevant action input: %s",
                                          action_to_str(act));
                                break;
                        }
                        if (!p.display->refresh()) {
                                return UserAction::CloseWindow;
                        }
                        p.time_provider->delay_ms(MOVE_REGISTERED_DELAY);
                        /* We continue here to skip the additional input
                           polling delay at the end of the loop and make
                           the input snappy. */
                        continue;
                } else {
                        action_input_on_last_iteration = false;
                }

                if (!p.display->refresh()) {
                        return UserAction::CloseWindow;
                }
                p.time_provider->delay_ms(INPUT_POLLING_DELAY);
        }

        // When the game is lost, we make all bombs explode.
        if (is_game_over) {
                for (int y = 0; y < rows; y++) {
                        for (int x = 0; x < cols; x++) {
                                MinesweeperGridCell cell = grid[y][x];
                                if (cell.is_bomb) {
                                        Point point = {.x = x, .y = y};
                                        uncover_grid_cell(*p.display, point,
                                                          *gd, grid,
                                                          total_uncovered);
                                }
                        }
                }

                pause_until_any_directional_input(p.directional_controllers,
                                                  *p.time_provider, *p.display);
                display_game_over(*p.display, customization);
                p.time_provider->delay_ms(MOVE_REGISTERED_DELAY);
        } else {
                pause_until_any_directional_input(p.directional_controllers,
                                                  *p.time_provider, *p.display);
                display_game_won(*p.display, customization);
                p.time_provider->delay_ms(MOVE_REGISTERED_DELAY);
        }
        if (!p.display->refresh()) {
                return UserAction::CloseWindow;
        }
        return UserAction::PauseAndPlayAgain;
}

void place_bombs(std::vector<std::vector<MinesweeperGridCell>> &grid,
                 int bomb_number, const Point &caret_position)
{
        int rows = grid.size();
        int cols = (*grid.begin().base()).size();
        for (int i = 0; i < bomb_number; i++) {
                while (true) {
                        int x = rand() % cols;
                        int y = rand() % rows;

                        Point random_position = {.x = x, .y = y};

                        bool is_close_to_caret =
                            is_adjacent(caret_position, random_position);
                        if (!grid[y][x].is_bomb && !is_close_to_caret) {
                                grid[y][x].is_bomb = true;
                                grid[y][x].adjacent_bombs = 0;

                                Point current = {.x = x, .y = y};
                                for (Point nb : get_neighbours_inside_grid(
                                         current, rows, cols)) {
                                        grid[nb.y][nb.x].adjacent_bombs++;
                                }

                                break;
                        }
                }
        }
}

void erase_caret(const Display &display, const Point &grid_position,
                 const MinesweeperGridDimensions &dimensions,
                 Color grid_background_color)
{
        // We need to ensure that the caret is rendered INSIDE the text
        // cell and its border doesn't overlap the neighbouring cells.
        // Otherwise, we'll get weird rendering artifacts where the
        // border get clipped.
        int border_offset = 1;
        Point actual_position = {
            .x = dimensions.left_horizontal_margin +
                 grid_position.x * FONT_WIDTH + border_offset,
            .y = dimensions.top_vertical_margin + grid_position.y * FONT_SIZE +
                 border_offset};

        display.draw_rectangle(actual_position, FONT_WIDTH - 2 * border_offset,
                               FONT_SIZE - 2 * border_offset,
                               grid_background_color, 1, false);
}

void draw_caret(const Display &display, const Point &grid_position,
                const MinesweeperGridDimensions &dimensions)
{

        // We need to ensure that the caret is rendered INSIDE the text
        // cell and its border doesn't overlap the neighbouring cells.
        // Otherwise, we'll get weird rendering artifacts.
        int border_offset = 1;
        Point actual_position = {
            .x = dimensions.left_horizontal_margin +
                 grid_position.x * FONT_WIDTH + border_offset,
            .y = dimensions.top_vertical_margin + grid_position.y * FONT_SIZE +
                 border_offset};

        display.draw_rectangle(actual_position, FONT_WIDTH - 2 * border_offset,
                               FONT_SIZE - 2 * border_offset, White, 1, false);
}
void uncover_grid_cell(const Display &display, const Point &grid_position,
                       const MinesweeperGridDimensions &dimensions,
                       std::vector<std::vector<MinesweeperGridCell>> &grid,
                       int &total_uncovered)
{

        char text[2];

        MinesweeperGridCell cell = grid[grid_position.y][grid_position.x];
        // We need this check as we 're-uncover' cells after the caret
        // passes over them to remove rendering overlap artifacts.
        if (!cell.is_uncovered) {
                total_uncovered++;
                grid[grid_position.y][grid_position.x].is_uncovered = true;
        }
        Color text_color = White;
        if (cell.is_bomb) {
                sprintf(text, "*");
        } else if (cell.adjacent_bombs == 0) {
                sprintf(text, " ");
        } else {
                sprintf(text, "%d", cell.adjacent_bombs);
                /* We override the rendering color depending on the
                   number of bombs around the cell to make it easier to
                   read the UI. */
                switch (cell.adjacent_bombs) {
                case 1:
                        text_color = Cyan;
                        break;
                case 2:
                        text_color = Green;
                        break;
                case 3:
                        text_color = Red;
                        break;
                case 4:
                        text_color = Magenta;
                        break;
                }
        }
        Point actual_position = {.x = dimensions.left_horizontal_margin +
                                      grid_position.x * FONT_WIDTH,
                                 .y = dimensions.top_vertical_margin +
                                      grid_position.y * FONT_SIZE};
        int border_offset = 1;
        display.draw_rectangle({actual_position.x + border_offset,
                                actual_position.y + border_offset},
                               FONT_WIDTH - 2 * border_offset,
                               FONT_SIZE - 2 * border_offset, Black, 1, true);

        display.draw_string(actual_position, text, FontSize::Size16, Black,
                            text_color);
}

std::optional<UserAction> uncover_grid_cells_starting_from(
    const Display &display, const Point &grid_position,
    const MinesweeperGridDimensions &dimensions,
    std::vector<std::vector<MinesweeperGridCell>> &grid, int &total_uncovered)
{

        uncover_grid_cell(display, grid_position, dimensions, grid,
                          total_uncovered);

        // We need to react to the window close even if it happens when
        // the recursive uncovering is happening. Else we are risking
        // leaking resources.
        if (!display.refresh()) {
                return UserAction::CloseWindow;
        }

        int rows = grid.size();
        int cols = (*grid.begin().base()).size();
        MinesweeperGridCell current_cell =
            grid[grid_position.y][grid_position.x];

        if (!current_cell.is_bomb && current_cell.adjacent_bombs == 0) {
                auto neighbours =
                    get_neighbours_inside_grid(grid_position, rows, cols);
                for (Point nb : neighbours) {
                        MinesweeperGridCell neighbour_cell = grid[nb.y][nb.x];

                        if (!neighbour_cell.is_uncovered &&
                            !neighbour_cell.is_flagged) {
                                uncover_grid_cells_starting_from(
                                    display, nb, dimensions, grid,
                                    total_uncovered);
                        }
                }
        }
        return std::nullopt;
}

void flag_grid_cell(const Display &display, const Point &grid_position,
                    const MinesweeperGridDimensions &dimensions,
                    std::vector<std::vector<MinesweeperGridCell>> &grid,
                    const UserInterfaceCustomization &customization)
{

        grid[grid_position.y][grid_position.x].is_flagged = true;
        Point actual_position = {.x = dimensions.left_horizontal_margin +
                                      grid_position.x * FONT_WIDTH,
                                 .y = dimensions.top_vertical_margin +
                                      grid_position.y * FONT_SIZE};

        {
                char text[2];
                sprintf(text, "f");
                display.draw_string(actual_position, text, FontSize::Size16,
                                    customization.accent_color, White);
        }
        // TODO: figure out what this is
        if (false) {
                char text[2];
                sprintf(text, "*");
                display.draw_string(actual_position, text, FontSize::Size16,
                                    customization.accent_color, White);
        }
}

void unflag_grid_cell(const Display &display, const Point &grid_position,
                      const MinesweeperGridDimensions &dimensions,
                      std::vector<std::vector<MinesweeperGridCell>> &grid,
                      Color grid_background_color)
{

        grid[grid_position.y][grid_position.x].is_flagged = false;
        Point actual_position = {.x = dimensions.left_horizontal_margin +
                                      grid_position.x * FONT_WIDTH,
                                 .y = dimensions.top_vertical_margin +
                                      grid_position.y * FONT_SIZE};

        display.clear_region(actual_position,
                             {.x = actual_position.x + FONT_WIDTH,
                              .y = actual_position.y + FONT_SIZE},
                             grid_background_color);
}

Configuration *
assemble_minesweeper_configuration(const PersistentStorage &storage);
void extract_game_config(MinesweeperConfiguration &game_config,
                         const Configuration &config);

std::optional<UserAction>
Minesweeper::collect_config(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            MinesweeperConfiguration &game_config) const
{
        auto config = std::unique_ptr<Configuration>(
            assemble_minesweeper_configuration(*p.persistent_storage));

        auto maybe_interrupt = collect_configuration(p, *config, customization);
        if (maybe_interrupt)
                return maybe_interrupt;

        extract_game_config(game_config, *config.get());
        return std::nullopt;
}

MinesweeperConfiguration *
load_initial_minesweeper_config(const PersistentStorage &storage)
{
        int storage_offset = get_settings_storage_offset(Game::Minesweeper);
        LOG_DEBUG(TAG, "Loading minesweeper saved config from offset %d",
                  storage_offset);

        MinesweeperConfiguration config;

        LOG_DEBUG(TAG, "Trying to load initial settings from the "
                       "persistent storage");
        storage.get(storage_offset, config);

        MinesweeperConfiguration *output = new MinesweeperConfiguration();

        if (!config.header.validate_against(DEFAULT_MINESWEEPER_CONFIG)) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "minesweeper configuration, using default values.");
                memcpy(output, &DEFAULT_MINESWEEPER_CONFIG,
                       sizeof(MinesweeperConfiguration));
                storage.put(storage_offset, DEFAULT_MINESWEEPER_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(MinesweeperConfiguration));
        }

        LOG_DEBUG(TAG, "Loaded minesweeper game configuration: mines_num=%d",
                  output->mines_num);

        return output;
}

Configuration *
assemble_minesweeper_configuration(const PersistentStorage &storage)
{
        auto initial_config = std::unique_ptr<MinesweeperConfiguration>(
            load_initial_minesweeper_config(storage));

        ConfigurationOption *mines_count = ConfigurationOption::of_integers(
            "Number of mines", {10, 15, 25, 30, 35}, initial_config->mines_num);

        std::vector<ConfigurationOption *> options = {mines_count};

        return new Configuration("Minesweeper", options);
}

void extract_game_config(MinesweeperConfiguration &game_config,
                         const Configuration &config)
{
        ConfigurationOption mines_num = *config.options[0];
        game_config.mines_num = mines_num.get_curr_int_value();
}

MinesweeperGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius)
{
        // Bind input params to short names for improved readability.
        int w = display_width;
        int h = display_height;
        int r = display_rounded_corner_radius;

        int usable_width = w - r;
        int usable_height = h - r;

        int max_cols = usable_width / FONT_WIDTH;
        int max_rows = usable_height / FONT_SIZE;

        int actual_width = max_cols * FONT_WIDTH;
        int actual_height = max_rows * FONT_SIZE;

        // We calculate centering margins
        int left_horizontal_margin = (w - actual_width) / 2;
        int top_vertical_margin = (h - actual_height) / 2;

        LOG_DEBUG(TAG,
                  "Calculated grid dimensions: %d rows, %d cols, "
                  "left margin: %d, top margin: %d, actual width: %d, "
                  "actual height: %d",
                  max_rows, max_cols, left_horizontal_margin,
                  top_vertical_margin, actual_width, actual_height);

        return new MinesweeperGridDimensions(
            max_rows, max_cols, top_vertical_margin, left_horizontal_margin,
            actual_width, actual_height);
}

void draw_controls_hints(const Display &display,
                         const MinesweeperGridDimensions &dimensions,
                         int border_offset);
void draw_game_canvas(const Platform &p,
                      const MinesweeperGridDimensions &dimensions,
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
            customization.accent_color);

        p.display->draw_rectangle(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            actual_width + 2 * border_offset, actual_height + 2 * border_offset,
            Gray, border_width, false);

        if (customization.show_help_text) {
                draw_controls_hints(*p.display, dimensions, border_offset);
        }
}

void draw_controls_hints(const Display &display,
                         const MinesweeperGridDimensions &dimensions,
                         int border_offset)
{
        int x_margin = dimensions.left_horizontal_margin;
        int y_margin = dimensions.top_vertical_margin;

        int actual_width = dimensions.actual_width;
        int actual_height = dimensions.actual_height;
        int text_below_grid_y = y_margin + actual_height + 2 * border_offset;
        int r = 2;
        int d = 2 * r;
        int circle_y_axis = text_below_grid_y + FONT_SIZE / 2 + r / 4;
        const char *select = "Select";
        int select_len = strlen(select) * FONT_WIDTH;
        const char *flag = "Flag";
        int flag_len = strlen(flag) * FONT_WIDTH;
        // We calculate the even spacing for the two indicators
        int circles_width = 2 * d;
        int total_width = select_len + flag_len + circles_width;
        int available_width = display.get_width() - 2 * x_margin;
        int remainder_space = available_width - total_width;
        int even_separator = remainder_space / 3;

        int green_circle_x = x_margin + even_separator;
        display.draw_circle({.x = green_circle_x, .y = circle_y_axis}, r, Green,
                            0, true);

        int select_text_x = green_circle_x + d;
        display.draw_string({.x = select_text_x, .y = text_below_grid_y},
                            (char *)select, FontSize::Size16, Black, White);

        int flag_red_circle_x = select_text_x + select_len + even_separator;
        display.draw_circle({.x = flag_red_circle_x, .y = circle_y_axis}, r,
                            Red, 0, true);
        int flag_text_x = flag_red_circle_x + d;
        display.draw_string({.x = flag_text_x, .y = text_below_grid_y},
                            (char *)flag, FontSize::Size16, Black, White);
}

void Minesweeper::render_thumbnail(
    const Platform &platform, const UserInterfaceCustomization &customization)
{

        const Display &display = *platform.display;

        clear_half_display_and_render_subtitle(platform, customization,
                                               "Minesweeper");

        TftCompatibleDisplay &tft =
            *platform.display->cast_into_tft_compatible();

        // [BEGIN lopaka generated]
        auto draw_polygon_10_copy_5 = [&]() {
                tft.drawLine(161, 155, 167, 149, 0xBDF7);
                tft.drawLine(167, 149, 171, 153, 0xBDF7);
                tft.drawLine(171, 153, 165, 159, 0xBDF7);
                tft.drawLine(165, 159, 161, 155, 0xBDF7);
        };

        auto draw_polygon_10_copy_11 = [&]() {
                tft.drawLine(137, 177, 143, 171, 0xBDF7);
                tft.drawLine(143, 171, 147, 175, 0xBDF7);
                tft.drawLine(147, 175, 141, 181, 0xBDF7);
                tft.drawLine(141, 181, 137, 177, 0xBDF7);
        };

        auto draw_polygon_10_copy_1 = [&]() {
                tft.drawLine(146, 156, 147, 154, 0xFFFF);
                tft.drawLine(147, 154, 148, 156, 0xFFFF);
                tft.drawLine(148, 156, 147, 158, 0xFFFF);
                tft.drawLine(147, 158, 146, 156, 0xFFFF);
        };

        auto draw_polygon_10_copy_3 = [&]() {
                tft.drawLine(137, 153, 141, 149, 0xBDF7);
                tft.drawLine(141, 149, 144, 152, 0xBDF7);
                tft.drawLine(144, 152, 140, 156, 0xBDF7);
                tft.drawLine(140, 156, 137, 153, 0xBDF7);
        };

        auto draw_polygon_10_copy_13 = [&]() {
                tft.drawLine(200, 114, 206, 108, 0xBDF7);
                tft.drawLine(206, 108, 210, 112, 0xBDF7);
                tft.drawLine(210, 112, 204, 118, 0xBDF7);
                tft.drawLine(204, 118, 200, 114, 0xBDF7);
        };

        auto draw_polygon_10_copy_15 = [&]() {
                tft.drawLine(176, 136, 182, 130, 0xBDF7);
                tft.drawLine(182, 130, 186, 134, 0xBDF7);
                tft.drawLine(186, 134, 180, 140, 0xBDF7);
                tft.drawLine(180, 140, 176, 136, 0xBDF7);
        };

        auto draw_polygon_10_copy_12 = [&]() {
                tft.drawLine(176, 112, 180, 108, 0xBDF7);
                tft.drawLine(180, 108, 183, 111, 0xBDF7);
                tft.drawLine(183, 111, 179, 115, 0xBDF7);
                tft.drawLine(179, 115, 176, 112, 0xBDF7);
        };

        auto draw_polygon_10_copy_14 = [&]() {
                tft.drawLine(203, 137, 207, 133, 0xBDF7);
                tft.drawLine(207, 133, 210, 136, 0xBDF7);
                tft.drawLine(210, 136, 206, 140, 0xBDF7);
                tft.drawLine(206, 140, 203, 137, 0xBDF7);
        };

        auto draw_polygon_10_copy_10 = [&]() {
                tft.drawLine(164, 178, 168, 174, 0xBDF7);
                tft.drawLine(168, 174, 171, 177, 0xBDF7);
                tft.drawLine(171, 177, 167, 181, 0xBDF7);
                tft.drawLine(167, 181, 164, 178, 0xBDF7);
        };

        auto draw_polygon_10_copy_16 = [&]() {
                tft.drawLine(185, 115, 186, 113, 0xFFFF);
                tft.drawLine(186, 113, 187, 115, 0xFFFF);
                tft.drawLine(187, 115, 186, 117, 0xFFFF);
                tft.drawLine(186, 117, 185, 115, 0xFFFF);
        };

        auto draw_polygon_10_copy_18 = [&]() {
                tft.drawLine(124, 114, 130, 108, 0xBDF7);
                tft.drawLine(130, 108, 134, 112, 0xBDF7);
                tft.drawLine(134, 112, 128, 118, 0xBDF7);
                tft.drawLine(128, 118, 124, 114, 0xBDF7);
        };

        auto draw_polygon_10_copy_20 = [&]() {
                tft.drawLine(100, 136, 106, 130, 0xBDF7);
                tft.drawLine(106, 130, 110, 134, 0xBDF7);
                tft.drawLine(110, 134, 104, 140, 0xBDF7);
                tft.drawLine(104, 140, 100, 136, 0xBDF7);
        };

        auto draw_polygon_10_copy_17 = [&]() {
                tft.drawLine(100, 112, 104, 108, 0xBDF7);
                tft.drawLine(104, 108, 107, 111, 0xBDF7);
                tft.drawLine(107, 111, 103, 115, 0xBDF7);
                tft.drawLine(103, 115, 100, 112, 0xBDF7);
        };

        auto draw_polygon_10_copy_19 = [&]() {
                tft.drawLine(127, 137, 131, 133, 0xBDF7);
                tft.drawLine(131, 133, 134, 136, 0xBDF7);
                tft.drawLine(134, 136, 130, 140, 0xBDF7);
                tft.drawLine(130, 140, 127, 137, 0xBDF7);
        };

        auto draw_polygon_10_copy_21 = [&]() {
                tft.drawLine(109, 115, 110, 113, 0xFFFF);
                tft.drawLine(110, 113, 111, 115, 0xFFFF);
                tft.drawLine(111, 115, 110, 117, 0xFFFF);
                tft.drawLine(110, 117, 109, 115, 0xFFFF);
        };
        // polygon 10 copy 5
        draw_polygon_10_copy_5();
        // polygon 10 copy 11
        draw_polygon_10_copy_11();
        // ellipse 1 copy 1
        tft.fillCircle(154, 165, 18, 0x73AE);
        // rect 4 copy 5
        tft.fillRect(171, 162, 5, 7, 0xBDF7);
        // rect 4 copy 6
        tft.fillRect(151, 144, 7, 5, 0xBDF7);
        // polygon 10 copy 1
        draw_polygon_10_copy_1();
        // polygon 10 copy 3
        draw_polygon_10_copy_3();
        // triangle 15 copy 1
        tft.fillTriangle(141, 150, 138, 153, 144, 153, 0xBDF7);
        // ellipse 2 copy 1
        tft.fillCircle(154, 165, 4, 0xEF7D);
        // polygon 10 copy 13
        draw_polygon_10_copy_13();
        // string 19
        tft.setTextColor(0xF206);
        tft.setTextSize(4);
        tft.drawString("3", 145, 109);
        // polygon 10 copy 15
        draw_polygon_10_copy_15();
        // ellipse 1 copy 2
        tft.fillCircle(193, 124, 18, 0x73AE);
        // rect 4 copy 16
        tft.fillRect(210, 121, 5, 7, 0xBDF7);
        // rect 4 copy 17
        tft.fillRect(190, 103, 7, 5, 0xBDF7);
        // polygon 10 copy 12
        draw_polygon_10_copy_12();
        // triangle 15 copy 12
        tft.fillTriangle(180, 109, 177, 112, 183, 112, 0xBDF7);
        // ellipse 2 copy 2
        tft.fillCircle(193, 124, 4, 0xEF7D);
        // string 19 copy 1
        tft.setTextColor(0x8E09);
        tft.drawString("2", 107, 152);
        // rect 4 copy 18
        tft.fillRect(190, 141, 7, 5, 0xBDF7);
        // string 19 copy 2
        tft.drawString("2", 185, 152);
        // rect 4 copy 19
        tft.fillRect(172, 121, 5, 7, 0xBDF7);
        // polygon 10 copy 14
        draw_polygon_10_copy_14();
        // rect 4 copy 14
        tft.fillRect(151, 182, 7, 5, 0xBDF7);
        // rect 4 copy 15
        tft.fillRect(133, 162, 5, 7, 0xBDF7);
        // triangle 15 copy 13
        tft.fillTriangle(206, 109, 203, 112, 209, 112, 0xBDF7);
        // triangle 15 copy 14
        tft.fillTriangle(180, 133, 177, 136, 183, 136, 0xBDF7);
        // polygon 10 copy 10
        draw_polygon_10_copy_10();
        // triangle 15 copy 15
        tft.fillTriangle(206, 133, 203, 136, 209, 136, 0xBDF7);
        // triangle 19 copy 14
        tft.fillTriangle(180, 139, 182, 136, 178, 136, 0xBDF7);
        // triangle 15 copy 9
        tft.fillTriangle(167, 150, 164, 153, 170, 153, 0xBDF7);
        // triangle 19 copy 15
        tft.fillTriangle(206, 139, 208, 136, 204, 136, 0xBDF7);
        // triangle 15 copy 10
        tft.fillTriangle(141, 174, 138, 177, 144, 177, 0xBDF7);
        // triangle 19 copy 16
        tft.fillTriangle(206, 115, 208, 112, 204, 112, 0xBDF7);
        // triangle 15 copy 11
        tft.fillTriangle(167, 174, 164, 177, 170, 177, 0xBDF7);
        // triangle 19 copy 17
        tft.fillTriangle(179, 114, 181, 111, 177, 111, 0xBDF7);
        // triangle 19 copy 10
        tft.fillTriangle(141, 180, 143, 177, 139, 177, 0xBDF7);
        // polygon 10 copy 16
        draw_polygon_10_copy_16();
        // triangle 19 copy 11
        tft.fillTriangle(167, 180, 169, 177, 165, 177, 0xBDF7);
        // triangle 19 copy 12
        tft.fillTriangle(167, 156, 169, 153, 165, 153, 0xBDF7);
        // polygon 10 copy 18
        draw_polygon_10_copy_18();
        // triangle 19 copy 13
        tft.fillTriangle(140, 155, 142, 152, 138, 152, 0xBDF7);
        // polygon 10 copy 20
        draw_polygon_10_copy_20();
        // ellipse 1 copy 3
        tft.fillCircle(117, 124, 18, 0x73AE);
        // rect 4 copy 20
        tft.fillRect(134, 121, 5, 7, 0xBDF7);
        // rect 4 copy 21
        tft.fillRect(114, 103, 7, 5, 0xBDF7);
        // polygon 10 copy 17
        draw_polygon_10_copy_17();
        // triangle 15 copy 16
        tft.fillTriangle(104, 109, 101, 112, 107, 112, 0xBDF7);
        // ellipse 2 copy 3
        tft.fillCircle(117, 124, 4, 0xEF7D);
        // rect 4 copy 22
        tft.fillRect(114, 141, 7, 5, 0xBDF7);
        // rect 4 copy 23
        tft.fillRect(96, 121, 5, 7, 0xBDF7);
        // polygon 10 copy 19
        draw_polygon_10_copy_19();
        // triangle 15 copy 17
        tft.fillTriangle(130, 109, 127, 112, 133, 112, 0xBDF7);
        // triangle 15 copy 18
        tft.fillTriangle(104, 133, 101, 136, 107, 136, 0xBDF7);
        // triangle 15 copy 19
        tft.fillTriangle(130, 133, 127, 136, 133, 136, 0xBDF7);
        // triangle 19 copy 18
        tft.fillTriangle(104, 139, 106, 136, 102, 136, 0xBDF7);
        // triangle 19 copy 19
        tft.fillTriangle(130, 139, 132, 136, 128, 136, 0xBDF7);
        // triangle 19 copy 20
        tft.fillTriangle(130, 115, 132, 112, 128, 112, 0xBDF7);
        // triangle 19 copy 21
        tft.fillTriangle(103, 114, 105, 111, 101, 111, 0xBDF7);
        // polygon 10 copy 21
        draw_polygon_10_copy_21();
        // [END lopaka generated]
}
