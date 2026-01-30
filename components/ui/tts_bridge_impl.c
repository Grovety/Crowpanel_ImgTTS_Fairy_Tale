#include "tts_bridge.h"
#include "ui.h"   

static start_tts_cb_t g_cb = NULL;

extern void stop_tts_playback_impl(void);
extern void pause_tts_playback_impl(void);
extern void resume_tts_playback_impl(void);

void start_tts_playback_c(const char *text)
{
    if (!text || !g_cb) {
        return;
    }

    ui_bird_talk_anim_start();

    g_cb(text);
}

void register_start_tts_cb(start_tts_cb_t cb)
{
    g_cb = cb;
}

void tts_stop_playback(void)
{
    stop_tts_playback_impl();

    ui_bird_talk_anim_stop();
}

void tts_pause_playback(void)
{
    pause_tts_playback_impl();

    ui_bird_talk_anim_stop();
}

void tts_resume_playback(void)
{
    resume_tts_playback_impl();

    ui_bird_talk_anim_start();
}
