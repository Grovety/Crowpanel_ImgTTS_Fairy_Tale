#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*uart_json_on_text_cb_t)(const char *text);

int uart_json_init(uart_json_on_text_cb_t on_text);

#ifdef __cplusplus
}
#endif
