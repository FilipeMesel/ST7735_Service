#ifndef UI_LAYOUT_H_
#define UI_LAYOUT_H_

#include "../include/ui_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de Layout dividida em 3 Viewports
 */
typedef struct {
    ui_rect_t header;   /* Padrão: 128x20 (Top) */
    ui_rect_t content;  /* Padrão: 128x120 (Middle) */
    ui_rect_t footer;   /* Padrão: 128x20 (Bottom) */
} ui_layout_t;

/**
 * @brief Inicializa o layout padrão do sistema com as 3 regiões dimensionadas.
 * 
 * @param layout Ponteiro para a estrutura de layout a ser inicializada.
 */
void ui_layout_init_standard(ui_layout_t *layout);

/**
 * @brief Atualiza de forma rápida o texto da região de Header.
 * 
 * @param layout Ponteiro para o layout ativo.
 * @param title Texto a ser exibido no cabeçalho.
 * @param fg_color Cor do texto.
 * @param bg_color Cor de fundo da barra do cabeçalho.
 */
void ui_draw_header(const ui_layout_t *layout, const char *title, uint16_t fg_color, uint16_t bg_color);

/**
 * @brief Atualiza de forma rápida o texto da região de Footer.
 * 
 * @param layout Ponteiro para o layout ativo.
 * @param status_text Texto de status/rodapé.
 * @param fg_color Cor do texto.
 * @param bg_color Cor de fundo da barra do rodapé.
 */
void ui_draw_footer(const ui_layout_t *layout, const char *status_text, uint16_t fg_color, uint16_t bg_color);

#ifdef __cplusplus
}
#endif

#endif /* UI_LAYOUT_H_ */