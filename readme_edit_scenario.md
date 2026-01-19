# Edit Scenario (fairy_tale)

## 1) Project Structure

```text
components/
  ui/
    include/
      builtin_texts.h            (←) story node IDs (case enum)
    fairy_tale/                  (←) scenario-specific visuals
      visuals_fairy_tale.c       (←) mapping story nodes → LVGL images + loaders
      ui_img_fairy_tale_01_png.c (←) C image descriptors (wrappers for binary frames)
      ui_img_fairy_tale_02_png.c
      ...
    builtin_texts_fairy_tale.c   (←) story text fragments for tinyTTS
    story_fairy_tale.c           (←) branching logic, state machine, node transitions
spiffs_root/
  assets_fairy_tale/             (←) .bin files for scenario cards
    ui_img_fairy_tale_01_png.bin
    ui_img_fairy_tale_02_png.bin
    ...
```

---

## 2) Produce RAW Binary Images

### Option A — SquareLine Studio (SLS)

1. Open the project in **SquareLine Studio** (LVGL v8).  
2. Create/Configure export:
   - set the **export path**;
   - set output format to **binary raw**.
3. Import images: *Assets* → **Import** (PNG / UI frame sample size 578×339).  
4. Export/Generate: **Export UI files**.
   - Copy generated `*.bin` from `export_root/drive/assets` to `/spiffs_root/assets_fairy_tale`.
   - Copy generated C arrays `img_*.c` from your “UI file export path” to `/components/ui/fairy_tale`.

### Option B — LVGL Image Converter (official online tool)

1. Open **LVGL Image Converter**.  
2. Upload PNG/JPG.  
3. Parameters:
   - **Color format:** `True color (RGB565)`;
   - **Alpha:** `None` (or as required);
   - **Output:** `Binary`.  
4. Download `*.bin` and place them in `/spiffs_root/assets_fairy_tale`.  
   For C arrays, select **C array** and save resulting `img_*.c` files into `/components/ui/fairy_tale`.

---

## 3) Editng scenario Step-by-Step (with inline code anchors)

### 3.1 Define the cases (IDs)

Edit `components/ui/include/builtin_texts.h` — add new case IDs **before** `CASE_TXT_COUNT`. Keep `CASE_TXT_COUNT` last.

```c
typedef enum {
    CASE_TXT_01 = 0,
    /* ... existing ... */
    CASE_TXT_10,
    /* add new cases here */
    // CASE_TXT_11,
    // CASE_TXT_12,
    CASE_TXT_COUNT   /* must stay last */
} builtin_text_case_t;
```
---

### 3.2 Provide text fragments for tinyTTS

Edit `components/ui/builtin_texts_fairy_tale.c` and define the story fragments for each case ID.

Typical pattern:

```c
#include "builtin_texts.h"

const char* builtin_text_set(builtin_text_case_t id) {
    switch (id) {
        case CASE_TXT_01:
            return "Once upon a time, ...";
        /* ... existing cases ... */
        // case CASE_TXT_02:
        //     return "The heroine enters the dark forest...";
        default:
            return "";
    }
}
```

---

### 3.3 Prepare raw image binaries (.bin)

Convert your PNG/JPG to LVGL **binary** (`True color / RGB565`) and place them under:

```
assets/
  ui_img_fairy_tale_01_png.bin
  ui_img_fairy_tale_02_png.bin
  ui_img_fairy_tale_03_png.bin
```

> Use the exact filenames you’ll reference in your C image wrappers (next step).

---

### 3.4 Create C image wrappers (one per image)

After generate **C arrays** for images (`img_*.c`), store them **in the scenario folder**  

```
components/ui/
  ui_img_fairy_tale_01_png.c
  ui_img_fairy_tale_02_png.c
  ui_img_fairy_tale_03_png.c
```
---

### 3.5 Map cases → images for Screen

Edit `components/ui/fairy_tale/visuals_fairy_tale.c` to map each `builtin_text_case_t` to its LVGL image and loader.

Example pattern:

```c
#include "visuals.h"

typedef void (*img_loader_t)(void);

typedef struct { 
  const lv_img_dsc_t* img; 
  img_loader_t load; 
} case_visual_t;

static const case_visual_t kVisuals[CASE_TXT_COUNT] = {
    [CASE_TXT_01] = { &ui_img_fairy_tale_01_png, ui_img_fairy_tale_01_png_load },
    /* ... existing mappings ... */
    // [CASE_TXT_11] = { &ui_img_fairy_tale_11_png, ui_img_fairy_tale_11_png_load },
};
```

---

### 3.6 Define branching logic (story_fairy_tale.c)

The file `components/ui/story_fairy_tale.c` implements the **story state machine** and branching rules used by `ui_choice1` / `ui_choice2`.  
Each story node is represented by a `builtin_text_case_t` value and described by a `story_node_t` structure.

At the top of the file, a set of macros is used to give readable aliases to the case IDs and to separate **internal branching nodes** from **terminal endings**:

```c
#include "story_fairy_tale.h"
#include <stddef.h>

#define A       CASE_TXT_01
#define B1      CASE_TXT_02
#define B2      CASE_TXT_03
#define B1_1    CASE_TXT_04
#define B1_2    CASE_TXT_05
#define B2_1    CASE_TXT_06
#define B2_2    CASE_TXT_07
#define B1_1_1  CASE_TXT_08
#define B1_1_2  CASE_TXT_09
#define B1_2_1  CASE_TXT_10
#define B1_2_2  CASE_TXT_11
#define B2_1_1  CASE_TXT_12
#define B2_1_2  CASE_TXT_13
#define B2_2_1  CASE_TXT_14
#define B2_2_2  CASE_TXT_15

/* 16 terminal endings */
#define D1      CASE_TXT_16
#define D2      CASE_TXT_17
#define D3      CASE_TXT_18
#define D4      CASE_TXT_19
#define D5      CASE_TXT_20
#define D6      CASE_TXT_21
#define D7      CASE_TXT_22
#define D8      CASE_TXT_23
#define D9      CASE_TXT_24
#define D10     CASE_TXT_25
#define D11     CASE_TXT_26
#define D12     CASE_TXT_27
#define D13     CASE_TXT_28
#define D14     CASE_TXT_29
#define D15     CASE_TXT_30
#define D16     CASE_TXT_31
```

Each node contains:

- the **question** text shown above the choices,
- two **choice labels** (`choice1`, `choice2`) used for `ui_choice1` and `ui_choice2`,
- a flag `is_final` indicating whether this node is a terminal ending,
- and the IDs of the next nodes (`next1`, `next2`) for each choice.

```c
typedef struct {
    const char *question;        // question for this node
    const char *choice1;         // label for ui_choice1
    const char *choice2;         // label for ui_choice2
    bool        is_final;        // true for one of the 16 endings
    builtin_text_case_t next1;   // next case if ui_choice1 is pressed
    builtin_text_case_t next2;   // next case if ui_choice2 is pressed
} story_node_t;
```

All nodes are stored in a static lookup table indexed by `builtin_text_case_t`:

```c
static const story_node_t NODES[CASE_TXT_COUNT] = {
    [A] = {
        .question = "Where should she go next ?",
        .choice1  = "To a cottage with smoke from the chimney.",
        .choice2  = "To a shining waterfall where she can drink and rest.",
        .is_final = false,
        .next1    = B1,
        .next2    = B2,
    },
    [B1] = {
        .question = "What should she do ?",
        .choice1  = "Go inside and see who lives there.",
        .choice2  = "Look around the outside to make sure it's safe",
        .is_final = false,
        .next1    = B1_1,
        .next2    = B1_2,
    },
    [B2] = {
        .question = "What should she do ?",
        .choice1  = "Ask the deer to show the way to people.",
        .choice2  = "Stay by the waterfall and rest.",
        .is_final = false,
        .next1    = B2_1,
        .next2    = B2_2,
    },

    /* ... intermediate branching nodes ... */

    [B2_2_2] = {
        .question = "What should she do next ?",
        .choice1  = "Follow the old woman's trail.",
        .choice2  = "Stay by the waterfall until morning.",
        .is_final = false,
        .next1    = D15,
        .next2    = D16,
    },

    /* 16 terminal endings: no further choices, only final narration */
    [D1]  = { .question = "", .choice1 = "", .choice2 = "", .is_final = true },
    [D2]  = { .question = "", .choice1 = "", .choice2 = "", .is_final = true },
    /* ... D3 .. D15 ... */
    [D16] = { .question = "", .choice1 = "", .choice2 = "", .is_final = true },
};
```

---

### 3.7 Rebuild (clean when assets change)

```
idf.py fullclean
idf.py build
idf.py -p COMx flash monitor
```

> The clean build ensures the SPIFFS image is rebuilt with your new `.bin` files.
