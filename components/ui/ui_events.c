#include "ui_events.h"
#include "ui.h"
#include "tts_bridge.h"
#include "builtin_texts.h"
#include "esp_log.h"
#include "esp_system.h" 
#include "lvgl.h"
#include "visuals.h"
#include "esp_heap_caps.h"
#include "story_fairy_tale.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "ui_helpers.h"
#include "user_name_store.h" 

/* UI timing tuning (ms) */

#define UI_FADE_MS            500   
#define UI_CHOICE_MS          80
#define UI_SCREEN_3_MS        10
#define UI_SCREEN_FADE_MS     10   
#define UI_QUESTION_HOLD_MS  2400   
#define UI_OUTRO_DELAY_MS     600   
#define UI_AGAIN_MS           100


static builtin_text_case_t s_current = STORY_START_CASE;
static lv_timer_t *s_tts_timer           = NULL; 
static lv_timer_t *s_after_tts_timer     = NULL; 
static lv_timer_t *s_question_hold_timer = NULL; 
static lv_timer_t *s_outro_tts_timer     = NULL; 

static bool s_intro_mode       = true;  
static bool s_outro_mode       = false; 

static int  s_last_active_screen = 0; 
static bool s_settings_open      = false;

static const char* TAG_UI = "ui_events";

static void show_case(builtin_text_case_t c);
static void tts_timer_cb(lv_timer_t* t);
static void schedule_tts_after_delay(void);
static void ui_notify_tts_finished_async(void *arg);

static void free_all_images_except(const lv_img_dsc_t* except_s,
                                   const lv_img_dsc_t* except_l);
static void apply_image_for_case_internal(builtin_text_case_t c,
                                          bool need_small,
                                          bool need_large);

static void pause_question_and_choice_animations(void);
static void resume_question_and_choice_animations(void);

static void start_choices_fade_in(void);  
static void question_hold_timer_cb(lv_timer_t *t);
static void question_fade_out_ready_cb(lv_anim_t *a);
static void question_fade_in_ready_cb(lv_anim_t *a);
static void start_question_sequence(const story_node_t *node); 

static void outro_tts_timer_cb(lv_timer_t *t);

static void anim_set_opa_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    if (!obj) return;
    lv_obj_set_style_opa(obj, (lv_opa_t)v, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void pause_question_and_choice_animations(void)
{
    if (s_question_hold_timer) {
        lv_timer_pause(s_question_hold_timer);
    }
    if (ui_que2) {
        lv_anim_del(ui_que2, anim_set_opa_cb);
    }
    if (ui_ContainerCh) {
        lv_anim_del(ui_ContainerCh, anim_set_opa_cb);
    }
    if (ui_arr1) {
        lv_anim_del(ui_arr1, anim_set_opa_cb);
    }
    if (ui_arr2) {
        lv_anim_del(ui_arr2, anim_set_opa_cb);
    }
}

static void resume_question_and_choice_animations(void)
{
    const story_node_t *node = story_get_node(s_current);
    if (!node) {
        return;
    }

    if (node->is_final) {
        return;
    }

    if (s_question_hold_timer) {
        lv_timer_del(s_question_hold_timer);
        s_question_hold_timer = NULL;
    }
    start_question_sequence(node);
}



static void start_choices_fade_in(void)
{
    const story_node_t *node = story_get_node(s_current);

    if (!node || node->is_final) {
        if (ui_ContainerCh) lv_obj_add_flag(ui_ContainerCh, LV_OBJ_FLAG_HIDDEN);
        if (ui_arr1)        lv_obj_add_flag(ui_arr1, LV_OBJ_FLAG_HIDDEN);
        if (ui_arr2)        lv_obj_add_flag(ui_arr2, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (ui_Labelch1) {
        lv_label_set_text(ui_Labelch1, node->choice1 ? node->choice1 : "");
    }
    if (ui_Labelch2) {
        lv_label_set_text(ui_Labelch2, node->choice2 ? node->choice2 : "");
    }

    if (ui_ContainerCh) {
        lv_obj_clear_flag(ui_ContainerCh, LV_OBJ_FLAG_HIDDEN);
        anim_set_opa_cb(ui_ContainerCh, LV_OPA_TRANSP);
    }
    if (ui_arr1) {
        lv_obj_clear_flag(ui_arr1, LV_OBJ_FLAG_HIDDEN);
        anim_set_opa_cb(ui_arr1, LV_OPA_TRANSP);
    }
    if (ui_arr2) {
        lv_obj_clear_flag(ui_arr2, LV_OBJ_FLAG_HIDDEN);
        anim_set_opa_cb(ui_arr2, LV_OPA_TRANSP);
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, anim_set_opa_cb);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, UI_FADE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);


    if (ui_ContainerCh) {
        lv_anim_set_var(&a, ui_ContainerCh);
        lv_anim_start(&a);
    }
    if (ui_arr1) {
        lv_anim_set_var(&a, ui_arr1);
        lv_anim_start(&a);
    }
    if (ui_arr2) {
        lv_anim_set_var(&a, ui_arr2);
        lv_anim_start(&a);
    }
}

static void question_fade_out_ready_cb(lv_anim_t *a)
{
    (void)a;
    if (ui_que2) {
        lv_obj_add_flag(ui_que2, LV_OBJ_FLAG_HIDDEN);
    }
    start_choices_fade_in();
}

static void question_hold_timer_cb(lv_timer_t *t)
{
    (void)t;

    if (s_question_hold_timer) {
        lv_timer_del(s_question_hold_timer);
        s_question_hold_timer = NULL;
    }

    if (!ui_que2) return;

    lv_anim_t a_out;
    lv_anim_init(&a_out);
    lv_anim_set_var(&a_out, ui_que2);
    lv_anim_set_exec_cb(&a_out, anim_set_opa_cb);
    lv_anim_set_values(&a_out, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a_out, 350);
    lv_anim_set_ready_cb(&a_out, question_fade_out_ready_cb);
    lv_anim_start(&a_out);
}

static void question_fade_in_ready_cb(lv_anim_t *a)
{
    (void)a;

    if (s_question_hold_timer) {
        lv_timer_del(s_question_hold_timer);
        s_question_hold_timer = NULL;
    }
    s_question_hold_timer = lv_timer_create(question_hold_timer_cb, UI_QUESTION_HOLD_MS, NULL);
}

static void start_question_sequence(const story_node_t *node)
{
    if (!ui_que2 || !ui_Labelq2 || !node) return;

    lv_label_set_text(ui_Labelq2, node->question ? node->question : "");

    lv_obj_clear_flag(ui_que2, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_que2, LV_OPA_TRANSP);

    if (ui_ContainerCh) lv_obj_add_flag(ui_ContainerCh, LV_OBJ_FLAG_HIDDEN);
    if (ui_arr1)        lv_obj_add_flag(ui_arr1, LV_OBJ_FLAG_HIDDEN);
    if (ui_arr2)        lv_obj_add_flag(ui_arr2, LV_OBJ_FLAG_HIDDEN);

    lv_anim_t a_in;
    lv_anim_init(&a_in);
    lv_anim_set_var(&a_in, ui_que2);
    lv_anim_set_exec_cb(&a_in, anim_set_opa_cb);
    lv_anim_set_values(&a_in, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a_in, UI_FADE_MS);
    lv_anim_set_path_cb(&a_in, lv_anim_path_ease_in_out);
    lv_anim_set_ready_cb(&a_in, question_fade_in_ready_cb);
    lv_anim_start(&a_in);
}

static void start_final_end_sequence(void)
{
    if (!ui_end2 || !ui_Labelend2) return;

    if (ui_que2)        lv_obj_add_flag(ui_que2, LV_OBJ_FLAG_HIDDEN);
    if (ui_ContainerCh) lv_obj_add_flag(ui_ContainerCh, LV_OBJ_FLAG_HIDDEN);
    if (ui_arr1)        lv_obj_add_flag(ui_arr1, LV_OBJ_FLAG_HIDDEN);
    if (ui_arr2)        lv_obj_add_flag(ui_arr2, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(ui_Labelend2, "Try again");

    lv_obj_clear_flag(ui_end2, LV_OBJ_FLAG_HIDDEN);
    _ui_opacity_set(ui_end2, LV_OPA_TRANSP);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui_end2);
    lv_anim_set_exec_cb(&a, anim_set_opa_cb);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, UI_FADE_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    if (s_question_hold_timer) {
        lv_timer_del(s_question_hold_timer);
        s_question_hold_timer = NULL;
    }

    s_outro_mode = true;

    ui_bird1_use_talk_gif();
    if (ui_bird1) {
        lv_obj_set_x(ui_bird1, 352);
        lv_obj_set_y(ui_bird1, -204);
    }

    if (s_outro_tts_timer) {
        lv_timer_del(s_outro_tts_timer);
        s_outro_tts_timer = NULL;
    }
    s_outro_tts_timer = lv_timer_create(outro_tts_timer_cb, UI_OUTRO_DELAY_MS, NULL);
}


static void outro_tts_timer_cb(lv_timer_t *t)
{
    (void)t;

    const char *outro = builtin_get_outro_text();
    if (outro) {
        start_tts_playback_c(outro);
    }

    if (s_outro_tts_timer) {
        lv_timer_del(s_outro_tts_timer);
        s_outro_tts_timer = NULL;
    }
}

static void free_all_images_except(const lv_img_dsc_t* except_s,
                                   const lv_img_dsc_t* except_l)
{
    const void *src1 = NULL;
    const void *src2 = NULL;

    if (ui_Img1) src1 = lv_img_get_src(ui_Img1);
    if (ui_Img2) src2 = lv_img_get_src(ui_Img2);

    for (int i = 0; i < CASE_TXT_COUNT; ++i) {
        const case_visual_t *v = &kVisuals[i];

        const lv_img_dsc_t* arr[2] = { v->img_s, v->img_l };
        for (int j = 0; j < 2; ++j) {
            const lv_img_dsc_t *d = arr[j];
            if (!d) continue;

            if (d == except_s || d == except_l) continue;
            
            if (is_pinned_image(d)) continue;
            
            if ((const void*)d == src1 || (const void*)d == src2) continue;

            if (!d->data) continue;
           
            lv_img_cache_invalidate_src((const void*)d);

            ESP_LOGD(TAG_UI, "free image buffer: %p (case=%d, which=%s)",
                     d->data, i, (j == 0 ? "S" : "L"));

            heap_caps_free((void*)d->data);
            ((lv_img_dsc_t*)d)->data = NULL;
            ((lv_img_dsc_t*)d)->data_size = 0;
        }
    }
}


static void ensure_small_image_for_case(builtin_text_case_t c)
{
    if (c < 0 || c >= CASE_TXT_COUNT) {
        return;
    }

    const case_visual_t *v = &kVisuals[c];

    if (v->img_s && v->img_s->data) {
        if (ui_Img1) {
            lv_img_set_src(ui_Img1, v->img_s);
        }
        return;
    }

    if (v->load_s) {
        ESP_LOGI(TAG_UI, "ensure_small_image_for_case: load SMALL for case %d", (int)c);
        v->load_s();
    }

    if (!v->img_s || !v->img_s->data) {
        ESP_LOGE(TAG_UI,
                 "ensure_small_image_for_case: SMALL image for case %d is not available",
                 (int)c);
        return;
    }

    if (ui_Img1) {
        lv_img_set_src(ui_Img1, v->img_s);
    }

    free_all_images_except(v->img_s, v->img_l);
}

static void apply_image_for_case_internal(builtin_text_case_t c,
                                          bool need_small,
                                          bool need_large)
{
    if (c < 0 || c >= CASE_TXT_COUNT) {
        return;
    }

    const case_visual_t *v = &kVisuals[c];

    const lv_img_dsc_t *img_s = v->img_s;
    const lv_img_dsc_t *img_l = v->img_l;

    if (need_large) {
        if (v->load_l && (!img_l || !img_l->data)) {
            ESP_LOGI(TAG_UI, "apply_image_for_case_internal: load LARGE for case %d", (int)c);
            v->load_l();
            img_l = v->img_l;
        }

        if (!img_l || !img_l->data) {
            ESP_LOGE(TAG_UI,
                     "apply_image_for_case_internal: LARGE image for case %d is not available, keeping previous image",
                     (int)c);
            return;
        }
    }

    if (need_small) {
        if (v->load_s && (!img_s || !img_s->data)) {
            ESP_LOGI(TAG_UI, "apply_image_for_case_internal: load SMALL for case %d", (int)c);
            v->load_s();
            img_s = v->img_s;
        }

        if (!img_s || !img_s->data) {
            ESP_LOGW(TAG_UI,
                     "apply_image_for_case_internal: SMALL image for case %d is not available",
                     (int)c);
        }
    }

    if (need_large && ui_Img2 && img_l && img_l->data) {
        lv_img_set_src(ui_Img2, img_l);
    }
    if (need_small && ui_Img1 && img_s && img_s->data) {
        lv_img_set_src(ui_Img1, img_s);
    }

    const lv_img_dsc_t *keep_s = need_small ? img_s : v->img_s;
    const lv_img_dsc_t *keep_l = need_large ? img_l : v->img_l;
    free_all_images_except(keep_s, keep_l);
}

void apply_image_for_case(builtin_text_case_t c)
{
    apply_image_for_case_internal(c, true, true);
}

void on_btn_change_pressed(lv_event_t * e)
{
    (void)e;
    builtin_text_next();
    builtin_text_case_t c = builtin_text_get();
    ESP_LOGI(TAG_UI, "Built-in text changed to case #%d", (int)c);
    apply_image_for_case(c);   
}

void on_btn_say_pressed(lv_event_t * e)
{
    (void)e;
    const char* text = get_builtin_text();
    start_tts_playback_c(text);
}

static void apply_name_async(void *param)
{
    char *name = (char *)param;
    if (!name) {
        return;
    }

    if (ui_Name) {
        lv_label_set_text(ui_Name, name);
    }

    free(name);
}

void on_text_update_from_uart(const char *text)
{
    if (!text) return;

    size_t len = strlen(text);
    if (len == 0) return;

    while (len > 0 && (text[len - 1] == '\r' || text[len - 1] == '\n')) {
        len--;
    }
    if (len == 0) return;

    if (len > 63) {
        len = 63;
    }

    char *copy = (char *)malloc(len + 1);
    if (!copy) return;

    memcpy(copy, text, len);
    copy[len] = '\0';

    lv_async_call(apply_name_async, copy);
}

static void after_tts_timer_cb(lv_timer_t *t)
{
    (void)t;

    const story_node_t* node = story_get_node(s_current);
    const bool is_final = (node && node->is_final);

    _ui_screen_change(&ui_Screen1,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_SCREEN_FADE_MS,
                      0,
                      &ui_Screen1_screen_init);

    if (is_final) {
        start_final_end_sequence();
    } else {
        start_question_sequence(node);
    }

    if (s_after_tts_timer) {
        lv_timer_del(s_after_tts_timer);
        s_after_tts_timer = NULL;
    }
}


static void ui_notify_tts_finished_async(void *arg)
{
    LV_UNUSED(arg);

    if (s_settings_open) {
        return;
    }

    if (s_intro_mode) {
        s_intro_mode = false;
        show_case(STORY_START_CASE);
        return;
    }

    if (s_outro_mode) {
    s_outro_mode = false;

    ui_bird1_use_idle_gif();
    if (ui_bird1) {
        lv_obj_set_x(ui_bird1, 342);
        lv_obj_set_y(ui_bird1, -204);
    }
    return;
    }

    ensure_small_image_for_case(s_current);

    if (s_after_tts_timer) {
        lv_timer_reset(s_after_tts_timer);
    } else {
        s_after_tts_timer = lv_timer_create(after_tts_timer_cb, 1000, NULL);
    }

}

void ui_notify_tts_finished(void)
{
    lv_async_call(ui_notify_tts_finished_async, NULL);
}

void button_choose_1(lv_event_t * e)
{
    (void)e;
    const story_node_t* node = story_get_node(s_current);
    if (!node || node->is_final) return;
    show_case(node->next1);
}

void button_choose_2(lv_event_t * e)
{
    (void)e;
    const story_node_t* node = story_get_node(s_current);
    if (!node || node->is_final) return;
    show_case(node->next2);
}

static void tts_timer_cb(lv_timer_t* t)
{
    (void)t;

    const char *text = NULL;
    if (s_intro_mode) {
        text = builtin_get_intro_text();
    } else {
        text = get_builtin_text();
    }

    if (text) {
        start_tts_playback_c(text);
    }

    if (s_tts_timer) {
        lv_timer_del(s_tts_timer);
        s_tts_timer = NULL;
    }
}

static void schedule_tts_after_delay(void)
{
    if (!s_tts_timer) {
        s_tts_timer = lv_timer_create(tts_timer_cb, 1000 /* ms */, NULL);
        lv_timer_pause(s_tts_timer);
    }

    lv_timer_set_period(s_tts_timer, 1000);
    lv_timer_reset(s_tts_timer);
    lv_timer_resume(s_tts_timer);
}

static void show_case(builtin_text_case_t c)
{
    s_current = c;
    builtin_text_set(c);    

    if (s_question_hold_timer) {
        lv_timer_del(s_question_hold_timer);
        s_question_hold_timer = NULL;
    }

    apply_image_for_case_internal(c, false, true);

    if (ui_que2)        lv_obj_add_flag(ui_que2, LV_OBJ_FLAG_HIDDEN);
    if (ui_ContainerCh) lv_obj_add_flag(ui_ContainerCh, LV_OBJ_FLAG_HIDDEN);
    if (ui_arr1)        lv_obj_add_flag(ui_arr1, LV_OBJ_FLAG_HIDDEN);
    if (ui_arr2)        lv_obj_add_flag(ui_arr2, LV_OBJ_FLAG_HIDDEN);
    if (ui_end2)        lv_obj_add_flag(ui_end2, LV_OBJ_FLAG_HIDDEN);

    schedule_tts_after_delay();
}

void ui_story_start(void)
{
    show_case(STORY_START_CASE);
}

void end_event(lv_event_t * e)
{
    (void)e;

    if (s_outro_tts_timer) {
        lv_timer_del(s_outro_tts_timer);
        s_outro_tts_timer = NULL;
        s_outro_mode = false;
    }

    tts_stop_playback();      

    ui_bird1_use_idle_gif();
    if (ui_bird1) {
        lv_obj_set_x(ui_bird1, 342);
        lv_obj_set_y(ui_bird1, -204);
    }

    if (s_tts_timer) {
        lv_timer_del(s_tts_timer);
        s_tts_timer = NULL;
    }
    if (s_after_tts_timer) {
        lv_timer_del(s_after_tts_timer);
        s_after_tts_timer = NULL;
    }
    if (s_question_hold_timer) {
        lv_timer_del(s_question_hold_timer);
        s_question_hold_timer = NULL;
    }

    show_case(STORY_START_CASE);
    _ui_screen_change(&ui_Screen2,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_AGAIN_MS,
                      0,
                      &ui_Screen2_screen_init);
}


void ui_handle_choice1(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    button_choose_1(e);

    _ui_screen_change(&ui_Screen2,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_CHOICE_MS,
                      0,
                      &ui_Screen2_screen_init);
}

void ui_handle_choice2(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    button_choose_2(e);

    _ui_screen_change(&ui_Screen2,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_CHOICE_MS,
                      0,
                      &ui_Screen2_screen_init);
}


void ui_handle_settings_from_screen1(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }

    s_last_active_screen = 1;
    s_settings_open      = true;

    pause_question_and_choice_animations();

    if (s_outro_mode) {
        tts_stop_playback();

        if (s_outro_tts_timer) {
            lv_timer_del(s_outro_tts_timer);
            s_outro_tts_timer = NULL;
        }
    }

    _ui_screen_change(&ui_Screen3,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_SCREEN_3_MS,
                      0,
                      &ui_Screen3_screen_init);
}


void ui_handle_settings_from_screen2(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }

    s_last_active_screen = 2;
    s_settings_open      = true;

    tts_stop_playback();  

    if (s_tts_timer) {
        lv_timer_del(s_tts_timer);
        s_tts_timer = NULL;
    }
    if (s_after_tts_timer) {
        lv_timer_del(s_after_tts_timer);
        s_after_tts_timer = NULL;
    }

    _ui_screen_change(&ui_Screen3,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_SCREEN_3_MS,
                      0,
                      &ui_Screen3_screen_init);
}


void ui_handle_settings_back_from_screen3(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }

    s_settings_open = false;

    if (s_last_active_screen == 1) {
    _ui_screen_change(&ui_Screen1,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_SCREEN_3_MS,
                      0,
                      &ui_Screen1_screen_init);

    resume_question_and_choice_animations();

    if (s_outro_mode && s_outro_tts_timer == NULL) {
        s_outro_tts_timer = lv_timer_create(outro_tts_timer_cb, UI_OUTRO_DELAY_MS, NULL);
    }
} else if (s_last_active_screen == 2) {
    _ui_screen_change(&ui_Screen2,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      UI_SCREEN_3_MS,
                      0,
                      &ui_Screen2_screen_init);
    schedule_tts_after_delay();
    }
    else {
        _ui_screen_change(&ui_Screen2,
                          LV_SCR_LOAD_ANIM_FADE_ON,
                          UI_SCREEN_3_MS,
                          0,
                          &ui_Screen2_screen_init);
    }
}

void reload_app(lv_event_t * e)
{
    (void)e;
    esp_restart();
}

void save_name(lv_event_t * e)
{
    (void)e;

    if (!ui_Name) {
        return;
    }

    const char *text = lv_label_get_text(ui_Name);
    if (!text) {
        return;
    }

    char buf[USER_NAME_MAX_LEN + 1];
    size_t len = strnlen(text, USER_NAME_MAX_LEN);
    memcpy(buf, text, len);
    buf[len] = '\0';

    user_name_set(buf);
}

