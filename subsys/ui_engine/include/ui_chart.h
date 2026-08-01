#ifndef UI_CHART_H_
#define UI_CHART_H_

#include "ui_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_CHART_MAX_SAMPLES 128

/**
 * @brief Contexto do Gráfico de Telemetria / Biometria em Tempo Real
 */
typedef struct {
    ui_rect_t bounds;                      /* Área delimitada do gráfico */
    int16_t sample_buffer[UI_CHART_MAX_SAMPLES]; /* Ring buffer de amostras */
    uint16_t head_index;                   /* Índice da última amostra inserida */
    
    int16_t min_val;                       /* Limite mínimo de sinal (ADC scaling) */
    int16_t max_val;                       /* Limite máximo de sinal (ADC scaling) */
    
    uint16_t line_color;                   /* Cor da curva/gráfico */
    uint16_t bg_color;                     /* Cor de fundo para limpeza da faixa */
    
    uint8_t curr_x;                        /* Coluna atual de desenho (0 a width-1) */
    uint8_t prev_y;                        /* Ponto Y anterior para interpolação da linha */
} ui_chart_t;

/**
 * @brief Inicializa o componente de gráfico vinculando-o a uma região viewport.
 * 
 * @param chart Contexto do gráfico.
 * @param bounds Região de tela reservada (normalmente a área Content).
 * @param min_val Valor mínimo esperado da amostragem.
 * @param max_val Valor máximo esperado da amostragem.
 * @param line_color Cor da linha do gráfico.
 * @param bg_color Cor de fundo.
 */
void ui_chart_init(ui_chart_t *chart, ui_rect_t bounds, int16_t min_val, int16_t max_val, uint16_t line_color, uint16_t bg_color);

/**
 * @brief Adiciona uma nova amostra bruta vinda do sensor/ADC ao buffer circular.
 * 
 * @param chart Contexto do gráfico.
 * @param raw_value Valor do sensor.
 */
void ui_chart_add_sample(ui_chart_t *chart, int16_t raw_value);

/**
 * @brief Renderiza incrementalmente a nova amostra usando varredura por faixa vertical.
 * 
 * @param chart Contexto do gráfico.
 */
void ui_chart_render(ui_chart_t *chart);

#ifdef __cplusplus
}
#endif

#endif /* UI_CHART_H_ */