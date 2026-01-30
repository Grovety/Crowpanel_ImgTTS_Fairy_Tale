#include "ui.h"
#include "ui_events.h"      
#include "builtin_texts.h"     

lv_obj_t * ui_Screen2 = NULL;
lv_obj_t * ui_Img2 = NULL;
lv_obj_t * ui_bird2 = NULL;
lv_obj_t * ui_frame2 = NULL;
lv_obj_t * ui_sett2 = NULL;
// event funtions
void ui_event_sett2(lv_event_t * e)
{
    ui_handle_settings_from_screen2(e);
}


// build funtions

void ui_Screen2_screen_init(void)
{
    ui_Screen2 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_src(ui_Screen2, &ui_img_night_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_opa(ui_Screen2, 70, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Img2 = lv_img_create(ui_Screen2);
    lv_obj_set_width(ui_Img2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Img2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Img2, 0);
    lv_obj_set_y(ui_Img2, 4);
    lv_obj_set_align(ui_Img2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Img2, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Img2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_Img2, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Img2, lv_color_hex(0xF3F0A7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Img2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_Img2, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_bird2 = lv_img_create(ui_Screen2);
    lv_img_set_src(ui_bird2, &ui_img_bird_png);
    lv_obj_set_width(ui_bird2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_bird2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_bird2, 353);
    lv_obj_set_y(ui_bird2, -187);
    lv_obj_set_align(ui_bird2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_bird2, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_bird2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_frame2 = lv_img_create(ui_Screen2);
    lv_img_set_src(ui_frame2, &ui_img_frame_lrg_png);
    lv_obj_set_width(ui_frame2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_frame2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_frame2, 1);
    lv_obj_set_y(ui_frame2, 5);
    lv_obj_set_align(ui_frame2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_frame2, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_frame2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_sett2 = lv_imgbtn_create(ui_Screen2);
    lv_imgbtn_set_src(ui_sett2, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_sett_off_png, NULL);
    lv_imgbtn_set_src(ui_sett2, LV_IMGBTN_STATE_PRESSED, NULL, &ui_img_sett_off_png, NULL);
    lv_obj_set_width(ui_sett2, 67);
    lv_obj_set_height(ui_sett2, 66);
    lv_obj_set_x(ui_sett2, -365);
    lv_obj_set_y(ui_sett2, -205);
    lv_obj_set_align(ui_sett2, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(ui_sett2, ui_event_sett2, LV_EVENT_ALL, NULL);

}

void ui_Screen2_screen_destroy(void)
{
    if(ui_Screen2) lv_obj_del(ui_Screen2);

    // NULL screen variables
    ui_Screen2 = NULL;
    ui_Img2 = NULL;
    ui_bird2 = NULL;
    ui_frame2 = NULL;
    ui_sett2 = NULL;

}
