#ifndef UI_ENGINE_H_
#define UI_ENGINE_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dimensões nativas do display ST7735 */
#define UI_LCD_WIDTH     128
#define UI_LCD_HEIGHT    160

/* Definições de Cores RGB565 (Com byte swap para SPI ST7735) */
#define UI_COLOR_BLACK   0x0000
#define UI_COLOR_BLUE    0xF800
#define UI_COLOR_WHITE   0xFFFF
#define UI_COLOR_RED     0x00F8
#define UI_COLOR_GREEN   0xE007
#define UI_COLOR_YELLOW  0xE0FF

/**
 * @brief Estrutura de Delimitação / Bounding Box (Região da Tela)
 */
typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
} ui_rect_t;

/**
 * @brief Tipos de Comandos assíncronos enviados para o motor de renderização
 */
typedef enum {
    UI_CMD_FILL_RECT,
    UI_CMD_DRAW_STRING,
    UI_CMD_RENDER_CHART_SAMPLE,
    UI_CMD_RENDER_MENU
} ui_msg_type_t;

/**
 * @brief Estrutura da Fila de Mensagens (Tagged Union Thread-Safe)
 */
typedef struct {
    ui_msg_type_t type;
    union {
        struct {
            ui_rect_t rect;
            uint16_t color;
        } fill_rect;
        
        struct {
            uint8_t x;
            uint8_t y;
            char text[32];
            uint16_t fg_color;
            uint16_t bg_color;
            uint8_t scale;
        } draw_string;

        struct {
            void *chart_ptr; // Ponteiro para ui_chart_t
        } chart_update;

        struct {
            void *menu_ptr;  // Ponteiro para ui_menu_t
        } menu_update;
    } params;
} ui_msg_t;

/**
 * @brief Inicializa os periféricos (SPI, GPIOs), configura o ST7735 e inicia a thread consumidora.
 * 
 * @return int 0 em caso de sucesso ou código de erro negativo.
 */
int ui_engine_init(void);

/**
 * @brief Envia um comando de desenho de forma assíncrona (não-bloqueante) para a fila do motor.
 * 
 * @param msg Ponteiro para a estrutura do comando a ser executado.
 * @param timeout Tempo limite do Zephyr (ex: K_NO_WAIT ou K_MSEC(10)).
 * @return int 0 se o comando foi enfileirado com sucesso.
 */
int ui_engine_post_cmd(const ui_msg_t *msg, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* UI_ENGINE_H_ */