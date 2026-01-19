#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void start_tts_playback_c(const char *text);

void tts_stop_playback();

void tts_pause_playback(void);
void tts_resume_playback(void);

typedef void (*start_tts_cb_t)(const char *);
void register_start_tts_cb(start_tts_cb_t cb);

#ifdef __cplusplus
}
#endif
