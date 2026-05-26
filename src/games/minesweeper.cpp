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
        int available_height =
            display.get_height() - display.get_display_corner_radius();
        display.clear_region({0, available_height / 2},
                             {display.get_width(), available_height}, Black);
        const char *subtitle = "Minesweeper";
        render_menu_subtitle(
            display, Configuration(subtitle, {new ConfigurationOption()}),
            false, strlen(subtitle), customization);
        TftCompatibleDisplay &tft =
            *platform.display->cast_into_tft_compatible();

        // [BEGIN lopaka generated]

        auto draw_polygon_10_copy_1 = [&]() {
                tft.drawLine(134, 153, 142, 145, 0xBDF7);
                tft.drawLine(142, 145, 147, 150, 0xBDF7);
                tft.drawLine(147, 150, 139, 158, 0xBDF7);
                tft.drawLine(139, 158, 134, 153, 0xBDF7);
        };

        auto draw_polygon_10_copy_3 = [&]() {
                tft.drawLine(161, 180, 169, 172, 0xBDF7);
                tft.drawLine(169, 172, 174, 177, 0xBDF7);
                tft.drawLine(174, 177, 166, 185, 0xBDF7);
                tft.drawLine(166, 185, 161, 180, 0xBDF7);
        };

        auto draw_polygon_10_copy_4 = [&]() {
                tft.drawLine(152, 159, 166, 145, 0xBDF7);
                tft.drawLine(166, 145, 174, 153, 0xBDF7);
                tft.drawLine(174, 153, 160, 167, 0xBDF7);
                tft.drawLine(160, 167, 152, 159, 0xBDF7);
        };

        auto draw_polygon_10_copy_5 = [&]() {
                tft.drawLine(134, 177, 148, 163, 0xBDF7);
                tft.drawLine(148, 163, 156, 171, 0xBDF7);
                tft.drawLine(156, 171, 142, 185, 0xBDF7);
                tft.drawLine(142, 185, 134, 177, 0xBDF7);
        };

        auto draw_polygon_10_copy_6 = [&]() {
                tft.drawLine(175, 113, 183, 105, 0xBDF7);
                tft.drawLine(183, 105, 188, 110, 0xBDF7);
                tft.drawLine(188, 110, 180, 118, 0xBDF7);
                tft.drawLine(180, 118, 175, 113, 0xBDF7);
        };

        auto draw_polygon_10_copy_7 = [&]() {
                tft.drawLine(202, 140, 210, 132, 0xBDF7);
                tft.drawLine(210, 132, 215, 137, 0xBDF7);
                tft.drawLine(215, 137, 207, 145, 0xBDF7);
                tft.drawLine(207, 145, 202, 140, 0xBDF7);
        };

        auto draw_polygon_10_copy_8 = [&]() {
                tft.drawLine(193, 119, 207, 105, 0xBDF7);
                tft.drawLine(207, 105, 215, 113, 0xBDF7);
                tft.drawLine(215, 113, 201, 127, 0xBDF7);
                tft.drawLine(201, 127, 193, 119, 0xBDF7);
        };

        auto draw_polygon_10_copy_9 = [&]() {
                tft.drawLine(175, 137, 189, 123, 0xBDF7);
                tft.drawLine(189, 123, 197, 131, 0xBDF7);
                tft.drawLine(197, 131, 183, 145, 0xBDF7);
                tft.drawLine(183, 145, 175, 137, 0xBDF7);
        };
        // ellipse 1 copy 1
        tft.fillCircle(154, 165, 16, 0xBDF7);
        // rect 4 copy 3
        tft.fillRect(130, 159, 9, 13, 0xBDF7);
        // rect 4 copy 5
        tft.fillRect(170, 159, 9, 13, 0xBDF7);
        // rect 4 copy 6
        tft.fillRect(148, 141, 13, 9, 0xBDF7);
        // rect 4 copy 7
        tft.fillRect(148, 181, 13, 9, 0xBDF7);
        // polygon 10 copy 1
        draw_polygon_10_copy_1();
        // polygon 10 copy 3
        draw_polygon_10_copy_3();
        // polygon 10 copy 4
        draw_polygon_10_copy_4();
        // polygon 10 copy 5
        draw_polygon_10_copy_5();
        // triangle 15 copy 1
        tft.fillTriangle(142, 145, 135, 153, 149, 153, 0xBDF7);
        // triangle 15 copy 2
        tft.fillTriangle(141, 170, 135, 177, 147, 177, 0xBDF7);
        // triangle 15 copy 3
        tft.fillTriangle(168, 170, 162, 177, 174, 177, 0xBDF7);
        // triangle 19 copy 4
        tft.fillTriangle(142, 184, 149, 176, 135, 176, 0xBDF7);
        // triangle 15 copy 4
        tft.fillTriangle(166, 145, 160, 152, 172, 152, 0xBDF7);
        // triangle 19 copy 1
        tft.fillTriangle(141, 158, 147, 153, 135, 153, 0xBDF7);
        // triangle 19 copy 2
        tft.fillTriangle(166, 161, 173, 153, 159, 153, 0xBDF7);
        // triangle 19 copy 3
        tft.fillTriangle(166, 184, 173, 176, 159, 176, 0xBDF7);
        // ellipse 2 copy 1
        tft.fillCircle(154, 165, 12, 0x0);
        // string 19
        tft.setTextColor(0xFFFF);
        tft.setTextSize(4);
        tft.drawString("1", 106, 154);
        // ellipse 1 copy 2
        tft.fillCircle(195, 125, 16, 0xBDF7);
        // rect 4 copy 8
        tft.fillRect(171, 119, 9, 13, 0xBDF7);
        // rect 4 copy 9
        tft.fillRect(211, 119, 9, 13, 0xBDF7);
        // rect 4 copy 10
        tft.fillRect(189, 101, 13, 9, 0xBDF7);
        // rect 4 copy 11
        tft.fillRect(189, 141, 13, 9, 0xBDF7);
        // polygon 10 copy 6
        draw_polygon_10_copy_6();
        // polygon 10 copy 7
        draw_polygon_10_copy_7();
        // polygon 10 copy 8
        draw_polygon_10_copy_8();
        // polygon 10 copy 9
        draw_polygon_10_copy_9();
        // triangle 15 copy 5
        tft.fillTriangle(183, 105, 176, 113, 190, 113, 0xBDF7);
        // triangle 15 copy 6
        tft.fillTriangle(182, 130, 176, 137, 188, 137, 0xBDF7);
        // triangle 15 copy 7
        tft.fillTriangle(209, 130, 203, 137, 215, 137, 0xBDF7);
        // triangle 15 copy 8
        tft.fillTriangle(207, 105, 201, 112, 213, 112, 0xBDF7);
        // triangle 19 copy 5
        tft.fillTriangle(182, 118, 188, 113, 176, 113, 0xBDF7);
        // triangle 19 copy 6
        tft.fillTriangle(207, 121, 214, 113, 200, 113, 0xBDF7);
        // triangle 19 copy 7
        tft.fillTriangle(207, 144, 214, 136, 200, 136, 0xBDF7);
        // triangle 19 copy 8
        tft.fillTriangle(183, 144, 190, 136, 176, 136, 0xBDF7);
        // ellipse 2 copy 2
        tft.fillCircle(195, 125, 12, 0x0);
        // string 19 copy 1
        tft.setTextColor(0x8E09);
        tft.drawString("2", 143, 107);
        // string 19 copy 2
        tft.drawString("2", 185, 154);
        // line 40
        tft.drawLine(0, 0, 0, 0, 0xFFFF);
        // rect 41
        tft.fillRect(109, 131, 15, 5, 0xFFFF);
        // rect 42
        tft.fillRect(114, 105, 5, 26, 0xFFFF);
        // triangle 43
        tft.fillTriangle(119, 105, 123, 123, 114, 123, 0xFFFF);
        // rect 41 copy 1
        tft.fillRect(147, 175, 15, 5, customization.accent_color);
        // rect 42 copy 1
        tft.fillRect(152, 149, 5, 26, customization.accent_color);
        // triangle 43 copy 1
        tft.fillTriangle(157, 149, 161, 167, 152, 167,
                         customization.accent_color);
        // [END lopaka generated]
}
