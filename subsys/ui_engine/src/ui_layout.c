#include "../include/ui_layout.h"
#include <string.h>

#define LCD_WIDTH  128
#define LCD_HEIGHT 160

void ui_layout_init_standard(ui_layout_t *layout) {
    if (!layout) return;

    layout->header.x = 0;
    layout->header.y = 0;
    layout->header.width = LCD_WIDTH;
    layout->header.height = 20;

    layout->content.x = 0;
    layout->content.y = 20;
    layout->content.width = LCD_WIDTH;
    layout->content.height = 120;

    layout->footer.x = 0;
    layout->footer.y = 140;
    layout->footer.width = LCD_WIDTH;
    layout->footer.height = 20;
}

void ui_draw_header(const ui_layout_t *layout, const char *title, uint16_t fg_color, uint16_t bg_color) {
    if (!layout || !title) return;

    // 1. Limpa o fundo via fila de mensagens
    ui_msg_t msg_bg = {
        .type = UI_CMD_FILL_RECT,
        .params.fill_rect = {
            .rect = layout->header,
            .color = bg_color
        }
    };
    ui_engine_post_cmd(&msg_bg, K_FOREVER);

    // 2. Centraliza e envia o texto via fila
    uint8_t str_len = strlen(title);
    int text_x = layout->header.x + ((layout->header.width - (str_len * 6)) / 2);
    int text_y = layout->header.y + ((layout->header.height - 8) / 2);
    if (text_x < 0) text_x = 0;

    ui_msg_t msg_txt = {
        .type = UI_CMD_DRAW_STRING,
        .params.draw_string = {
            .x = (uint8_t)text_x,
            .y = (uint8_t)text_y,
            .fg_color = fg_color,
            .bg_color = bg_color,
            .scale = 1
        }
    };
    strncpy(msg_txt.params.draw_string.text, title, sizeof(msg_txt.params.draw_string.text) - 1);
    ui_engine_post_cmd(&msg_txt, K_FOREVER);
}

void ui_draw_footer(const ui_layout_t *layout, const char *status_text, uint16_t fg_color, uint16_t bg_color) {
    if (!layout || !status_text) return;

    // 1. Limpa o fundo do footer
    ui_msg_t msg_bg = {
        .type = UI_CMD_FILL_RECT,
        .params.fill_rect = {
            .rect = layout->footer,
            .color = bg_color
        }
    };
    ui_engine_post_cmd(&msg_bg, K_FOREVER);

    // 2. Desenha o texto do footer
    uint8_t str_len = strlen(status_text);
    int text_x = layout->footer.x + ((layout->footer.width - (str_len * 6)) / 2);
    int text_y = layout->footer.y + ((layout->footer.height - 8) / 2);
    if (text_x < 0) text_x = 0;

    ui_msg_t msg_txt = {
        .type = UI_CMD_DRAW_STRING,
        .params.draw_string = {
            .x = (uint8_t)text_x,
            .y = (uint8_t)text_y,
            .fg_color = fg_color,
            .bg_color = bg_color,
            .scale = 1
        }
    };
    strncpy(msg_txt.params.draw_string.text, status_text, sizeof(msg_txt.params.draw_string.text) - 1);
    ui_engine_post_cmd(&msg_txt, K_FOREVER);
}