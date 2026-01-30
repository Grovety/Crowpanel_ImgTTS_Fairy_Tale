cmake_minimum_required(VERSION 3.5)

set(COMPONENTS "main")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(proj)

set(SPIFFS_SRC_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/spiffs_root")
set(SPIFFS_STAGE_DIR "${CMAKE_BINARY_DIR}/spiffs_stage")

foreach(subdir assets assets_s assets_l)
  if(NOT EXISTS "${SPIFFS_SRC_DIR}/${subdir}")
    message(FATAL_ERROR "Missing folder: ${SPIFFS_SRC_DIR}/${subdir}")
  endif()
endforeach()

add_custom_command(
  OUTPUT "${SPIFFS_STAGE_DIR}/.prepared"
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${SPIFFS_STAGE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SPIFFS_STAGE_DIR}"

  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${SPIFFS_SRC_DIR}/assets"
          "${SPIFFS_STAGE_DIR}/assets"

  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${SPIFFS_SRC_DIR}/assets_s"
          "${SPIFFS_STAGE_DIR}/assets_s"

  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${SPIFFS_SRC_DIR}/assets_l"
          "${SPIFFS_STAGE_DIR}/assets_l"

  COMMAND ${CMAKE_COMMAND} -E touch "${SPIFFS_STAGE_DIR}/.prepared"
  VERBATIM
)

add_custom_target(spiffs_stage ALL DEPENDS "${SPIFFS_STAGE_DIR}/.prepared")

spiffs_create_partition_image(spiffs "${SPIFFS_STAGE_DIR}" FLASH_IN_PROJECT)

add_dependencies(spiffs_spiffs_bin spiffs_stage)
