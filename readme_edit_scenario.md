# Edit Scenario (fairy_tale)

## 1) Project Structure

```text
components/
  ui/
    include/
      builtin_texts.h            (←) story node IDs (case enum)
    fairy_tale/                  (←) scenario-specific visuals
      visuals_fairy_tale.c       (←) mapping story nodes → {small+large} LVGL images
      ui_s/                        (←) C image wrappers for small (Screen1)
        ui_img_*_s_png.c
        ...
      ui_l/                        (←) C image wrappers for large (Screen2)
        ui_img_*_l_png.c
        ...
    builtin_texts_fairy_tale.c   (←) narration texts + intro/outro phrases for tinyTTS
    story_fairy_tale.c           (←) branching logic (questions/choices/transitions)
    ui_img_loading_gif.c           (←) GIF crow animations (gifdec)
spiffs_root/
  assets/                          (←) UI assets + GIFs
    crow_idle.gif
    crow_talk.gif
    ...
  assets_s/                        (←) small story images
    ui_img_*_s_png.bin
    ...
  assets_l/                        (←) large story images
    ui_img_*_l_png.bin
    ...
```

---

## 2) Produce RAW Binary Images

You now need two versions of each story illustration:
- small (S) for Screen1 → store in `spiffs_root/assets_s/`
- large (L) for Screen2 → store in `spiffs_root/assets_l/`

### Option A — SquareLine Studio (SLS)

1. Open the project in **SquareLine Studio** (LVGL v8).  
2. Create/Configure export:
   - set the **export path**;
   - set output format to **binary raw**.
3. Import images: *Assets* → **Import** (PNG).  
4. Export/Generate: **Export UI files**.
   - Copy generated C wrappers for small images `img_*.c` from your “UI file export path” to `components/ui/fairy_tale/ui_s/`.
   - Copy generated C wrappers for large images `img_*.c` from your “UI file export path” to `components/ui/fairy_tale/ui_l/`.
   - Copy generated small `*.bin` from `export_root/drive/assets` to `spiffs_root/assets_s/`.
   - Copy generated large `*.bin` from `export_root/drive/assets` to `spiffs_root/assets_l/`.

> ### Mandatory step is zlib compression story `.bin`

The `assets_s/` and `assets_l/` image sets are large enough that storing them as raw RGB565 binaries may exceed the SPIFFS partition size (14 MB).  
Therefore, after copying the generated `.bin` files into `spiffs_root/assets_s/` and `spiffs_root/assets_l/`, you must compress them with zlib before building `spiffs.bin`.

- Run the script `zlib_script.py` from `spiffs_root/`:
   ```bash
   python zlib_script.py
   ```
The script replaces each ui_img_*_png.bin with its zlib-compressed version.

### Option B — LVGL Image Converter (official online tool)

1. Open **LVGL Image Converter**.  
2. Upload PNG/JPG.  
3. Parameters:
   - **Color format:** `True color (RGB565)`;
   - **Alpha:** `None`;
   - **Output:** `Binary`.  
4. Download `*.bin` and place them:
   - small → `spiffs_root/assets_s/`
   - large → `spiffs_root/assets_l/`  
 
The runtime loader detects compressed binaries automatically: if the file size on flash differs from the expected uncompressed size, the file is treated as zlib-compressed and decompressed into PSRAM.

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

Edit `components/ui/builtin_texts_fairy_tale.c` and define the narration fragment for each `builtin_text_case_t`.

- Story fragments array:

```c
static const char* kThirdTexts[] = {
    // A (CASE_TXT_01)
    "Once upon a time ...",

    // B1 (CASE_TXT_02)
    "Snow White walked ...",

    /* ... more fragments ... */

    // D16 (CASE_TXT_31)
    "At dawn, the forest awoke ..."
};
```

- Intro / Outro phrases:

The file also defines two fixed narrator phrases:

`builtin_get_intro_text()` — intro phrase played at the beginning.
It injects the user name from user_name_store and falls back to "my little friend" if no name is stored.

`builtin_get_outro_text()` — outro phrase played when the story ends (before showing “Try again”).


---

### 3.3 Prepare raw image binaries (.bin)

Convert your PNG to LVGL **binary** (`True color / RGB565`) and place them under:

```
Small (S) → spiffs_root/assets_s/ui_img_*_s_png.bin

Large (L) → spiffs_root/assets_l/ui_img_*_l_png.bin
```

> Use the exact filenames you’ll reference in your C image wrappers (next step).

---

### 3.4 Create C image wrappers (one per image)

After generate **C arrays** for images (`img_*.c`), store them **in the scenario folder**  

```
components/ui/fairy_tale/ui_s/ui_img_*_s_png.c

components/ui/fairy_tale/ui_l/ui_img_*_l_png.c
```
---

### 3.5 Map cases → images for Screen

Edit `components/ui/fairy_tale/visuals_fairy_tale.c` to map each `builtin_text_case_t` to its LVGL image and loader.

Example pattern:

```c
#include "visuals.h"

typedef void (*img_loader_t)(void);

typedef struct {
    const lv_img_dsc_t* img_s;
    img_loader_t        load_s;
    const lv_img_dsc_t* img_l;
    img_loader_t        load_l;
} case_visual_t;

static const case_visual_t kVisuals[CASE_TXT_COUNT] = {
    [CASE_TXT_*] = {
        .img_s  = &ui_img_*_s_png,
        .load_s = ui_img_*_s_png_load,
        .img_l  = &ui_img_*_l_png,
        .load_l = ui_img_*_l_png_load,
    },
    ...
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
- two **choice labels** (`choice1`, `choice2`),
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
### 3.7 Narrator GIFs (idle/talk) and speed control

The narrator animation is rendered from GIF files stored in SPIFFS:

- `spiffs_root/assets/crow_idle.gif` — idle animation
- `spiffs_root/assets/crow_talk.gif` — talking animation

GIF decoding is done with **gifdec**. Each screen has its own LVGL image object:
- `ui_bird1` (Screen1)
- `ui_bird2` (Screen2)
- `ui_bird3` (Screen3)

All settings are located in `components/ui/ui_img_loading_gif.c`.

You can change GIF paths:
```c
#define UI_BIRD_NORM_GIF_PATH  "/spiffs/assets/crow_idle.gif"
#define UI_BIRD_TALK_GIF_PATH  "/spiffs/assets/crow_talk.gif"
```

And tune animation speed:
```c
#define GIF_IDLE_SPEED_PCT   120u
#define GIF_TALK_SPEED_PCT   300u
```

---

### 3.8 Rebuild (clean when assets change)

```
idf.py fullclean
idf.py build
idf.py -p COMx flash
```

> The clean build ensures the SPIFFS image is rebuilt with your new `.bin` files.
