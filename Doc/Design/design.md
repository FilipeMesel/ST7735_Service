# Zephyr ST7735 Display Subsystem & Abstraction API Architectures

## 1. Executive Summary & Core Objectives

This specification defines a modular, non-blocking, multi-threaded Display & UI Subsystem built on top of **Zephyr RTOS** for the **ST7735 1.8" TFT (128x160)** display. 

The architecture decouples application business logic from physical display hardware, SPI drivers, and low-level pixel protocols. It provides clean, zero-flicker UI primitives tailored for resource-constrained microcontrollers like the ESP32.

### Key Features
* **Zero-Flicker Partial Refresh:** Uses bounding-box invalidation to re-render only dirty screen sub-regions.
* **Thread-Safe Asynchronous Engine:** Dedicated rendering thread fed by a lock-free message queue (`k_msgq`) to keep the main application thread responsive.
* **Deterministic Memory Footprint:** Eliminates dynamic allocation (`malloc`/`free`) in favor of static Zephyr Memory Slabs and statically sized circular buffers.
* **High-Level UI Components:** Out-of-the-box support for layouts (Headers, Footers, Content Regions), dynamic real-time plotting (ECG/PPG charts), dynamic menus, and animated sprite buffers.

---

## 2. Recommended Directory Structure

To maintain compatibility with Zephyr's modular ecosystem, this library can be organized as an out-of-tree module (`zephyr/module.yml`) or directly under the `drivers`/`subsys` directories of your application repository.

```text
app_workspace/
├── CMakeLists.txt
├── prj.conf
├── app.overlay
├── subsys/
│   └── ui_engine/
│       ├── CMakeLists.txt
│       ├── Kconfig
│       ├── include/
│       │   └── ui_engine/
│       │       ├── ui_engine.h       <-- Main Public API Entrypoint
│       │       ├── ui_layout.h       <-- Region & Layout Management
│       │       ├── ui_chart.h        <-- Real-time Plotting Engines
│       │       └── ui_menu.h         <-- Stateful Menu Navigation
│       └── src/
│           ├── ui_engine.c       <-- Thread Pipeline & Command Processing
│           ├── ui_layout.c       <-- Screen Clipping & Dirty Rects
│           ├── ui_chart.c        <-- Ring Buffer & ECG Renderers
│           ├── ui_menu.c         <-- Tree Traversal Logic
│           └── ui_font5x7.c      <-- Embedded Bitmap Font Data
└── src/
    └── main.c                    <-- User Application Entrypoint
```

## 3. Layered Architectural Overview

```text
+-------------------------------------------------------------------+
|                     Application Layer                             |
|    (e.g., Tamagotchi State Machine  /  Heart Rate Monitor Task)   |
+-------------------------------------------------------------------+
                                 |
                                 v  (Asynchronous Thread-Safe API Call)
+-------------------------------------------------------------------+
|                        UI Abstraction API                         |
|      (ui_engine, ui_layout, ui_chart, ui_menu modules)            |
+-------------------------------------------------------------------+
                                 |
                                 v  (Command Queue / Zephyr WorkQueue)
+-------------------------------------------------------------------+
|                  UI Engine Render Thread (Consumer)               |
|         [ Handles Dirty Rectangles, Clipping, & Double Buffer ]   |
+-------------------------------------------------------------------+
                                 |
                                 v  (Line / Block Buffer Writes)
+-------------------------------------------------------------------+
|                     Zephyr Display Subsystem                      |
|                     (spi_write_dt / mipi_dbi)                     |
+-------------------------------------------------------------------+
                                 |
                                 v  (Hardware SPI Protocol)
+-------------------------------------------------------------------+
|                   Physical Hardware (ESP32 + ST7735)              |
+-------------------------------------------------------------------+
```

## 4. Design of Public API & Interface Specifications

The public interface is split into four cohesive modules designed to isolate application developers from graphics execution details and hardware-specific configurations.

### 4.1. Core Engine Interface (`ui_engine.h`)
* **Role:** Manages system lifecycle, thread initialization, synchronization primitives, and low-level command dispatching.
* **Data Abstractions:**
  * **Bounding Box (`ui_rect_t`):** Defines absolute $(X, Y, \text{Width}, \text{Height})$ coordinates for clipping and partial redrawing.
  * **Command Structures (`ui_msg_t`):** Tagged unions containing draw operations (fill rect, draw string, plot update, render menu) queued as fixed-size messages.
* **Key Operations:**
  * System initialization function (`ui_engine_init`) to configure SPI controllers, GPIO lines (DC/RST), and spawn the consumer thread.
  * Asynchronous message posting (`ui_engine_post_cmd`) with non-blocking timeouts for thread-safe operations from high-priority application tasks.

### 4.2. Region & Layout Manager (`ui_layout.h`)
* **Role:** Segregates the $128 \times 160$ panel into standardized viewports to eliminate overlapping redraw conflicts.
* **Data Abstractions:**
  * **Screen Layout (`ui_layout_t`):** Struct holding three distinct `ui_rect_t` viewports:
    * **Header Region:** Fixed top viewport ($128 \times 20$ pixels) reserved for status icons, system time, or telemetry titles.
    * **Content Region:** Dynamic middle viewport ($128 \times 120$ pixels) allocated for primary active components (e.g., charts, pet sprites, menus).
    * **Footer Region:** Fixed bottom viewport ($128 \times 20$ pixels) reserved for system alerts or navigation keys.
* **Key Operations:**
  * Standard layout initializer (`ui_layout_init_standard`).
  * Dedicated header/footer renderers accepting text strings, foreground colors, and background colors.

### 4.3. Real-Time Waveform Plotter (`ui_chart.h`)
* **Role:** Provides high-performance, zero-flicker streaming graphics (ECG/PPG plots, line graphs) without re-clearing the display area.
* **Data Abstractions:**
  * **Chart Context (`ui_chart_t`):** Encapsulates viewport boundaries, value range constraints ($\text{Min}/\text{Max}$ normalization), rendering colors, a circular data buffer, and write pointers.
* **Key Operations:**
  * Chart context setup (`ui_chart_init`) specifying dynamic target bounds and scaling boundaries.
  * Sample insertion (`ui_chart_add_sample`) to push raw telemetry points into the ring buffer.
  * Incremental render driver (`ui_chart_render`) that erases previous line segments and draws new segments strictly within dirty vertical slices.

### 4.4. Stateful Menu Navigation (`ui_menu.h`)
* **Role:** Offers interactive list-based menu rendering with automatic cursor position tracking and highlight state transitions.
* **Data Abstractions:**
  * **Menu Context (`ui_menu_t`):** Tracks an array of text strings, current active index selection, visible item counts, and spatial bounds.
* **Key Operations:**
  * Menu initialization (`ui_menu_init`) binding raw text strings to target UI regions.
  * Navigation handler (`ui_menu_navigate`) accepting relative step directions ($+1 / -1$) to modify highlight indices.
  * Menu renderer (`ui_menu_render`) that draws list items and redraws item backgrounds selectively during cursor shifts.

---

## 5. Architectural Execution Model & Operational Workflows

### 5.1. Asynchronous Producer-Consumer Pipeline

To guarantee real-time execution bounds for critical tasks (such as biometric sensor sampling or game loops), rendering operations run completely off-loaded on a dedicated UI consumer thread:

```text
+-------------------+           +-------------------+           +---------------------+
| Application Task  |           | Zephyr Message    |           | Render Consumer     |
| (Producer)        |           | Queue (k_msgq)    |           | Thread (Priority 5) |
+-------------------+           +-------------------+           +---------------------+
          |                               |                                |
          |--- ui_engine_post_cmd() ----->|                                |
          |    (Non-blocking post)        |--- k_msgq_get() (Blocked) ---->|
          |                               |    (Awakens UI thread)         |
          |                               |                                |-- Process command
          |                               |                                |-- Set DC / CS GPIOs
          |                               |                                |-- Burst SPI Transfers
          v                               v                                v
```

### 5.2. Zero-Flicker Waveform Refresh Pipeline

To prevent flickering during real-time telemetry rendering (such as ECG plots), the chart engine relies on a **Dirty Vertical Strip Invalidation** technique rather than clearing the whole screen:

1. **New Sample Arrival:** A sensor task pushes a new value to the circular buffer.
2. **Column Invalidation:** The engine calculates the current horizontal plotting position $X_{\text{curr}}$ and target vertical coordinate $Y_{\text{curr}}$.
3. **Selective Erasure:** Only a single vertical strip ($1 \text{ pixel wide} \times \text{Chart Height}$) at $X_{\text{curr}}$ is cleared to background color.
4. **Vector Connection:** A line segment is drawn connecting $(X_{\text{prev}}, Y_{\text{prev}})$ to $(X_{\text{curr}}, Y_{\text{curr}})$.
5. **Pointer Advance:** The horizontal index advances ($X_{\text{curr}} = (X_{\text{curr}} + 1) \bmod \text{Chart Width}$), creating a continuous rolling waveform.

---

## 6. Practical Integration Scenarios

### 6.1. Scenario A: Animated Tamagotchi Application
* **Initialization Phase:**
  * Calls `ui_engine_init` to start background graphics execution.
  * Calls `ui_layout_init_standard` to section the display into header, content, and footer viewports.
  * Invokes `ui_draw_header` with static text `"TAMAGOTCHI"` and `ui_draw_footer` with status text `"Pet: Happy"`.
  * Instantiates a `ui_menu_t` component inside the content region with items: `"Feed"`, `"Play"`, `"Sleep"`.
* **Execution Loop:**
  * A main state-machine thread runs high-level pet behavior timers.
  * Every cycle, state transitions trigger `ui_menu_navigate` calls when key events occur, asynchronously updating cursor positions without stalling game logic.

### 6.2. Scenario B: Real-Time Heart Rate / ECG Monitor
* **Initialization Phase:**
  * Initializes core display pipeline and standard UI regions.
  * Renders header `"ECG TELEMETRY"` in high-visibility red, and footer `"BPM: 72 (NORMAL)"` in green.
  * Instantiates a `ui_chart_t` component assigned strictly to the content region ($128 \times 120$ pixels) with scaling parameters matching expected ADC ranges ($0\text{--}100\text{ mV}$).
* **Execution Loop:**
  * A high-priority hardware timer thread samples an analog sensor at $50\text{ Hz}$.
  * Each new sample is passed directly to `ui_chart_add_sample` followed by `ui_chart_render`.
  * The chart updates incrementally line-by-line across the screen, providing smooth waveform animation while keeping the CPU free for telemetry processing.

---

## 7. Architectural Trade-offs & Engineering Justifications

1. **Asynchronous Message Queues vs. Direct Synchronous Writes:**
   * *Justification:* SPI bus operations at high refresh rates block CPU execution. Routing calls through a lock-free `k_msgq` decouples real-time sensor tasks from SPI bus latency.

2. **Line-Buffer Iteration vs. Full Framebuffer:**
   * *Justification:* A full RGB565 framebuffer at $128 \times 160$ resolution requires $40.96 \text{ KB}$ of RAM. Operating with stack-allocated single-line buffers ($256 \text{ bytes}$) delivers smooth performance while preserving SRAM for core RTOS operations.

3. **Dirty Rectangle Invalidation vs. Full Clear:**
   * *Justification:* Clearing full display areas before redrawing creates objectionable screen flashing (flicker). Restricted bounding-box updates ensure consistent visual quality and lower power consumption on the SPI bus.
