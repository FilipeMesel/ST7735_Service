# 🎨 Zephyr ST7735 Display Subsystem & Abstraction API

A modular, non-blocking, multi-threaded Display & UI Subsystem built on top of **Zephyr RTOS** for the **ST7735 1.8" TFT (128x160)** display.

The goal of this architecture is to decouple application business logic from physical display hardware, SPI drivers, and low-level pixel protocols. The library provides clean, zero-flicker UI primitives tailored for resource-constrained microcontrollers like the ESP32.

---

## 🚀 Key Features

* **Zero-Flicker Partial Refresh:** Uses bounding-box invalidation to re-render only dirty screen sub-regions.
* **Thread-Safe Asynchronous Engine:** Dedicated rendering consumer thread fed by a lock-free message queue (`k_msgq`) to keep the main application thread responsive.
* **Deterministic Memory Footprint:** Eliminates dynamic allocation (`malloc`/`free`) in favor of static Zephyr Memory Slabs and statically sized circular buffers.
* **High-Level UI Components:** Out-of-the-box support for layouts (Headers, Footers, Content Regions), real-time plotting (ECG/PPG charts), dynamic menus, and animated sprite buffers.

---

## 🏗️ Architectural Overview

The subsystem is organized into distinct layers to isolate high-level tasks from physical SPI transfers:

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
                                 v  (Command Queue / k_msgq)
+-------------------------------------------------------------------+
|                  UI Engine Render Thread (Consumer)               |
|         [ Handles Dirty Rectangles, Clipping, & Buffers ]         |
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

## 📁 Project Directory Structure

This library can be integrated as an out-of-tree Zephyr module (zephyr/module.yml) or directly under the subsys/ directory of your application workspace:

```text
app_workspace/
├── CMakeLists.txt
├── prj.conf
├── app.overlay
├── subsys/
│   └── ui_engine/
│       ├── include/
│       │   ├── ui_engine.h       <-- Main Public API Entrypoint
│       │   ├── ui_layout.h       <-- Region & Layout Management
│       │   ├── ui_chart.h        <-- Real-time Plotting Engines
│       │   └── ui_menu.h         <-- Stateful Menu Navigation
│       └── src/
│           ├── ui_engine.c       <-- Thread Pipeline & Command Processing
│           ├── ui_layout.c       <-- Screen Clipping & Dirty Rects
│           ├── ui_chart.c        <-- Ring Buffer & ECG Renderers
│           ├── ui_menu.c         <-- Tree Traversal Logic
│           └── ui_font5x7.c      <-- Embedded Bitmap Font Data
└── src/
    └── main.c                    <-- User Application Entrypoint
```
## 🧩 API Modules

### 1. Core Engine Interface (`ui_engine.h`)
Manages system lifecycle, rendering thread initialization, and non-blocking asynchronous command dispatching via `k_msgq`[cite: 9].
* **Core Abstractions:** `ui_rect_t` (bounding-box regions) and `ui_msg_t` (tagged union command payload)[cite: 9].

### 2. Region & Layout Manager (`ui_layout.h`)
Segregates the $128 \times 160$ panel into three standardized viewports to prevent overlapping redraw conflicts[cite: 9]:
* **Header Region:** Top viewport ($128 \times 20\text{ px}$) reserved for status indicators or titles[cite: 9].
* **Content Region:** Dynamic middle viewport ($128 \times 120\text{ px}$) allocated for primary components (charts, menus, animations)[cite: 9].
* **Footer Region:** Bottom viewport ($128 \times 20\text{ px}$) reserved for system alerts or navigation keys[cite: 9].

### 3. Real-Time Waveform Plotter (`ui_chart.h`)
Streaming waveform graphics renderer (ECG/PPG plots) featuring **Dirty Vertical Strip Invalidation**[cite: 9]:
* Instead of clearing the entire screen, it erases only a single 1-pixel wide vertical column ahead of the write pointer before drawing the new line segment, resulting in smooth, flicker-free rendering[cite: 9].

### 4. Stateful Menu Navigation (`ui_menu.h`)
Interactive list-based menu component supporting state transitions, highlight shifting, and step-based navigation[cite: 9].

## 💡 Quick Start Example

```c
#include <zephyr/kernel.h>
#include "ui_engine.h"
#include "ui_layout.h"
#include "ui_chart.h"

void main(void) {
    // 1. Initialize display hardware and spawn rendering consumer thread
    ui_engine_init();

    // 2. Set up standard viewports layout
    ui_layout_t layout;
    ui_layout_init_standard(&layout);

    // 3. Render static header and footer regions
    ui_draw_header(&layout, "ECG MONITOR", UI_COLOR_WHITE, UI_COLOR_RED);
    ui_draw_footer(&layout, "BPM: 72 (OK)", UI_COLOR_GREEN, UI_COLOR_BLACK);

    // 4. Initialize real-time chart in the content region
    ui_chart_t ecg_chart;
    ui_chart_init(&ecg_chart, layout.content, 0, 100, UI_COLOR_GREEN, UI_COLOR_BLACK);

    while (1) {
        // Read sensor telemetry sample at 50Hz
        int16_t adc_sample = read_biometric_sensor(); 
        
        // Push sample to ring buffer and render incrementally
        ui_chart_add_sample(&ecg_chart, adc_sample);
        ui_chart_render(&ecg_chart);

        k_msleep(20);
    }
}
```

## 🛠️ Engineering Trade-offs & Justifications

* **Asynchronous Message Queues vs. Direct Synchronous Writes:** SPI operations at high refresh rates block CPU execution[cite: 9]. Routing draw calls through `k_msgq` decouples real-time sensor tasks from SPI bus latency[cite: 9].
* **Line Buffers vs. Full Framebuffer:** A full RGB565 framebuffer at $128 \times 160$ resolution requires $\approx 41\text{ KB}$ of RAM[cite: 9]. Operating with stack-allocated single-line buffers ($\approx 256\text{ bytes}$) delivers smooth performance while preserving SRAM for core RTOS operations[cite: 9].
* **Dirty Rectangle Invalidation vs. Full Clear:** Restricting updates strictly to modified bounding boxes prevents screen flickering and reduces overall SPI bus power consumption[cite: 9].