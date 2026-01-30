#include "ui.h"
#include "ui_events.h"      
#include "builtin_texts.h" 

lv_obj_t * ui_Screen1 = NULL;
lv_obj_t * ui_Img1 = NULL;
lv_obj_t * ui_ContainerCh = NULL;
lv_obj_t * ui_ch1 = NULL;
lv_obj_t * ui_Labelch1 = NULL;
lv_obj_t * ui_ch2 = NULL;
lv_obj_t * ui_Labelch2 = NULL;
lv_obj_t * ui_arr1 = NULL;
lv_obj_t * ui_arr2 = NULL;
lv_obj_t * ui_bird1 = NULL;
lv_obj_t * ui_frame1 = NULL;
lv_obj_t * ui_que2 = NULL;
lv_obj_t * ui_Labelq2 = NULL;
lv_obj_t * ui_sett1 = NULL;
lv_obj_t * ui_end2 = NULL;
lv_obj_t * ui_Labelend2 = NULL;

void ui_event_ch1(lv_event_t * e) { ui_handle_choice1(e); }
void ui_event_ch2(lv_event_t * e) { ui_handle_choice2(e); }


void ui_event_sett1(lv_event_t * e)
{
    ui_handle_settings_from_screen1(e);
}

void ui_event_end2(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_CLICKED) {
        end_event(e);
    }
}

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_src(ui_Screen1, &ui_img_night_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_opa(ui_Screen1, 70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_Screen1, 35, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(ui_Screen1, 25, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Img1 = lv_img_create(ui_Screen1);
    lv_obj_set_width(ui_Img1, 458);
    lv_obj_set_height(ui_Img1, 263);
    lv_obj_set_x(ui_Img1, 1);
    lv_obj_set_y(ui_Img1, -78);
    lv_obj_set_align(ui_Img1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Img1, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Img1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_Img1, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Img1, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_Img1, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ContainerCh = lv_obj_create(ui_Screen1);
    lv_obj_remove_style_all(ui_ContainerCh);
    lv_obj_set_width(ui_ContainerCh, 821);
    lv_obj_set_height(ui_ContainerCh, 154);
    lv_obj_set_x(ui_ContainerCh, 1);
    lv_obj_set_y(ui_ContainerCh, 180);
    lv_obj_set_align(ui_ContainerCh, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_ContainerCh, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_ContainerCh, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ui_ContainerCh, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_pad_left(ui_ContainerCh, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_ContainerCh, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_ContainerCh, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_ContainerCh, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(ui_ContainerCh, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ui_ContainerCh, 155, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ch1 = lv_imgbtn_create(ui_ContainerCh);
    lv_imgbtn_set_src(ui_ch1, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_choice_png, NULL);
    lv_imgbtn_set_src(ui_ch1, LV_IMGBTN_STATE_PRESSED, NULL, &ui_img_choice_png, NULL);
    lv_obj_set_height(ui_ch1, 114);
    lv_obj_set_width(ui_ch1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_x(ui_ch1, 237);
    lv_obj_set_y(ui_ch1, 161);
    lv_obj_set_align(ui_ch1, LV_ALIGN_CENTER);

    ui_Labelch1 = lv_label_create(ui_ch1);
    lv_obj_set_width(ui_Labelch1, 241);
    lv_obj_set_height(ui_Labelch1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Labelch1, 0);
    lv_obj_set_y(ui_Labelch1, 7);
    lv_obj_set_align(ui_Labelch1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Labelch1, "");
    lv_obj_set_style_text_color(ui_Labelch1, lv_color_hex(0xF3F0A7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Labelch1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_Labelch1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Labelch1, &ui_font_Font2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ch2 = lv_imgbtn_create(ui_ContainerCh);
    lv_imgbtn_set_src(ui_ch2, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_choice_png, NULL);
    lv_imgbtn_set_src(ui_ch2, LV_IMGBTN_STATE_PRESSED, NULL, &ui_img_choice_png, NULL);
    lv_obj_set_height(ui_ch2, 114);
    lv_obj_set_width(ui_ch2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_x(ui_ch2, 0);
    lv_obj_set_y(ui_ch2, -1);
    lv_obj_set_align(ui_ch2, LV_ALIGN_CENTER);

    ui_Labelch2 = lv_label_create(ui_ch2);
    lv_obj_set_width(ui_Labelch2, 241);
    lv_obj_set_height(ui_Labelch2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Labelch2, 0);
    lv_obj_set_y(ui_Labelch2, 7);
    lv_obj_set_align(ui_Labelch2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Labelch2, "");
    lv_obj_set_style_text_color(ui_Labelch2, lv_color_hex(0xF3F0A7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Labelch2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_Labelch2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Labelch2, &ui_font_Font2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_arr1 = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_arr1, &ui_img_arrow_png);
    lv_obj_set_width(ui_arr1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_arr1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_arr1, -211);
    lv_obj_set_y(ui_arr1, 102);
    lv_obj_set_align(ui_arr1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_arr1, LV_OBJ_FLAG_ADV_HITTEST | LV_OBJ_FLAG_IGNORE_LAYOUT);     /// Flags
    lv_obj_clear_flag(ui_arr1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_angle(ui_arr1, 1400);

    ui_arr2 = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_arr2, &ui_img_arrow_png);
    lv_obj_set_width(ui_arr2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_arr2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_arr2, 208);
    lv_obj_set_y(ui_arr2, 104);
    lv_obj_set_align(ui_arr2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_arr2, LV_OBJ_FLAG_ADV_HITTEST | LV_OBJ_FLAG_IGNORE_LAYOUT);     /// Flags
    lv_obj_clear_flag(ui_arr2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_angle(ui_arr2, 450);

    ui_bird1 = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_bird1, &ui_img_bird_png);
    lv_obj_set_width(ui_bird1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_bird1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_bird1, 342);
    lv_obj_set_y(ui_bird1, -204);
    lv_obj_set_align(ui_bird1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_bird1, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_bird1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_frame1 = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_frame1, &ui_img_frame_sml_png);
    lv_obj_set_width(ui_frame1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_frame1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_frame1, 2);
    lv_obj_set_y(ui_frame1, -79);
    lv_obj_set_align(ui_frame1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_frame1, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_frame1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_que2 = lv_img_create(ui_Screen1);
    lv_img_set_src(ui_que2, &ui_img_question_png);
    lv_obj_set_width(ui_que2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_que2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_que2, 0);
    lv_obj_set_y(ui_que2, 155);
    lv_obj_set_align(ui_que2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_que2, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_que2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Labelq2 = lv_label_create(ui_que2);
    lv_obj_set_width(ui_Labelq2, 252);
    lv_obj_set_height(ui_Labelq2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Labelq2, 1);
    lv_obj_set_y(ui_Labelq2, 6);
    lv_obj_set_align(ui_Labelq2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Labelq2, "");
    lv_obj_set_style_text_color(ui_Labelq2, lv_color_hex(0xF3F0A7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Labelq2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_Labelq2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Labelq2, &ui_font_Font1, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_sett1 = lv_imgbtn_create(ui_Screen1);
    lv_imgbtn_set_src(ui_sett1, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_sett_off_png, NULL);
    lv_imgbtn_set_src(ui_sett1, LV_IMGBTN_STATE_PRESSED, NULL, &ui_img_sett_off_png, NULL);
    lv_obj_set_width(ui_sett1, 67);
    lv_obj_set_height(ui_sett1, 66);
    lv_obj_set_x(ui_sett1, -365);
    lv_obj_set_y(ui_sett1, -223);
    lv_obj_set_align(ui_sett1, LV_ALIGN_CENTER);

     ui_end2 = lv_imgbtn_create(ui_Screen1);
    lv_imgbtn_set_src(ui_end2, LV_IMGBTN_STATE_RELEASED, NULL, &ui_img_question_png, NULL);
    lv_imgbtn_set_src(ui_end2, LV_IMGBTN_STATE_PRESSED, NULL, &ui_img_question_png, NULL);
    lv_obj_set_height(ui_end2, 121);
    lv_obj_set_width(ui_end2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_x(ui_end2, 0);
    lv_obj_set_y(ui_end2, 155);
    lv_obj_set_align(ui_end2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_end2, LV_OBJ_FLAG_HIDDEN);     /// Flags

    ui_Labelend2 = lv_label_create(ui_end2);
    lv_obj_set_width(ui_Labelend2, 252);
    lv_obj_set_height(ui_Labelend2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Labelend2, 1);
    lv_obj_set_y(ui_Labelend2, 6);
    lv_obj_set_align(ui_Labelend2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Labelend2, "");
    lv_obj_set_style_text_color(ui_Labelend2, lv_color_hex(0xF3F0A7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Labelend2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_Labelend2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Labelend2, &ui_font_Font1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_ch1, ui_event_ch1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ch2, ui_event_ch2, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_sett1, ui_event_sett1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_end2, ui_event_end2, LV_EVENT_ALL, NULL);

}

void ui_Screen1_screen_destroy(void)
{
    if(ui_Screen1) lv_obj_del(ui_Screen1);

    // NULL screen variables
    ui_Screen1 = NULL;
    ui_Img1 = NULL;
    ui_ContainerCh = NULL;
    ui_ch1 = NULL;
    ui_Labelch1 = NULL;
    ui_ch2 = NULL;
    ui_Labelch2 = NULL;
    ui_arr1 = NULL;
    ui_arr2 = NULL;
    ui_bird1 = NULL;
    ui_frame1 = NULL;
    ui_que2 = NULL;
    ui_Labelq2 = NULL;
    ui_sett1 = NULL;
    ui_end2 = NULL;
    ui_Labelend2 = NULL;
}
