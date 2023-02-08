#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <stm32wbxx_ll_tim.h>
#include "tamalib/tamalib.h"
#include "tama.h"
#include "compiled/assets_icons.h"

TamaApp* g_ctx;
FuriMutex* g_state_mutex;
uint8_t layout_mode = 0; // 3: portrait => 4: portrait <=
// 0: landscape (small) 1: landscape (big) 2: landscape (full)
bool in_menu = false;

uint8_t speed = 1;
const uint8_t speed_options[] = {1, 2, 4};
const uint8_t min_speed = 1;
const uint8_t max_speed = 4;
const uint8_t speed_options_size = 3;
// = sizeof(speed_options) / sizeof(speed_options[0]);

uint8_t menu_cursor = 3; // 0: layout mode; 1: speed; 2: A+C;
const uint8_t menu_items = 4; // 3: Close menu, Save & Exit
uint8_t sub_menu_buttons = 0;
uint8_t sub_menu_last = 0;

static const Icon* icons_list[] = {
    &I_icon_0,
    &I_icon_1,
    &I_icon_2,
    &I_icon_3,
    &I_icon_4,
    &I_icon_5,
    &I_icon_6,
    &I_icon_7,
};

static InputKey m = InputKeyUp;
static InputKey a = InputKeyLeft;
static InputKey b = InputKeyDown;
static InputKey c = InputKeyRight;

static void speed_up() {
    switch(speed) {
    case max_speed:
        speed = speed_options[0];
        break;
    default:
        for(uint8_t i = 0; i < speed_options_size - 1; i++) {
            if(speed == speed_options[i]) {
                speed = speed_options[i + 1];
                break;
            }
        }
        break;
    }
    tamalib_set_speed(speed);
}
static void speed_down() {
    switch(speed) {
    case min_speed:
        speed = speed_options[speed_options_size - 1];
        break;
    default:
        for(uint8_t i = speed_options_size - 1; i > 0; i--) {
            if(speed == speed_options[i]) {
                speed = speed_options[i - 1];
                break;
            }
        }
        break;
    }
    tamalib_set_speed(speed);
}

// static void draw_landscape(Canvas* const canvas, void* cb_ctx)
static void draw_landscape(Canvas* const canvas, uint8_t scale) {
    // FURI_LOG_D(TAG, "Drawing frame");
    // Calculate positioning
    uint16_t canv_width = canvas_width(canvas);
    uint16_t canv_height = canvas_height(canvas);
    uint16_t lcd_matrix_scaled_width = 32 * scale;
    uint16_t lcd_matrix_scaled_height = 16 * scale;
    uint16_t lcd_matrix_top = (canv_height - lcd_matrix_scaled_height) / 2; // 0
    uint16_t lcd_matrix_left = (canv_width - lcd_matrix_scaled_width) / 2;

    // uint16_t lcd_icon_upper_top = lcd_matrix_top - TAMA_LCD_ICON_SIZE - TAMA_LCD_ICON_MARGIN;
    // uint16_t lcd_icon_lower_top = lcd_matrix_top + lcd_matrix_scaled_height + TAMA_LCD_ICON_MARGIN;
    uint16_t lcd_icon_upper_left = lcd_matrix_left;
    uint16_t lcd_icon_lower_left = lcd_matrix_left;
    uint16_t lcd_icon_spacing_horiz =
        (lcd_matrix_scaled_width - (4 * TAMA_LCD_ICON_SIZE)) / 3 + TAMA_LCD_ICON_SIZE;

    uint16_t y = lcd_matrix_top;
    for(uint8_t row = 0; row < 16; ++row) {
        uint16_t x = lcd_matrix_left;
        uint32_t row_pixels = g_ctx->framebuffer[row];
        for(uint8_t col = 0; col < 32; ++col) {
            if(row_pixels & 1) {
                canvas_draw_box(canvas, x, y, scale, scale);
            }
            x += scale;
            row_pixels >>= 1;
        }
        y += scale;
    }

    // Start drawing icons
    uint8_t lcd_icons = g_ctx->icons;

    // Draw top icons
    y = 0;
    uint16_t x_ic = lcd_icon_upper_left;
    for(uint8_t i = 0; i < 4; ++i) {
        if(lcd_icons & 1) {
            canvas_draw_icon(canvas, x_ic, y, icons_list[i]);
        }
        // x_ic += TAMA_LCD_ICON_SIZE + 4;
        // if(scale == 3) {
        //     y += 16;
        // } else {
        x_ic += lcd_icon_spacing_horiz;
        // }
        lcd_icons >>= 1;
    }

    // Draw bottom icons
    y = 64 - TAMA_LCD_ICON_SIZE;
    // if(scale == 3) {
    // y = 0;
    // x_ic = 128 - TAMA_LCD_ICON_SIZE;
    //     x_ic = 0;
    // } else {
    // y = 64 - TAMA_LCD_ICON_SIZE;
    x_ic = lcd_icon_lower_left;
    // }
    for(uint8_t i = 4; i < 8; ++i) {
        // canvas_draw_frame(canvas, x_ic, y, TAMA_LCD_ICON_SIZE, TAMA_LCD_ICON_SIZE);
        if(lcd_icons & 1) {
            canvas_draw_icon(canvas, x_ic, y, icons_list[i]);
        }
        // if(scale == 3) {
        //     y += 16;
        // } else {
        x_ic += lcd_icon_spacing_horiz;
        // }
        lcd_icons >>= 1;
    }
}
// static void draw_portrait_right(Canvas* const canvas, void* cb_ctx)
static void draw_portrait_right(Canvas* const canvas, uint8_t scale) {
    // FURI_LOG_D(TAG, "Drawing frame");
    // Calculate positioning
    // uint16_t canv_width = canvas_width(canvas);
    uint16_t canv_height = canvas_height(canvas);
    uint16_t lcd_matrix_scaled_width = 32 * scale;
    uint16_t lcd_matrix_scaled_height = 16 * scale;
    // uint16_t lcd_matrix_top = 0;
    uint16_t lcd_matrix_top = (canv_height - lcd_matrix_scaled_height) / 2;
    // uint16_t lcd_matrix_left = (canv_width - lcd_matrix_scaled_width) / 2;
    uint16_t lcd_matrix_left = 64 - TAMA_LCD_ICON_SIZE;
    uint16_t lcd_icon_upper_left = lcd_matrix_left;
    uint16_t lcd_icon_lower_left = lcd_matrix_left;
    uint16_t lcd_icon_spacing_horiz =
        (lcd_matrix_scaled_width - (4 * TAMA_LCD_ICON_SIZE)) / 3 + TAMA_LCD_ICON_SIZE;

    uint16_t y = lcd_matrix_top; // 64
    for(uint8_t row = 0; row < 16; ++row) {
        uint16_t x = 128; // lcd_matrix_left
        uint32_t row_pixels = g_ctx->framebuffer[row];
        for(uint8_t col = 0; col < 32; ++col) {
            if(row_pixels & 1) {
                canvas_draw_box(canvas, y + 32, x - 66, scale, scale);
            }
            x -= scale;
            row_pixels >>= 1;
        }
        y += scale;
    }

    // Start drawing icons
    uint8_t lcd_icons = g_ctx->icons;

    // Draw top icons
    // y = lcd_icon_upper_top;
    y = 30;
    // y = 64 - TAMA_LCD_ICON_SIZE;
    uint16_t x_ic = lcd_icon_upper_left;
    // uint16_t x_ic = 64 - TAMA_LCD_ICON_SIZE;
    for(uint8_t i = 0; i < 4; ++i) {
        if(lcd_icons & 1) {
            canvas_draw_icon(canvas, y, x_ic, icons_list[i]);
        }
        x_ic -= lcd_icon_spacing_horiz; // TAMA_LCD_ICON_SIZE + 4;
        lcd_icons >>= 1;
    }

    // Draw bottom icons
    y = 84; // lcd_icon_lower_top
    x_ic = lcd_icon_lower_left; // 64 - TAMA_LCD_ICON_SIZE
    for(uint8_t i = 4; i < 8; ++i) {
        // canvas_draw_frame(canvas, x_ic, y, TAMA_LCD_ICON_SIZE, TAMA_LCD_ICON_SIZE);
        if(lcd_icons & 1) {
            canvas_draw_icon(canvas, y, x_ic, icons_list[i]);
        }
        x_ic -= lcd_icon_spacing_horiz;
        lcd_icons >>= 1;
    }
}
static void draw_portrait_left(Canvas* const canvas, uint8_t scale) {
    // FURI_LOG_D(TAG, "Drawing frame");
    // Calculate positioning
    // uint16_t canv_width = canvas_width(canvas);
    // uint16_t canv_height = canvas_height(canvas);
    uint16_t lcd_matrix_scaled_width = 32 * scale;
    // uint16_t lcd_matrix_scaled_height = 16 * scale;
    // uint16_t lcd_matrix_top = 0;
    // uint16_t lcd_matrix_top = (canv_height - lcd_matrix_scaled_height) / 2;
    // uint16_t lcd_matrix_left = (canv_width - lcd_matrix_scaled_width) / 2;
    uint16_t lcd_matrix_left = 0;
    uint16_t lcd_icon_upper_left = lcd_matrix_left;
    uint16_t lcd_icon_lower_left = lcd_matrix_left;
    uint16_t lcd_icon_spacing_horiz =
        (lcd_matrix_scaled_width - (4 * TAMA_LCD_ICON_SIZE)) / 3 + TAMA_LCD_ICON_SIZE;

    // uint16_t y = 64 + lcd_matrix_top;
    uint16_t y = 64;
    for(uint8_t row = 0; row < 16; ++row) {
        uint16_t x = 0; // lcd_matrix_left
        uint32_t row_pixels = g_ctx->framebuffer[row];
        for(uint8_t col = 0; col < 32; ++col) {
            if(row_pixels & 1) {
                canvas_draw_box(canvas, y, x, scale, scale);
            }
            x += scale;
            row_pixels >>= 1;
        }
        y -= scale;
    }

    // Start drawing icons
    uint8_t lcd_icons = g_ctx->icons;

    // Draw top icons
    // y = lcd_icon_upper_top;
    y = 70;
    // y = 64 - TAMA_LCD_ICON_SIZE;
    uint16_t x_ic = lcd_icon_upper_left;
    // uint16_t x_ic = 64 - TAMA_LCD_ICON_SIZE;
    for(uint8_t i = 0; i < 4; ++i) {
        if(lcd_icons & 1) {
            canvas_draw_icon(canvas, y, x_ic, icons_list[i]);
        }
        x_ic += lcd_icon_spacing_horiz; // TAMA_LCD_ICON_SIZE + 4;
        lcd_icons >>= 1;
    }

    // Draw bottom icons
    y = 16; // lcd_icon_lower_top
    x_ic = lcd_icon_lower_left; // 64 - TAMA_LCD_ICON_SIZE
    for(uint8_t i = 4; i < 8; ++i) {
        // canvas_draw_frame(canvas, x_ic, y, TAMA_LCD_ICON_SIZE, TAMA_LCD_ICON_SIZE);
        if(lcd_icons & 1) {
            canvas_draw_icon(canvas, y, x_ic, icons_list[i]);
        }
        x_ic += lcd_icon_spacing_horiz;
        lcd_icons >>= 1;
    }
}
// static void draw_mini(Canvas* const canvas, uint16_t inX, uint16_t inY)
static void draw_mini(Canvas* const canvas) {
    // Calculate positioning
    // uint16_t y = inY;
    const uint16_t y = 34;
    const uint16_t x = 84;

    uint16_t y1 = y;
    for(uint8_t row = 0; row < 16; ++row) {
        // uint16_t x = inX;
        uint16_t x1 = x;
        uint32_t row_pixels = g_ctx->framebuffer[row];
        for(uint8_t col = 0; col < 32; ++col) {
            if(row_pixels & 1) {
                canvas_draw_dot(canvas, x1, y1);
            }
            x1 += 1;
            row_pixels >>= 1;
        }
        y1 += 1;
    }

    // Start drawing icons
    uint8_t lcd_icons = g_ctx->icons;

    // Draw top icons
    uint16_t y2 = y - 2;
    uint16_t x_ic = x;
    for(uint8_t i = 0; i < 4; ++i) {
        if(lcd_icons & 1) {
            // canvas_draw_icon(canvas, x_ic, y, icons_list[i]);
            canvas_draw_line(canvas, x_ic, y2, x_ic + 6, y2);
        }
        x_ic += 8;
        lcd_icons >>= 1;
    }

    // Draw bottom icons
    y2 = y + 17;
    x_ic = x;
    for(uint8_t i = 4; i < 8; ++i) {
        // canvas_draw_frame(canvas, x_ic, y, TAMA_LCD_ICON_SIZE, TAMA_LCD_ICON_SIZE);
        if(lcd_icons & 1) {
            // canvas_draw_icon(canvas, x_ic, y, icons_list[i]);
            canvas_draw_line(canvas, x_ic, y2, x_ic + 6, y2);
        }
        x_ic += 8;
        lcd_icons >>= 1;
    }
}

static void draw_menu(Canvas* const canvas) {
    canvas_draw_frame(canvas, 0, 0, 128, 64);
    canvas_draw_str_aligned(canvas, 64, 6, AlignCenter, AlignCenter, "Menu");
    canvas_draw_line(canvas, 0, 10, 128, 10);
    draw_mini(canvas);
    // draw_mini(canvas, 34, 84);

    switch(menu_cursor) {
    case 0:
        canvas_draw_triangle(canvas, 4, 16, 6, 6, CanvasDirectionLeftToRight);
        break;
    case 1:
        canvas_draw_triangle(canvas, 4, 26, 6, 6, CanvasDirectionLeftToRight);
        break;
    case 2:
        switch(sub_menu_buttons) {
        case 0:
            canvas_draw_triangle(canvas, 4, 36, 6, 6, CanvasDirectionLeftToRight);
            break;
        case 1:
            canvas_draw_triangle(canvas, 47, 44, 6, 4, CanvasDirectionBottomToTop);
            break;
        case 2:
            canvas_draw_triangle(canvas, 57, 44, 6, 4, CanvasDirectionBottomToTop);
            break;
        case 3:
            canvas_draw_triangle(canvas, 67, 44, 6, 4, CanvasDirectionBottomToTop);
            break;
        default:
            break;
        }
        break;
    case menu_items - 1:
        switch(sub_menu_last) {
        case 0:
            canvas_draw_triangle(canvas, 4, 56, 6, 6, CanvasDirectionLeftToRight);
            break;
        case 1:
            canvas_draw_triangle(canvas, 36, 56, 6, 6, CanvasDirectionLeftToRight);
            break;
        case 2:
            canvas_draw_triangle(canvas, 67, 56, 6, 6, CanvasDirectionLeftToRight);
            break;
        default:
            break;
        }
        break;
    }
    switch(layout_mode) {
    case 0:
        canvas_draw_str(canvas, 12, 20, "Layout: Landscape (small)");
        break;
    case 1:
        canvas_draw_str(canvas, 12, 20, "Layout: Landscape (big)");
        break;
    case 2:
        canvas_draw_str(canvas, 12, 20, "Layout: Landscape (full)");
        break;
    case 3:
        canvas_draw_str(canvas, 12, 20, "Layout: Portrait =>");
        break;
    case 4:
        canvas_draw_str(canvas, 12, 20, "Layout: Portrait <=");
        break;
    default:
        canvas_draw_str(canvas, 12, 20, "Layout: ???");
        break;
    }
    switch(speed) { // match with speed_options
    // case 0: // freeze menu too
    //     canvas_draw_str(canvas, 12, 30, "Speed: 0x");
    //     break;
    case 1:
        canvas_draw_str(canvas, 12, 30, "Speed: 1x");
        break;
    case 2:
        canvas_draw_str(canvas, 12, 30, "Speed:  2x");
        break;
    case 4:
        canvas_draw_str(canvas, 12, 30, "Speed:   4x (max)");
        break;
    default:
        canvas_draw_str(canvas, 12, 30, "Speed ??x");
        break;
    }
    canvas_draw_str(canvas, 12, 40, "A+C");
    canvas_draw_str(canvas, 45, 40, "A");
    canvas_draw_str(canvas, 55, 40, "B");
    canvas_draw_str(canvas, 65, 40, "C");

    canvas_draw_str(canvas, 12, 60, "Close");
    canvas_draw_str(canvas, 44, 60, "Save");
    canvas_draw_str(canvas, 75, 60, "Save & Exit");
}

static void tama_p1_draw_callback(Canvas* const canvas, void* cb_ctx) {
    furi_assert(cb_ctx);

    FuriMutex* const mutex = cb_ctx;
    if(furi_mutex_acquire(mutex, 25) != FuriStatusOk) return;

    if(g_ctx->rom == NULL) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 30, 30, "No ROM");
    } else if(g_ctx->halted) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 30, 30, "Halted");
    } else {
        if(in_menu) {
            // switch(layout_mode)
            // draw_menu_landscape(canvas);
            draw_menu(canvas);
        } else {
            switch(layout_mode) {
            case 0:
                draw_landscape(canvas, TAMA_SCREEN_SCALE_FACTOR); // 2
                break;
            case 1:
                draw_landscape(canvas, 3);
                break;
            case 2:
                draw_landscape(canvas, 4);
                break;
            case 3:
                draw_portrait_right(canvas, TAMA_SCREEN_SCALE_FACTOR);
                break;
            case 4:
                draw_portrait_left(canvas, TAMA_SCREEN_SCALE_FACTOR);
                break;
            default:
                break;
            }
        }
    }
    furi_mutex_release(mutex);
}

static void tama_p1_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    TamaEvent event = {.type = EventTypeInput, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void tama_p1_update_timer_callback(FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    TamaEvent event = {.type = EventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}

static void tama_p1_load_state() {
    state_t* state;
    uint8_t buf[4];
    bool error = false;
    state = tamalib_get_state();

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, TAMA_SAVE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_read(file, &buf, 4);
        if(buf[0] != (uint8_t)STATE_FILE_MAGIC[0] || buf[1] != (uint8_t)STATE_FILE_MAGIC[1] ||
           buf[2] != (uint8_t)STATE_FILE_MAGIC[2] || buf[3] != (uint8_t)STATE_FILE_MAGIC[3]) {
            FURI_LOG_E(TAG, "FATAL: Wrong state file magic in \"%s\" !\n", TAMA_SAVE_PATH);
            error = true;
        }

        storage_file_read(file, &buf, 1);
        if(buf[0] != STATE_FILE_VERSION) {
            FURI_LOG_E(TAG, "FATAL: Unsupported version");
            error = true;
        }
        if(!error) {
            FURI_LOG_D(TAG, "Reading save.bin");

            storage_file_read(file, &buf, 2);
            *(state->pc) = buf[0] | ((buf[1] & 0x1F) << 8);

            storage_file_read(file, &buf, 2);
            *(state->x) = buf[0] | ((buf[1] & 0xF) << 8);

            storage_file_read(file, &buf, 2);
            *(state->y) = buf[0] | ((buf[1] & 0xF) << 8);

            storage_file_read(file, &buf, 1);
            *(state->a) = buf[0] & 0xF;

            storage_file_read(file, &buf, 1);
            *(state->b) = buf[0] & 0xF;

            storage_file_read(file, &buf, 1);
            *(state->np) = buf[0] & 0x1F;

            storage_file_read(file, &buf, 1);
            *(state->sp) = buf[0];

            storage_file_read(file, &buf, 1);
            *(state->flags) = buf[0] & 0xF;

            storage_file_read(file, &buf, 4);
            *(state->tick_counter) = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

            storage_file_read(file, &buf, 4);
            *(state->clk_timer_timestamp) = buf[0] | (buf[1] << 8) | (buf[2] << 16) |
                                            (buf[3] << 24);

            storage_file_read(file, &buf, 4);
            *(state->prog_timer_timestamp) = buf[0] | (buf[1] << 8) | (buf[2] << 16) |
                                             (buf[3] << 24);

            storage_file_read(file, &buf, 1);
            *(state->prog_timer_enabled) = buf[0] & 0x1;

            storage_file_read(file, &buf, 1);
            *(state->prog_timer_data) = buf[0];

            storage_file_read(file, &buf, 1);
            *(state->prog_timer_rld) = buf[0];

            storage_file_read(file, &buf, 4);
            *(state->call_depth) = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

            FURI_LOG_D(TAG, "Restoring Interupts");
            for(uint32_t i = 0; i < INT_SLOT_NUM; i++) {
                storage_file_read(file, &buf, 1);
                state->interrupts[i].factor_flag_reg = buf[0] & 0xF;

                storage_file_read(file, &buf, 1);
                state->interrupts[i].mask_reg = buf[0] & 0xF;

                storage_file_read(file, &buf, 1);
                state->interrupts[i].triggered = buf[0] & 0x1;
            }

            /* First 640 half bytes correspond to the RAM */
            FURI_LOG_D(TAG, "Restoring RAM");
            for(uint32_t i = 0; i < MEM_RAM_SIZE; i++) {
                storage_file_read(file, &buf, 1);
                SET_RAM_MEMORY(state->memory, i + MEM_RAM_ADDR, buf[0] & 0xF);
            }

            /* I/Os are from 0xF00 to 0xF7F */
            FURI_LOG_D(TAG, "Restoring I/O");
            for(uint32_t i = 0; i < MEM_IO_SIZE; i++) {
                storage_file_read(file, &buf, 1);
                SET_IO_MEMORY(state->memory, i + MEM_IO_ADDR, buf[0] & 0xF);
            }
            FURI_LOG_D(TAG, "Refreshing Hardware");
            tamalib_refresh_hw();
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static void tama_p1_save_state() {
    // Saving state
    FURI_LOG_D(TAG, "Saving Gamestate");
    uint8_t buf[4];
    state_t* state;
    uint32_t offset = 0;
    state = tamalib_get_state();

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, TAMA_SAVE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        buf[0] = (uint8_t)STATE_FILE_MAGIC[0];
        buf[1] = (uint8_t)STATE_FILE_MAGIC[1];
        buf[2] = (uint8_t)STATE_FILE_MAGIC[2];
        buf[3] = (uint8_t)STATE_FILE_MAGIC[3];
        offset += storage_file_write(file, &buf, sizeof(buf));

        buf[0] = STATE_FILE_VERSION & 0xFF;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->pc) & 0xFF;
        buf[1] = (*(state->pc) >> 8) & 0x1F;
        offset += storage_file_write(file, &buf, 2);

        buf[0] = *(state->x) & 0xFF;
        buf[1] = (*(state->x) >> 8) & 0xF;
        offset += storage_file_write(file, &buf, 2);

        buf[0] = *(state->y) & 0xFF;
        buf[1] = (*(state->y) >> 8) & 0xF;
        offset += storage_file_write(file, &buf, 2);

        buf[0] = *(state->a) & 0xF;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->b) & 0xF;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->np) & 0x1F;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->sp) & 0xFF;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->flags) & 0xF;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->tick_counter) & 0xFF;
        buf[1] = (*(state->tick_counter) >> 8) & 0xFF;
        buf[2] = (*(state->tick_counter) >> 16) & 0xFF;
        buf[3] = (*(state->tick_counter) >> 24) & 0xFF;
        offset += storage_file_write(file, &buf, sizeof(buf));

        buf[0] = *(state->clk_timer_timestamp) & 0xFF;
        buf[1] = (*(state->clk_timer_timestamp) >> 8) & 0xFF;
        buf[2] = (*(state->clk_timer_timestamp) >> 16) & 0xFF;
        buf[3] = (*(state->clk_timer_timestamp) >> 24) & 0xFF;
        offset += storage_file_write(file, &buf, sizeof(buf));

        buf[0] = *(state->prog_timer_timestamp) & 0xFF;
        buf[1] = (*(state->prog_timer_timestamp) >> 8) & 0xFF;
        buf[2] = (*(state->prog_timer_timestamp) >> 16) & 0xFF;
        buf[3] = (*(state->prog_timer_timestamp) >> 24) & 0xFF;
        offset += storage_file_write(file, &buf, sizeof(buf));

        buf[0] = *(state->prog_timer_enabled) & 0x1;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->prog_timer_data) & 0xFF;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->prog_timer_rld) & 0xFF;
        offset += storage_file_write(file, &buf, 1);

        buf[0] = *(state->call_depth) & 0xFF;
        buf[1] = (*(state->call_depth) >> 8) & 0xFF;
        buf[2] = (*(state->call_depth) >> 16) & 0xFF;
        buf[3] = (*(state->call_depth) >> 24) & 0xFF;
        offset += storage_file_write(file, &buf, sizeof(buf));

        for(uint32_t i = 0; i < INT_SLOT_NUM; i++) {
            buf[0] = state->interrupts[i].factor_flag_reg & 0xF;
            offset += storage_file_write(file, &buf, 1);

            buf[0] = state->interrupts[i].mask_reg & 0xF;
            offset += storage_file_write(file, &buf, 1);

            buf[0] = state->interrupts[i].triggered & 0x1;
            offset += storage_file_write(file, &buf, 1);
        }

        /* First 640 half bytes correspond to the RAM */
        for(uint32_t i = 0; i < MEM_RAM_SIZE; i++) {
            buf[0] = GET_RAM_MEMORY(state->memory, i + MEM_RAM_ADDR) & 0xF;
            offset += storage_file_write(file, &buf, 1);
        }

        /* I/Os are from 0xF00 to 0xF7F */
        for(uint32_t i = 0; i < MEM_IO_SIZE; i++) {
            buf[0] = GET_IO_MEMORY(state->memory, i + MEM_IO_ADDR) & 0xF;
            offset += storage_file_write(file, &buf, 1);
        }
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_D(TAG, "Finished Writing %lu", offset);
}

static int32_t tama_p1_worker(void* context) {
    bool running = true;
    FuriMutex* mutex = context;
    while(furi_mutex_acquire(mutex, FuriWaitForever) != FuriStatusOk) furi_delay_tick(1);

    cpu_sync_ref_timestamp();
    LL_TIM_EnableCounter(TIM2);

    tama_p1_load_state();

    while(running) {
        if(furi_thread_flags_get()) {
            running = false;
        } else {
            // FURI_LOG_D(TAG, "Stepping"); // enabling this cause blank screen somehow
            // for (int i = 0; i < 100; ++i)
            tamalib_step(); // tamalib_mainloop();
        }
    }
    LL_TIM_DisableCounter(TIM2);
    furi_mutex_release(mutex);
    return 0;
}

static void tama_p1_init(TamaApp* const ctx) {
    g_ctx = ctx;
    memset(ctx, 0, sizeof(TamaApp));
    tama_p1_hal_init(&ctx->hal);

    // Load ROM
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FileInfo fi;
    if(storage_common_stat(storage, TAMA_ROM_PATH, &fi) == FSE_OK) {
        File* rom_file = storage_file_alloc(storage);
        if(storage_file_open(rom_file, TAMA_ROM_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
            ctx->rom = malloc((size_t)fi.size);
            uint8_t* buf_ptr = ctx->rom;
            size_t read = 0;
            while(read < fi.size) {
                size_t to_read = fi.size - read;
                if(to_read > UINT16_MAX) to_read = UINT16_MAX;
                uint16_t now_read = storage_file_read(rom_file, buf_ptr, (uint16_t)to_read);
                read += now_read;
                buf_ptr += now_read;
            }

            // Reorder endianess of ROM
            for(size_t i = 0; i < fi.size; i += 2) {
                uint8_t b = ctx->rom[i];
                ctx->rom[i] = ctx->rom[i + 1];
                ctx->rom[i + 1] = b & 0xF;
            }
        }

        storage_file_close(rom_file);
        storage_file_free(rom_file);
    }
    furi_record_close(RECORD_STORAGE);

    if(ctx->rom != NULL) {
        // Init TIM2
        // 64KHz
        LL_TIM_InitTypeDef tim_init = {
            .Prescaler = 999,
            .CounterMode = LL_TIM_COUNTERMODE_UP,
            .Autoreload = 0xFFFFFFFF,
        };
        LL_TIM_Init(TIM2, &tim_init);
        LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
        LL_TIM_DisableCounter(TIM2);
        LL_TIM_SetCounter(TIM2, 0);

        // Init TamaLIB
        tamalib_register_hal(&ctx->hal);
        tamalib_init((u12_t*)ctx->rom, NULL, 64000);
        tamalib_set_speed(speed);

        // TODO: implement fast forwarding
        // ctx->fast_forward_done = true;

        // Start stepping thread
        ctx->thread = furi_thread_alloc();
        furi_thread_set_name(ctx->thread, "TamaLIB");
        furi_thread_set_stack_size(ctx->thread, 2 * 1024);
        furi_thread_set_callback(ctx->thread, tama_p1_worker);
        furi_thread_set_context(ctx->thread, g_state_mutex);
        furi_thread_start(ctx->thread);
    }
}

static void tama_p1_deinit(TamaApp* const ctx) {
    if(ctx->rom != NULL) {
        tamalib_release();
        furi_thread_free(ctx->thread);
        free(ctx->rom);
    }
}

int32_t tama_p1_app(void* p) {
    UNUSED(p);

    TamaApp* ctx = malloc(sizeof(TamaApp));
    g_state_mutex = furi_mutex_alloc(FuriMutexTypeRecursive);
    tama_p1_init(ctx);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(TamaEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, tama_p1_draw_callback, g_state_mutex);
    view_port_input_callback_set(view_port, tama_p1_input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    FuriTimer* timer =
        furi_timer_alloc(tama_p1_update_timer_callback, FuriTimerTypePeriodic, event_queue);
    furi_timer_start(timer, furi_kernel_get_tick_frequency() / 30);

    // in_menu = false;
    // menu_cursor = 2;

    for(bool running = true; running;) {
        TamaEvent event;
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, FuriWaitForever);
        if(event_status == FuriStatusOk) {
            // Local override with acquired context
            if(furi_mutex_acquire(g_state_mutex, FuriWaitForever) != FuriStatusOk) continue;

            if(event.type == EventTypeTick) {
                // FURI_LOG_D(TAG, "EventTypeTick");
                view_port_update(view_port);
            } else if(event.type == EventTypeInput) {
                FURI_LOG_D(
                    TAG,
                    "EventTypeInput: %ld %d %d",
                    event.input.sequence,
                    event.input.key,
                    event.input.type);
                // InputType input_type = event.input.type; // idk why this is a variable
                btn_state_t tama_btn_state = 0; // BTN_STATE_RELEASED is 0

                if(in_menu) {
                    // if(menu_cursor >= 2 &&
                    //    (event.input.key == InputKeyUp || event.input.key == InputKeyDown)) {
                    //     tama_btn_state = BTN_STATE_RELEASED;
                    // }
                    if(event.input.key == InputKeyBack) {
                        tama_btn_state = BTN_STATE_RELEASED;
                        in_menu = false;
                    } else if(event.input.key == InputKeyUp && event.input.type == InputTypePress) {
                        tama_btn_state = BTN_STATE_RELEASED;
                        if(menu_cursor > 0) {
                            menu_cursor -= 1;
                        } else {
                            menu_cursor = menu_items - 1;
                        }
                        if(menu_cursor >= menu_items - 2 && sub_menu_last > 0) {
                            sub_menu_buttons = 1;
                            sub_menu_last = 1;
                        } else {
                            sub_menu_buttons = 0;
                            sub_menu_last = 0;
                        }
                    } else if(event.input.key == InputKeyDown && event.input.type == InputTypePress) {
                        tama_btn_state = BTN_STATE_RELEASED;
                        if(menu_cursor < menu_items - 1) {
                            menu_cursor += 1;
                        } else {
                            menu_cursor = 0;
                        }
                        if(menu_cursor >= menu_items - 2 && sub_menu_buttons > 0) {
                            sub_menu_buttons = 1;
                            sub_menu_last = 1;
                        } else {
                            sub_menu_buttons = 0;
                            sub_menu_last = 0;
                        }
                    } else if(event.input.key == InputKeyLeft && event.input.type == InputTypePress) {
                        switch(menu_cursor) {
                        case 0:
                            switch(layout_mode) {
                            case 0:
                                layout_mode = 4;
                                m = InputKeyRight;
                                a = InputKeyUp;
                                b = InputKeyLeft;
                                c = InputKeyDown;
                                break;
                            case 1:
                                layout_mode -= 1; // 0
                                m = InputKeyUp;
                                a = InputKeyLeft;
                                b = InputKeyDown;
                                c = InputKeyRight;
                                break;
                            case 2:
                                layout_mode -= 1; // 1
                                m = InputKeyUp;
                                a = InputKeyLeft;
                                b = InputKeyDown;
                                c = InputKeyRight;
                                break;
                            case 3:
                                layout_mode -= 1; // 2
                                m = InputKeyUp;
                                a = InputKeyLeft;
                                b = InputKeyDown;
                                c = InputKeyRight;
                                break;
                            case 4:
                                layout_mode -= 1; // 3
                                m = InputKeyLeft;
                                a = InputKeyDown;
                                b = InputKeyRight;
                                c = InputKeyUp;
                                break;
                            }
                            break;
                        case 1:
                            speed_down();
                            break;
                        case 2:
                            tama_btn_state = BTN_STATE_RELEASED;
                            switch(sub_menu_buttons) {
                            case 0:
                                sub_menu_buttons = 3;
                                break;
                            case 1:
                            case 2:
                            case 3:
                                sub_menu_buttons -= 1;
                                break;
                            default:
                                break;
                            }
                            break;
                        case menu_items - 1:
                            switch(sub_menu_last) {
                            case 0:
                                sub_menu_last = 2;
                                break;
                            case 1:
                            case 2:
                                sub_menu_last -= 1;
                                break;
                            default:
                                break;
                            }
                            break;
                        default:
                            break;
                        }
                    } else if(event.input.key == InputKeyRight && event.input.type == InputTypePress) {
                        switch(menu_cursor) {
                        case 0:
                            switch(layout_mode) {
                            case 0:
                                layout_mode += 1; // 1
                                m = InputKeyUp;
                                a = InputKeyLeft;
                                b = InputKeyDown;
                                c = InputKeyRight;
                                break;
                            case 1:
                                layout_mode += 1; // 2
                                m = InputKeyUp;
                                a = InputKeyLeft;
                                b = InputKeyDown;
                                c = InputKeyRight;
                                break;
                            case 2:
                                layout_mode += 1; // 3
                                m = InputKeyLeft;
                                a = InputKeyDown;
                                b = InputKeyRight;
                                c = InputKeyUp;
                                break;
                            case 3:
                                layout_mode += 1; // 4
                                m = InputKeyRight;
                                a = InputKeyUp;
                                b = InputKeyLeft;
                                c = InputKeyDown;
                                break;
                            case 4:
                                layout_mode = 0;
                                m = InputKeyUp;
                                a = InputKeyLeft;
                                b = InputKeyDown;
                                c = InputKeyRight;
                                break;
                            }
                            break;
                        case 1:
                            speed_up();
                            break;
                        case 2:
                            tama_btn_state = BTN_STATE_RELEASED;
                            switch(sub_menu_buttons) {
                            case 0:
                            case 1:
                            case 2:
                                sub_menu_buttons += 1;
                                break;
                            case 3:
                                sub_menu_buttons = 0;
                                break;
                            default:
                                break;
                            }
                            break;
                        case menu_items - 1:
                            switch(sub_menu_last) {
                            case 0:
                            case 1:
                                sub_menu_last += 1;
                                break;
                            case 2:
                                sub_menu_last = 0;
                                break;
                            default:
                                break;
                            }
                            break;
                        default:
                            break;
                        }
                    } else if(event.input.key == InputKeyOk) {
                        switch(menu_cursor) {
                        case 0:
                            if(event.input.type == InputTypePress) {
                                switch(layout_mode) {
                                case 0:
                                    layout_mode += 1; // 1
                                    m = InputKeyUp;
                                    a = InputKeyLeft;
                                    b = InputKeyDown;
                                    c = InputKeyRight;
                                    break;
                                case 1:
                                    layout_mode += 1; // 2
                                    m = InputKeyUp;
                                    a = InputKeyLeft;
                                    b = InputKeyDown;
                                    c = InputKeyRight;
                                    break;
                                case 2:
                                    layout_mode += 1; // 3
                                    m = InputKeyLeft;
                                    a = InputKeyDown;
                                    b = InputKeyRight;
                                    c = InputKeyUp;
                                    break;
                                case 3:
                                    layout_mode += 1; // 4
                                    m = InputKeyRight;
                                    a = InputKeyUp;
                                    b = InputKeyLeft;
                                    c = InputKeyDown;
                                    break;
                                case 4:
                                    layout_mode = 0;
                                    m = InputKeyUp;
                                    a = InputKeyLeft;
                                    b = InputKeyDown;
                                    c = InputKeyRight;
                                    break;
                                }
                            }
                            break;
                        case 1:
                            if(event.input.type == InputTypePress) {
                                speed_up();
                            }
                            break;
                        case 2:
                            if(event.input.type == InputTypePress)
                                tama_btn_state = BTN_STATE_PRESSED;
                            else if(event.input.type == InputTypeRelease)
                                tama_btn_state = BTN_STATE_RELEASED;

                            switch(sub_menu_buttons) {
                            case 0: // A+C
                                tamalib_set_button(BTN_LEFT, tama_btn_state);
                                tamalib_set_button(BTN_RIGHT, tama_btn_state);
                                break;
                            case 1: // A
                                tamalib_set_button(BTN_LEFT, tama_btn_state);
                                break;
                            case 2: // B
                                tamalib_set_button(BTN_MIDDLE, tama_btn_state);
                                break;
                            case 3: // C
                                tamalib_set_button(BTN_RIGHT, tama_btn_state);
                                break;
                            default:
                                break;
                            }
                            break;
                        case menu_items - 1:
                        default:
                            if(event.input.type == InputTypePress) {
                                switch(sub_menu_last) {
                                case 0: // close menu
                                    in_menu = false;
                                    break;
                                case 1: // Save
                                    if(speed != 1) {
                                        uint8_t temp = speed;
                                        speed = 1;
                                        tamalib_set_speed(speed);
                                        furi_timer_stop(timer);
                                        tama_p1_save_state();
                                        furi_timer_start(
                                            timer, furi_kernel_get_tick_frequency() / 30);
                                        speed = temp;
                                        tamalib_set_speed(speed);
                                    } else {
                                        furi_timer_stop(timer);
                                        tama_p1_save_state();
                                        furi_timer_start(
                                            timer, furi_kernel_get_tick_frequency() / 30);
                                    }
                                    break;
                                case 2: // Save & Exit
                                    if(speed != 1) {
                                        speed = 1;
                                        tamalib_set_speed(speed);
                                    }
                                    furi_timer_stop(timer);
                                    tama_p1_save_state();
                                    running = false;
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        }
                    }
                } else { // out of menu
                    if(event.input.key == InputKeyBack && event.input.type == InputTypeLong) {
                        if(speed != 1) {
                            speed = 1;
                            tamalib_set_speed(speed);
                        }
                        furi_timer_stop(timer);
                        tama_p1_save_state();
                        running = false;
                    } else if(
                        event.input.type == InputTypePress ||
                        event.input.type == InputTypeRelease) {
                        if(event.input.key != InputKeyBack && event.input.key != m) {
                            if(event.input.type == InputTypePress)
                                tama_btn_state = BTN_STATE_PRESSED;
                            else if(event.input.type == InputTypeRelease)
                                tama_btn_state = BTN_STATE_RELEASED;
                        } else {
                            tama_btn_state = BTN_STATE_RELEASED;
                        }
                        if(event.input.key == m && event.input.type == InputTypePress) {
                            in_menu = true;
                        } else if(event.input.key == a) {
                            tamalib_set_button(BTN_LEFT, tama_btn_state);
                        } else if(event.input.key == b) {
                            tamalib_set_button(BTN_MIDDLE, tama_btn_state);
                        } else if(event.input.key == c) {
                            tamalib_set_button(BTN_RIGHT, tama_btn_state);
                        } else if(event.input.key == InputKeyOk) {
                            tamalib_set_button(BTN_MIDDLE, tama_btn_state);
                        }
                    }
                }
            }
            furi_mutex_release(g_state_mutex);
        }
        // else {
        //     // Timeout
        //     FURI_LOG_D(TAG, "Timed out");
        // }
    }
    if(ctx->rom != NULL) {
        furi_thread_flags_set(furi_thread_get_id(ctx->thread), 1);
        furi_thread_join(ctx->thread);
    }
    furi_timer_free(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_mutex_free(g_state_mutex);
    tama_p1_deinit(ctx);
    free(ctx);
    return 0;
}
