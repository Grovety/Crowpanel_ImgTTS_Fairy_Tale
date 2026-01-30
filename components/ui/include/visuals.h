#pragma once
#include "lvgl.h"
#include <stdbool.h>
#include "builtin_texts.h"

typedef void (*img_loader_t)(void);

typedef struct {
    const lv_img_dsc_t* img_s;
    img_loader_t        load_s;
    const lv_img_dsc_t* img_l;
    img_loader_t        load_l;
} case_visual_t;

extern lv_img_dsc_t ui_img_arrow_png;
void ui_img_arrow_png_load(void);

extern lv_img_dsc_t ui_img_bird_png;
void ui_img_bird_png_load(void);

extern lv_img_dsc_t ui_img_choice_png;
void ui_img_choice_png_load(void);

extern lv_img_dsc_t ui_img_frame_lrg_png;
void ui_img_frame_lrg_png_load(void);

extern lv_img_dsc_t ui_img_frame_sml_png;
void ui_img_frame_sml_png_load(void);

extern lv_img_dsc_t ui_img_night_png;
void ui_img_night_png_load(void);

extern lv_img_dsc_t ui_img_question_png;
void ui_img_question_png_load(void);

extern lv_img_dsc_t ui_img_sett_off_png;
void ui_img_sett_off_png_load(void);

extern lv_img_dsc_t ui_img_sett_on_png;
void ui_img_sett_on_png_load(void);

extern lv_img_dsc_t ui_img_speech_png;
void ui_img_speech_png_load(void);

extern const case_visual_t kVisuals[CASE_TXT_COUNT];

static inline bool is_pinned_image(const lv_img_dsc_t* d)
{
    return d == &ui_img_arrow_png   ||
           d == &ui_img_bird_png    ||
           d == &ui_img_choice_png  ||
           d == &ui_img_frame_lrg_png ||
           d == &ui_img_frame_sml_png ||
           d == &ui_img_night_png   ||
           d == &ui_img_question_png||
           d == &ui_img_sett_off_png||
           d == &ui_img_sett_on_png ||
           d == &ui_img_speech_png;
}
