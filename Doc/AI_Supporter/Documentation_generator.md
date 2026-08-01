# Atue como um Engenheiro de Software Embarcado Especialista e Arquiteto de Sistemas C/C++

## 1. Contexto e Objetivo
Estou desenvolvendo um projeto no **ESP32** utilizando o **Zephyr RTOS** e um display **TFT ST7735 (1.8", 128x160)**.
O meu objetivo é pegar a minha implementação atual e transformá-la em uma **obra-prima de arquitetura de software embarcado**. 

Quero criar uma **API de interface gráfica (Display Component/Framework)** altamente abstrata, performática, desacoplada e reutilizável. Ela deve funcionar como um componente independente que eu possa simplesmente "plug-and-play" em qualquer projeto futuro que utilize esse display ou similar.

---

## 2. Requisitos de Arquitetura e Casos de Uso

A API deve ser desenhada para suportar com extrema facilidade cenários como:
- **Projeto A (Tamagotchi):** Animações leves, atualização contínua de status/menus, renderização de sprites e layouts dinâmicos.
- **Projeto B (Monitor de Frequência Cardíaca):** Plotagem e atualização contínua de gráficos em tempo real (ex: curva de ECG/PPG), mostradores de números/métricas (headers/footers) sem "flicker" (cintilação) na tela.

### Funcionalidades Esperadas na API:
1. **Layouts Standard/Regiões:** Abstração rápida para criação e gerenciamento de *Headers*, *Footers* e áreas de conteúdo principal.
2. **Gráficos em Tempo Real:** Módulos para plotagem e atualização eficiente de gráficos (linhas, barras, buffers cirulares).
3. **Gerenciador de Menus:** Estrutura simples para criação de menus navegáveis (listas, seleções, telas).
4. **Gerenciamento de Estado & Concorrência:** Aproveitamento das primitives do Zephyr RTOS (Workqueues, Semaphores/Mutexes, Threads dedicadas de Renderização, LVGL integration se aplicável ou Display Driver API nativa do Zephyr).

---

## 3. Diretrizes de Engenharia de Software Embarcado
Ao projetar a solução, siga os seguintes princípios:
- **Baixo Acoplamento e Alta Coesão:** O código de aplicação (ex: lógica do Tamagotchi) NÃO deve conhecer detalhes de hardware do ST7735 nem chamadas diretas de baixo nível da SPI.
- **Eficiência de Memória:** Como estamos em sistemas embarcados, minimize alocações dinâmicas (`malloc`). Prefira buffers estáticos ou pools de memória do Zephyr.
- **Prevenção de Flicker:** Uso adequado de estratégias de double-buffering ou partial screen refresh (atualização apenas da região modificada).
- **Flexibilidade no Zephyr:** Siga as convenções de projetos Zephyr (estruturação em `subsys` ou `drivers` customizados, uso do `Device Tree` e `Kconfig` para habilitação de módulos).

---

## 4. O que você deve entregar nesta resposta:

1. Preciso que você defina para mim a documentação de design do projeto

- Precisa ser um arquivo .md que defina:
1. O que a API faz
2. Estrutura de pastas
3. Divisão entre Api, subsistema e driver
4. Visão Geral da Arquitetura: Diagrama textual/conceitual da separação em camadas (Aplicação -> API Abstrata de Display -> Zephyr Display Subsystem -> Driver ST7735).
5. **Design da API (`public headers`):** Definição das estruturas C/C++ e assinaturas das funções principais (`display_ui_init`, `ui_create_header`, `ui_update_chart`, `ui_menu_render`, etc.).
3. **Exemplo Prático de Implementação:** Códigos funcionais que mostrem o esqueleto da biblioteca.
4. **Exemplo de Uso nos 2 Projetos:**
   - Exemplo curto de como a API seria chamada no contexto do **Tamagotchi**.
   - Exemplo curto de como a API seria chamada no contexto do **Monitor Cardíaco**.
5. **Estrutura de Pastas Sugerida:** Como organizar essa biblioteca dentro de um espaço de trabalho Zephyr (módulo reutilizável).

Por favor, apresente sua resposta com um arquivo .md limpo, comentado e justificativas arquiteturais para cada decisão tomada em inglês.