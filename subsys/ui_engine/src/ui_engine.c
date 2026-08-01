#include "../include/lcd.h"
#include "../include/ui_engine.h"
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(UI_ENGINE, LOG_LEVEL_INF);

/* Configuration Constants */
#define UI_STACK_SIZE       2048
#define UI_THREAD_PRIORITY  5
#define UI_MSGQ_MAX_MSGS    16

/* ST7735 Commands */
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_NORON   0x13
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

/* Zephyr Hardware Specs */
static const struct spi_dt_spec spi_spec = SPI_DT_SPEC_GET(DT_NODELABEL(st7735_dev), SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);
static const struct gpio_dt_spec dc_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(lcd_dc), gpios);
static const struct gpio_dt_spec rst_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(lcd_rst), gpios);

/* RTOS Thread & Queue Primitives */
K_MSGQ_DEFINE(ui_msgq, sizeof(ui_msg_t), UI_MSGQ_MAX_MSGS, 4);
K_THREAD_STACK_DEFINE(ui_stack_area, UI_STACK_SIZE);
static struct k_thread ui_thread_data;

/* Embedded 5x7 Font Bitmap Data */
static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
    0x14, 0x7f, 0x14, 0x7f, 0x14, 0x24, 0x2a, 0x7f, 0x2a, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62,
    0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x41, 0x00,
    0x00, 0x41, 0x22, 0x1c, 0x00, 0x14, 0x08, 0x3e, 0x08, 0x14, 0x08, 0x08, 0x3e, 0x08, 0x08,
    0x00, 0x50, 0x30, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x60, 0x60, 0x00, 0x00,
    0x20, 0x10, 0x08, 0x04, 0x02, 0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00, 0x42, 0x7f, 0x40, 0x00,
    0x42, 0x61, 0x51, 0x49, 0x46, 0x21, 0x41, 0x45, 0x4b, 0x31, 0x18, 0x14, 0x12, 0x7f, 0x10,
    0x27, 0x45, 0x45, 0x45, 0x39, 0x3c, 0x4a, 0x49, 0x49, 0x30, 0x01, 0x71, 0x09, 0x05, 0x03,
    0x36, 0x49, 0x49, 0x49, 0x36, 0x06, 0x49, 0x49, 0x29, 0x1e, 0x00, 0x36, 0x36, 0x00, 0x00,
    0x00, 0x56, 0x36, 0x00, 0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14,
    0x00, 0x41, 0x22, 0x14, 0x08, 0x02, 0x01, 0x51, 0x09, 0x06, 0x32, 0x49, 0x79, 0x41, 0x3e,
    0x7e, 0x11, 0x11, 0x11, 0x7e, 0x7f, 0x49, 0x49, 0x49, 0x36, 0x3e, 0x41, 0x41, 0x41, 0x22,
    0x7f, 0x41, 0x41, 0x22, 0x1c, 0x7f, 0x49, 0x49, 0x49, 0x41, 0x7f, 0x09, 0x09, 0x09, 0x01,
    0x3e, 0x41, 0x49, 0x49, 0x7a, 0x7f, 0x08, 0x08, 0x08, 0x7f, 0x00, 0x41, 0x7f, 0x41, 0x00,
    0x20, 0x40, 0x41, 0x3f, 0x01, 0x7f, 0x08, 0x14, 0x22, 0x41, 0x7f, 0x40, 0x40, 0x40, 0x40,
    0x7f, 0x02, 0x0c, 0x02, 0x7f, 0x7f, 0x04, 0x08, 0x10, 0x7f, 0x3e, 0x41, 0x41, 0x41, 0x3e,
    0x7f, 0x09, 0x09, 0x09, 0x06, 0x3e, 0x41, 0x51, 0x21, 0x5e, 0x7f, 0x09, 0x19, 0x29, 0x46,
    0x26, 0x49, 0x49, 0x49, 0x32, 0x01, 0x01, 0x7f, 0x01, 0x01, 0x3f, 0x40, 0x40, 0x40, 0x3f,
    0x1f, 0x20, 0x40, 0x20, 0x1f, 0x3f, 0x40, 0x38, 0x40, 0x3f, 0x63, 0x14, 0x08, 0x14, 0x63,
    0x07, 0x08, 0x70, 0x08, 0x07, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x7f, 0x41, 0x41, 0x00,
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41, 0x7f, 0x00, 0x04, 0x02, 0x01, 0x02, 0x04,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x01, 0x02, 0x04, 0x00, 0x20, 0x54, 0x54, 0x54, 0x78,
    0x7f, 0x48, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x20, 0x38, 0x44, 0x44, 0x48, 0x7f,
    0x38, 0x54, 0x54, 0x54, 0x18, 0x08, 0x7e, 0x09, 0x01, 0x02, 0x0c, 0x52, 0x52, 0x52, 0x3e,
    0x7f, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7d, 0x40, 0x00, 0x20, 0x40, 0x44, 0x3d, 0x00,
    0x7f, 0x10, 0x28, 0x44, 0x00, 0x00, 0x41, 0x7f, 0x40, 0x00, 0x7c, 0x04, 0x18, 0x04, 0x78,
    0x7c, 0x08, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38, 0x7c, 0x14, 0x14, 0x14, 0x08,
    0x08, 0x14, 0x14, 0x18, 0x7c, 0x7c, 0x08, 0x04, 0x04, 0x08, 0x48, 0x54, 0x54, 0x54, 0x20,
    0x04, 0x3e, 0x44, 0x24, 0x08, 0x3c, 0x40, 0x40, 0x20, 0x7c, 0x1c, 0x20, 0x40, 0x20, 0x1c,
    0x3c, 0x40, 0x30, 0x40, 0x3c, 0x44, 0x28, 0x10, 0x28, 0x44, 0x0c, 0x50, 0x50, 0x50, 0x3c,
    0x44, 0x64, 0x54, 0x4c, 0x44
};

/* --- Low-Level SPI Hardware Helpers --- */

static void lcd_cmd(uint8_t cmd) {
    gpio_pin_set_dt(&dc_gpio, 0);
    struct spi_buf buf = {.buf = &cmd, .len = 1};
    struct spi_buf_set tx = {.buffers = &buf, .count = 1};
    spi_write_dt(&spi_spec, &tx);
}

static void lcd_data(const uint8_t *data, size_t len) {
    if (len == 0) return;
    gpio_pin_set_dt(&dc_gpio, 1);
    struct spi_buf buf = {.buf = (void *)data, .len = len};
    struct spi_buf_set tx = {.buffers = &buf, .count = 1};
    spi_write_dt(&spi_spec, &tx);
}

static void st7735_hw_init(void) {
    gpio_pin_set_dt(&rst_gpio, 1);
    k_msleep(10);
    gpio_pin_set_dt(&rst_gpio, 0);
    k_msleep(10);
    gpio_pin_set_dt(&rst_gpio, 1);
    k_msleep(150);

    lcd_cmd(ST7735_SWRESET);
    k_msleep(150);

    lcd_cmd(ST7735_SLPOUT);
    k_msleep(500);

    lcd_cmd(ST7735_COLMOD);
    uint8_t format = 0x05; // 16-bit color (RGB565)
    lcd_data(&format, 1);
    k_msleep(10);

    lcd_cmd(ST7735_MADCTL);
    uint8_t madctl = 0xC8; // Orientation/RGB order
    lcd_data(&madctl, 1);

    lcd_cmd(ST7735_NORON);
    k_msleep(10);

    lcd_cmd(ST7735_DISPON);
    k_msleep(100);
}

/* --- Graphics Primitives --- */

static void internal_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    if (w == 0 || h == 0) return;

    lcd_cmd(ST7735_CASET);
    uint8_t caset[] = {0x00, x, 0x00, (uint8_t)(x + w - 1)};
    lcd_data(caset, 4);

    lcd_cmd(ST7735_RASET);
    uint8_t raset[] = {0x00, y, 0x00, (uint8_t)(y + h - 1)};
    lcd_data(raset, 4);

    lcd_cmd(ST7735_RAMWR);

    /* Direct burst write using static pixel buffer to prevent dynamic allocations */
    uint8_t pix[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    uint32_t total_pixels = w * h;
    
    for (uint32_t i = 0; i < total_pixels; i++) {
        lcd_data(pix, 2);
    }
}

static void internal_draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 122) c = '?';

    const uint8_t *char_bitmap = &font5x7[(c - 32) * 5];

    for (int i = 0; i < 5; i++) {
        uint8_t line = char_bitmap[i];
        for (int j = 0; j < 8; j++) {
            if (line & 0x01) {
                internal_fill_rect(x + (i * size), y + (j * size), size, size, color);
            } else if (bg != color) {
                internal_fill_rect(x + (i * size), y + (j * size), size, size, bg);
            }
            line >>= 1;
        }
    }
    if (bg != color) {
        internal_fill_rect(x + (5 * size), y, size, 8 * size, bg);
    }
}

static void internal_draw_string(uint8_t x, uint8_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
    uint8_t cur_x = x;
    uint8_t cur_y = y;

    while (*str) {
        if (*str == '\n') {
            cur_y += size * 8;
            cur_x = x;
        } else {
            internal_draw_char(cur_x, cur_y, *str, color, bg, size);
            cur_x += size * 6;
        }
        str++;
    }
}

/* --- Consumer Render Thread Implementation --- */

static void ui_render_thread(void *arg1, void *arg2, void *arg3) {
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    ui_msg_t msg;

    LOG_INF("UI Render Consumer Thread Started (Priority %d)", UI_THREAD_PRIORITY);

    while (1) {
        /* Block until a command arrives from the lock-free message queue */
        if (k_msgq_get(&ui_msgq, &msg, K_FOREVER) == 0) {
            switch (msg.type) {
            case UI_CMD_FILL_RECT:
                internal_fill_rect(msg.params.fill_rect.rect.x,
                                   msg.params.fill_rect.rect.y,
                                   msg.params.fill_rect.rect.width,
                                   msg.params.fill_rect.rect.height,
                                   msg.params.fill_rect.color);
                break;

            case UI_CMD_DRAW_STRING:
                internal_draw_string(msg.params.draw_string.x,
                                     msg.params.draw_string.y,
                                     msg.params.draw_string.text,
                                     msg.params.draw_string.fg_color,
                                     msg.params.draw_string.bg_color,
                                     msg.params.draw_string.scale);
                break;

            case UI_CMD_RENDER_CHART_SAMPLE:
                /* Forward hook for EP04 / ui_chart renderer module */
                break;

            case UI_CMD_RENDER_MENU:
                /* Forward hook for EP05 / ui_menu renderer module */
                break;

            default:
                LOG_WRN("Unknown UI command type: %d", msg.type);
                break;
            }
        }
    }
}

/* --- Public Subsystem Entry Points --- */

int ui_engine_init(void) {
    LOG_INF("Initializing UI Engine Subsystem...");

    /* Check bus and control line readiness */
    if (!spi_is_ready_dt(&spi_spec) || !gpio_is_ready_dt(&dc_gpio) || !gpio_is_ready_dt(&rst_gpio)) {
        LOG_ERR("SPI/GPIO peripherals not ready");
        return -ENODEV;
    }

    /* Configure GPIO direction */
    gpio_pin_configure_dt(&dc_gpio, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&rst_gpio, GPIO_OUTPUT_ACTIVE);

    /* Perform ST7735 hardware controller initialization */
    st7735_hw_init();

    /* Clear display area to initial black state */
    internal_fill_rect(0, 0, UI_LCD_WIDTH, UI_LCD_HEIGHT, UI_COLOR_BLACK);

    /* Spawn rendering consumer thread with Priority 5 */
    k_thread_create(&ui_thread_data, ui_stack_area,
                    K_THREAD_STACK_SIZEOF(ui_stack_area),
                    ui_render_thread, NULL, NULL, NULL,
                    UI_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(&ui_thread_data, "ui_render_thread");

    LOG_INF("UI Engine initialized successfully");
    return 0;
}

int ui_engine_post_cmd(const ui_msg_t *msg, k_timeout_t timeout) {
    if (!msg) {
        return -EINVAL;
    }

    /* Non-blocking/timed dispatch to queue to keep application tasks responsive */
    return k_msgq_put(&ui_msgq, msg, timeout);
}

/* Exportando as funções públicas para o lcd.h / ui_layout.c */
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color) {
    internal_fill_rect((uint8_t)x, (uint8_t)y, (uint8_t)w, (uint8_t)h, color);
}

void lcd_draw_string(int x, int y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
    internal_draw_string((uint8_t)x, (uint8_t)y, str, color, bg, (uint8_t)size);
}