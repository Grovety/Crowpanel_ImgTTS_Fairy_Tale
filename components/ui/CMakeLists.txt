cmake_minimum_required(VERSION 3.16)

set(UI_SCENARIO_NAME       "fairy_tale")
set(UI_SCENARIO_NAME_LOWER "fairy_tale")

set(ASSETS_S_SUBDIR "assets_s")   
set(ASSETS_L_SUBDIR "assets_l")   

message(STATUS
  "UI scenario fixed to: ${UI_SCENARIO_NAME} "
  "(assets_s: ${ASSETS_S_SUBDIR}, assets_l: ${ASSETS_L_SUBDIR})"
)

set(UI_COMMON_SRCS
  ui_Screen1.c
  ui_Screen2.c
  ui_Screen3.c
  ui.c
  ui_helpers.c
  ui_events.c
  ui_font_Font1.c
  ui_font_Font2.c
  ui_img_manager.c
  ui_img_loading_gif.c
  tts_bridge_impl.c
  story_fairy_tale.c
  builtin_texts_fairy_tale.c
  user_name_store.c
)

file(GLOB UI_SCREEN_SRCS
  "${CMAKE_CURRENT_LIST_DIR}/static_icons/*.c"
)

file(GLOB_RECURSE UI_FAIRY_SRCS
  "${CMAKE_CURRENT_LIST_DIR}/fairy_tale/*.c"
)

list(APPEND UI_COMMON_SRCS
  ${UI_SCREEN_SRCS}
  ${UI_FAIRY_SRCS}
)

list(REMOVE_DUPLICATES UI_COMMON_SRCS)

idf_component_register(
  SRCS
    ${UI_COMMON_SRCS}
  INCLUDE_DIRS
    "include"
    "fairy_tale"
  REQUIRES
    lvgl__lvgl
    espressif__esp32_display_panel
    gifdec
    zlib
  PRIV_REQUIRES
    esp_timer
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
  UI_SCENARIO_NAME=${UI_SCENARIO_NAME}
  UI_SCENARIO_NAME_LOWER=\"${UI_SCENARIO_NAME_LOWER}\"
  ASSETS_S_SUBDIR=\"${ASSETS_S_SUBDIR}\"
  ASSETS_L_SUBDIR=\"${ASSETS_L_SUBDIR}\"
  ASSETS_SUBDIR=\"${ASSETS_S_SUBDIR}\"
)
