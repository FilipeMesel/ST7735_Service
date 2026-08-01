#ifndef UI_MENU_H_
#define UI_MENU_H_

#include "ui_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_MENU_MAX_ITEMS 8

/**
 * @brief Contexto do Componente de Menu Navegável
 */
typedef struct {
    ui_rect_t bounds;                         /* Limites espaciais da lista */
    const char *items[UI_MENU_MAX_ITEMS];     /* Array de strings para os itens do menu */
    uint8_t item_count;                       /* Quantidade total de itens cadastrados */
    uint8_t selected_index;                   /* Índice atualmente selecionado/destacado */
    
    uint16_t text_color;                      /* Cor do texto normal */
    uint16_t bg_color;                        /* Cor de fundo normal */
    uint16_t highlight_fg;                    /* Cor do texto selecionado */
    uint16_t highlight_bg;                    /* Cor de fundo do item selecionado */
} ui_menu_t;

/**
 * @brief Inicializa o menu associando uma lista de opções a uma região gráfica.
 * 
 * @param menu Contexto do menu.
 * @param bounds Região de exibição (viewport).
 * @param items Array estático de ponteiros para strings com o nome das opções.
 * @param item_count Número de opções.
 */
void ui_menu_init(ui_menu_t *menu, ui_rect_t bounds, const char **items, uint8_t item_count);

/**
 * @brief Navega pela lista alterando a seleção atual.
 * 
 * @param menu Contexto do menu.
 * @param direction Passos de navegação (ex: +1 para baixo, -1 para cima).
 */
void ui_menu_navigate(ui_menu_t *menu, int8_t direction);

/**
 * @brief Renderiza ou atualiza o estado visual do menu na tela.
 * 
 * @param menu Contexto do menu.
 */
void ui_menu_render(const ui_menu_t *menu);

#ifdef __cplusplus
}
#endif

#endif /* UI_MENU_H_ */