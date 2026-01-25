#include "../common/platform/interface/platform.hpp"
#include "game_executor.hpp"
#include "game_menu.hpp"

#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "../common/constants.hpp"

#include "common_transitions.hpp"
#include "settings.hpp"
#include <optional>
#include "minesweeper.hpp"

#define TAG "minesweeper"

MinesweeperConfiguration DEFAULT_MINESWEEPER_CONFIG = {.mines_num = 25};

typedef struct MinesweeperGridDimensions {
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
} MinesweeperGridDimensions;

typedef struct MinesweeperGridCell {
        bool is_bomb;
        bool is_flagged;
        bool is_uncovered;
        int adjacent_bombs;

        MinesweeperGridCell()
            : is_bomb(false), is_flagged(false), is_uncovered(false),
              adjacent_bombs(0)
        {
        }

} MinesweeperGridCell;

static MinesweeperGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius);
static void draw_game_canvas(Platform *p, MinesweeperGridDimensions *dimensions,
                             UserInterfaceCustomization *customization);

static void erase_caret(Display *display, Point *grid_position,
                        MinesweeperGridDimensions *dimensions,
                        Color grid_background_color);
static void draw_caret(Display *display, Point *grid_position,
                       MinesweeperGridDimensions *dimensions);
/**
 * Performs a recursive uncovering waterfall: tries to uncover the current
 * cell, if the cell has 0 adjacent mines it uncovers all of its neighbours.
 */
static void uncover_grid_cells_starting_from(
    Display *display, Point *grid_position,
    MinesweeperGridDimensions *dimensions,
    std::vector<std::vector<MinesweeperGridCell>> *grid, int *total_uncovered);
static void
uncover_grid_cell(Display *display, Point *grid_position,
                  MinesweeperGridDimensions *dimensions,
                  std::vector<std::vector<MinesweeperGridCell>> *grid,
                  int *total_uncovered);
static void flag_grid_cell(Display *display, Point *grid_position,
                           MinesweeperGridDimensions *dimensions,
                           std::vector<std::vector<MinesweeperGridCell>> *grid,
                           UserInterfaceCustomization *customization);
static void
unflag_grid_cell(Display *display, Point *grid_position,
                 MinesweeperGridDimensions *dimensions,
                 std::vector<std::vector<MinesweeperGridCell>> *grid,
                 Color grid_background_color);

void place_bombs(std::vector<std::vector<MinesweeperGridCell>> *grid,
                 int bomb_number, Point *caret_position);

/**
 * Collects the minesweeper game configuration and either starts the game,
 * plays it and returns the user action to play again or returns early with
 * an 'interrrupt action' if the user requested help screen or exit.
 */
UserAction minesweeper_loop(Platform *platform,
                            UserInterfaceCustomization *customization);

std::optional<UserAction>
Minesweeper::game_loop(Platform *p, UserInterfaceCustomization *customization)
{
        const char *help_text =
            "Use the joystick to move the caret around the grid. Press green "
            "to uncover a cell, red to place a flag. The aim "
            "is to uncover all cells with no mines. Digits in the grid tell you"
            " the number of mines around the cell.";

        bool exit_requested = false;
        while (!exit_requested) {
                switch (minesweeper_loop(p, customization)) {
                case UserAction::PlayAgain:
                        LOG_DEBUG(TAG, "Minesweeper game loop finished. "
                                       "Pausing for input ");
                        Direction dir;
                        Action act;
                        pause_until_input(p->directional_controllers,
                                          p->action_controllers, &dir, &act,
                                          p->delay_provider, p->display);

                        if (act == Action::BLUE) {
                                exit_requested = true;
                        }
                        break;
                case UserAction::Exit:
                        exit_requested = true;
                        break;
                case UserAction::ShowHelp:
                        LOG_DEBUG(TAG,
                                  "User requested minesweeper help screen");
                        render_wrapped_help_text(p, customization, help_text);
                        wait_until_green_pressed(p);
                        break;
                case UserAction::CloseWindow:
                        return UserAction::CloseWindow;
                }
        }
        return std::nullopt;
}
UserAction minesweeper_loop(Platform *p,
                            UserInterfaceCustomization *customization)
{
        LOG_DEBUG(TAG, "Entering Minesweeper game loop");
        MinesweeperConfiguration config;

        auto maybe_interrupt =
            collect_minesweeper_config(p, &config, customization);

        if (maybe_interrupt) {
                return maybe_interrupt.value();
        }

        MinesweeperGridDimensions *gd = calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius());
        int rows = gd->rows;
        int cols = gd->cols;

        draw_game_canvas(p, gd, customization);
        LOG_DEBUG(TAG, "Minesweeper game canvas drawn.");

        if (!p->display->refresh()) {
                delete gd;
                return UserAction::CloseWindow;
        }

        std::vector<std::vector<MinesweeperGridCell>> grid(
            rows, std::vector<MinesweeperGridCell>(cols));

        /* We only place bombs after the user selects the cell to uncover.
           This avoids situations where the first selected cell is a bomb
           and the game is immediately over without user's logical error. */
        bool bombs_placed = false;

        Point caret_position = {.x = 0, .y = 0};
        draw_caret(p->display, &caret_position, gd);
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
                Direction dir;
                Action act;
                if (poll_directional_input(p->directional_controllers, &dir)) {
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
                        if (grid[caret_position.y][caret_position.x]
                                .is_uncovered) {
                                erase_caret(p->display, &caret_position, gd,
                                            Black);
                                // We need to 'uncover' the cell again to ensure
                                // that the numbers don't get cropped after the
                                // caret overlaps with them.
                                uncover_grid_cell(p->display, &caret_position,
                                                  gd, &grid, &total_uncovered);
                        } else if (grid[caret_position.y][caret_position.x]
                                       .is_flagged) {
                                erase_caret(p->display, &caret_position, gd,
                                            customization->accent_color);
                                // We need to unflag and flag the cell again to
                                // ensure that the flag indicator doesn't get
                                // cropped after the caret overlaps with them.
                                unflag_grid_cell(p->display, &caret_position,
                                                 gd, &grid,
                                                 customization->accent_color);
                                flag_grid_cell(p->display, &caret_position, gd,
                                               &grid, customization);
                        } else {
                                erase_caret(p->display, &caret_position, gd,
                                            customization->accent_color);
                        }
                        translate_within_bounds(&caret_position, dir, gd->rows,
                                                gd->cols);
                        draw_caret(p->display, &caret_position, gd);

                        p->delay_provider->delay_ms(MOVE_REGISTERED_DELAY);
                        /* We continue here to skip the additional input
                           polling delay at the end of the loop and make
                           the input snappy. */
                        continue;
                }
                if (poll_action_input(p->action_controllers, &act) &&
                    !action_input_on_last_iteration) {
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
                                                    p->display, &caret_position,
                                                    gd, &grid, customization);

                                        } else {
                                                unflag_grid_cell(
                                                    p->display, &caret_position,
                                                    gd, &grid,
                                                    customization
                                                        ->accent_color);
                                                draw_caret(p->display,
                                                           &caret_position, gd);
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
                                        place_bombs(&grid, config.mines_num,
                                                    &caret_position);
                                        bombs_placed = true;
                                        LOG_DEBUG(TAG, "Bombs placed.");
                                }
                                if (cell.is_bomb) {
                                        is_game_over = true;
                                }
                                if (!cell.is_flagged) {
                                        uncover_grid_cells_starting_from(
                                            p->display, &caret_position, gd,
                                            &grid, &total_uncovered);
                                }
                                break;
                        default:
                                LOG_DEBUG(TAG, "Irrelevant action input: %s",
                                          action_to_str(act));
                                break;
                        }
                        p->delay_provider->delay_ms(MOVE_REGISTERED_DELAY);
                        /* We continue here to skip the additional input
                           polling delay at the end of the loop and make
                           the input snappy. */
                        continue;
                } else {
                        action_input_on_last_iteration = false;
                }
                p->delay_provider->delay_ms(INPUT_POLLING_DELAY);
        }

        // When the game is lost, we make all bombs explode.
        if (is_game_over) {
                for (int y = 0; y < rows; y++) {
                        for (int x = 0; x < cols; x++) {
                                MinesweeperGridCell cell = grid[y][x];
                                if (cell.is_bomb) {
                                        Point point = {.x = x, .y = y};
                                        uncover_grid_cell(p->display, &point,
                                                          gd, &grid,
                                                          &total_uncovered);
                                }
                        }
                }

                pause_until_any_directional_input(
                    p->directional_controllers, p->delay_provider, p->display);
                display_game_over(p->display, customization);
                p->delay_provider->delay_ms(MOVE_REGISTERED_DELAY);
        } else {
                pause_until_any_directional_input(
                    p->directional_controllers, p->delay_provider, p->display);
                display_game_won(p->display, customization);
                p->delay_provider->delay_ms(MOVE_REGISTERED_DELAY);
        }
        if (!p->display->refresh()) {
                delete gd;
                return UserAction::CloseWindow;
        }
        return UserAction::PlayAgain;
}

void place_bombs(std::vector<std::vector<MinesweeperGridCell>> *grid,
                 int bomb_number, Point *caret_position)
{
        int rows = grid->size();
        int cols = (*grid->begin().base()).size();
        for (int i = 0; i < bomb_number; i++) {
                while (true) {
                        int x = rand() % cols;
                        int y = rand() % rows;

                        Point random_position = {.x = x, .y = y};

                        bool is_close_to_caret =
                            is_adjacent(caret_position, &random_position);
                        if (!(*grid)[y][x].is_bomb && !is_close_to_caret) {
                                (*grid)[y][x].is_bomb = true;
                                (*grid)[y][x].adjacent_bombs = 0;

                                Point current = {.x = x, .y = y};
                                for (Point nb : get_neighbours_inside_grid(
                                         &current, rows, cols)) {
                                        (*grid)[nb.y][nb.x].adjacent_bombs++;
                                }

                                break;
                        }
                }
        }
}

void erase_caret(Display *display, Point *grid_position,
                 MinesweeperGridDimensions *dimensions,
                 Color grid_background_color)
{
        // We need to ensure that the caret is rendered INSIDE the text
        // cell and its border doesn't overlap the neighbouring cells.
        // Otherwise, we'll get weird rendering artifacts where the border get
        // clipped.
        int border_offset = 1;
        Point actual_position = {
            .x = dimensions->left_horizontal_margin +
                 grid_position->x * FONT_WIDTH + border_offset,
            .y = dimensions->top_vertical_margin +
                 grid_position->y * FONT_SIZE + border_offset};

        display->draw_rectangle(actual_position, FONT_WIDTH - 2 * border_offset,
                                FONT_SIZE - 2 * border_offset,
                                grid_background_color, 1, false);
}

void draw_caret(Display *display, Point *grid_position,
                MinesweeperGridDimensions *dimensions)
{

        // We need to ensure that the caret is rendered INSIDE the text
        // cell and its border doesn't overlap the neighbouring cells.
        // Otherwise, we'll get weird rendering artifacts.
        int border_offset = 1;
        Point actual_position = {
            .x = dimensions->left_horizontal_margin +
                 grid_position->x * FONT_WIDTH + border_offset,
            .y = dimensions->top_vertical_margin +
                 grid_position->y * FONT_SIZE + border_offset};

        display->draw_rectangle(actual_position, FONT_WIDTH - 2 * border_offset,
                                FONT_SIZE - 2 * border_offset, White, 1, false);
}
void uncover_grid_cell(Display *display, Point *grid_position,
                       MinesweeperGridDimensions *dimensions,
                       std::vector<std::vector<MinesweeperGridCell>> *grid,
                       int *total_uncovered)
{

        Point actual_position = {.x = dimensions->left_horizontal_margin +
                                      grid_position->x * FONT_WIDTH,
                                 .y = dimensions->top_vertical_margin +
                                      grid_position->y * FONT_SIZE};

        char text[2];

        MinesweeperGridCell cell = (*grid)[grid_position->y][grid_position->x];
        // We need this check as we 're-uncover' cells after the caret
        // passes over them to remove rendering overlap artifacts.
        if (!cell.is_uncovered) {
                (*total_uncovered)++;
                (*grid)[grid_position->y][grid_position->x].is_uncovered = true;
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
        display->draw_rectangle(actual_position, FONT_WIDTH, FONT_SIZE, Black,
                                0, true);

        display->draw_string(actual_position, text, FontSize::Size16, Black,
                             text_color);
}

void uncover_grid_cells_starting_from(
    Display *display, Point *grid_position,
    MinesweeperGridDimensions *dimensions,
    std::vector<std::vector<MinesweeperGridCell>> *grid, int *total_uncovered)
{

        uncover_grid_cell(display, grid_position, dimensions, grid,
                          total_uncovered);

        int rows = grid->size();
        int cols = (*grid->begin().base()).size();
        MinesweeperGridCell current_cell =
            (*grid)[grid_position->y][grid_position->x];

        if (!current_cell.is_bomb && current_cell.adjacent_bombs == 0) {
                auto neighbours =
                    get_neighbours_inside_grid(grid_position, rows, cols);
                for (Point nb : neighbours) {
                        MinesweeperGridCell neighbour_cell =
                            (*grid)[nb.y][nb.x];

                        if (!neighbour_cell.is_uncovered &&
                            !neighbour_cell.is_flagged) {
                                uncover_grid_cells_starting_from(
                                    display, &nb, dimensions, grid,
                                    total_uncovered);
                        }
                }
        }
}

void flag_grid_cell(Display *display, Point *grid_position,
                    MinesweeperGridDimensions *dimensions,
                    std::vector<std::vector<MinesweeperGridCell>> *grid,
                    UserInterfaceCustomization *customization)
{

        (*grid)[grid_position->y][grid_position->x].is_flagged = true;
        Point actual_position = {.x = dimensions->left_horizontal_margin +
                                      grid_position->x * FONT_WIDTH,
                                 .y = dimensions->top_vertical_margin +
                                      grid_position->y * FONT_SIZE};

        {

                char text[2];
                sprintf(text, "f");
                display->draw_string(actual_position, text, FontSize::Size16,
                                     customization->accent_color, White);
        }
        if (false) {

                char text[2];
                sprintf(text, "*");
                display->draw_string(actual_position, text, FontSize::Size16,
                                     customization->accent_color, White);
        }
}

void unflag_grid_cell(Display *display, Point *grid_position,
                      MinesweeperGridDimensions *dimensions,
                      std::vector<std::vector<MinesweeperGridCell>> *grid,
                      Color grid_background_color)
{

        (*grid)[grid_position->y][grid_position->x].is_flagged = false;
        Point actual_position = {.x = dimensions->left_horizontal_margin +
                                      grid_position->x * FONT_WIDTH,
                                 .y = dimensions->top_vertical_margin +
                                      grid_position->y * FONT_SIZE};

        display->clear_region(actual_position,
                              {.x = actual_position.x + FONT_WIDTH,
                               .y = actual_position.y + FONT_SIZE},
                              grid_background_color);
}

Configuration *assemble_minesweeper_configuration(PersistentStorage *storage);
void extract_game_config(MinesweeperConfiguration *game_config,
                         Configuration *config);

std::optional<UserAction>
collect_minesweeper_config(Platform *p, MinesweeperConfiguration *game_config,
                           UserInterfaceCustomization *customization)
{
        Configuration *config =
            assemble_minesweeper_configuration(p->persistent_storage);

        auto maybe_interrupt = collect_configuration(p, config, customization);
        if (maybe_interrupt) {
                return maybe_interrupt;
        }

        extract_game_config(game_config, config);
        delete config;
        return std::nullopt;
}

MinesweeperConfiguration *
load_initial_minesweeper_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offset(Game::Minesweeper);
        LOG_DEBUG(TAG, "Loading minesweeper saved config from offset %d",
                  storage_offset);

        MinesweeperConfiguration config = {.mines_num = 0};

        LOG_DEBUG(
            TAG, "Trying to load initial settings from the persistent storage");
        storage->get(storage_offset, config);

        MinesweeperConfiguration *output = new MinesweeperConfiguration();

        if (config.mines_num == 0) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "minesweeper configuration, using default values.");
                memcpy(output, &DEFAULT_MINESWEEPER_CONFIG,
                       sizeof(MinesweeperConfiguration));
                storage->put(storage_offset, DEFAULT_MINESWEEPER_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(MinesweeperConfiguration));
        }

        LOG_DEBUG(TAG, "Loaded minesweeper game configuration: mines_num=%d",
                  output->mines_num);

        return output;
}

Configuration *assemble_minesweeper_configuration(PersistentStorage *storage)
{
        MinesweeperConfiguration *initial_config =
            load_initial_minesweeper_config(storage);

        ConfigurationOption *mines_count = ConfigurationOption::of_integers(
            "Number of mines", {10, 15, 25, 30, 35}, initial_config->mines_num);

        std::vector<ConfigurationOption *> options = {mines_count};

        return new Configuration("Minesweeper", options);
}

void extract_game_config(MinesweeperConfiguration *game_config,
                         Configuration *config)
{
        ConfigurationOption mines_num = *config->options[0];
        game_config->mines_num = mines_num.get_curr_int_value();
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

void draw_controls_hints(Display *display,
                         MinesweeperGridDimensions *dimensions,
                         int border_offset);
void draw_game_canvas(Platform *p, MinesweeperGridDimensions *dimensions,
                      UserInterfaceCustomization *customization)

{
        p->display->initialize();
        p->display->clear(Black);

        if (customization->rendering_mode == Detailed)
                p->display->draw_rounded_border(customization->accent_color);

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int actual_width = dimensions->actual_width;
        int actual_height = dimensions->actual_height;

        int border_width = 2;
        // We need to make the border rectangle and the canvas slightly
        // bigger to ensure that it does not overlap with the game area.
        // Otherwise the caret rendering erases parts of the border as
        // it moves around (as the caret intersects with the border
        // partially)
        int border_offset = 1;

        /* We don't draw the individual rectangles to make rendering
           faster on the physical Arduino LCD display. */
        p->display->clear_region(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            {.x = x_margin + actual_width + border_offset,
             .y = y_margin + actual_height + border_offset},
            customization->accent_color);

        p->display->draw_rectangle(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            actual_width + 2 * border_offset, actual_height + 2 * border_offset,
            Gray, border_width, false);

        if (customization->show_help_text) {
                draw_controls_hints(p->display, dimensions, border_offset);
        }
}

void draw_controls_hints(Display *display,
                         MinesweeperGridDimensions *dimensions,
                         int border_offset)
{
        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int actual_width = dimensions->actual_width;
        int actual_height = dimensions->actual_height;
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
        int available_width = display->get_width() - 2 * x_margin;
        int remainder_space = available_width - total_width;
        int even_separator = remainder_space / 3;

        int green_circle_x = x_margin + even_separator;
        display->draw_circle({.x = green_circle_x, .y = circle_y_axis}, r,
                             Green, 0, true);

        int select_text_x = green_circle_x + d;
        display->draw_string({.x = select_text_x, .y = text_below_grid_y},
                             (char *)select, FontSize::Size16, Black, White);

        int flag_red_circle_x = select_text_x + select_len + even_separator;
        display->draw_circle({.x = flag_red_circle_x, .y = circle_y_axis}, r,
                             Red, 0, true);
        int flag_text_x = flag_red_circle_x + d;
        display->draw_string({.x = flag_text_x, .y = text_below_grid_y},
                             (char *)flag, FontSize::Size16, Black, White);
}
