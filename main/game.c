// The device loop: keys -> ui -> sim ticks -> lift animation -> dirty-region
// paint, paced to 30 fps. Saves to NVS namespace "tower".
#include "game.h"

#include <stdio.h>

#include "boot.h"
#include "display.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gfx.h"
#include "input.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "paint.h"
#include "sound.h"
#include "tower.h"
#include "ui.h"

static const char *TAG = "tower";
static nvs_handle_t s_nvs;
static bool s_nvs_ok;
static uint8_t s_blob[SAVE_MAX];   // static: no malloc, ever
static uint8_t s_slot;             // last-used slot (0 or 1); autosave writes here
static bool s_has[2];

static const char *slot_key(int slot) { return slot ? "s1" : "s0"; }

static void save_slot(int slot)
{
    if (!s_nvs_ok || slot < 0 || slot > 1) return;
    int64_t t0 = esp_timer_get_time();
    int n = tw_save(s_blob);
    esp_err_t e = nvs_set_blob(s_nvs, slot_key(slot), s_blob, (size_t)n);
    if (e == ESP_OK) { s_slot = (uint8_t)slot; e = nvs_set_u8(s_nvs, "slot", s_slot); }
    if (e == ESP_OK) e = nvs_commit(s_nvs);
    if (e != ESP_OK) { ESP_LOGW(TAG, "save %d failed (%d B): %s", slot + 1, n, esp_err_to_name(e)); tw_note("SAVE FAILED: NVS FULL?"); }
    else { s_has[slot] = true; char b[40]; snprintf(b, sizeof b, "SAVED TO SLOT %d", slot + 1); tw_note(b);
           ESP_LOGI(TAG, "saved slot %d, %d B in %d ms", slot + 1, n, (int)((esp_timer_get_time() - t0) / 1000)); }
}
static bool load_slot(int slot)
{
    if (!s_nvs_ok || slot < 0 || slot > 1 || !s_has[slot]) return false;
    size_t n = sizeof s_blob;
    if (nvs_get_blob(s_nvs, slot_key(slot), s_blob, &n) != ESP_OK) return false;
    bool ok = tw_load(s_blob, (int)n);
    if (ok) { s_slot = (uint8_t)slot; if (s_nvs_ok) { nvs_set_u8(s_nvs, "slot", s_slot); nvs_commit(s_nvs); } }
    ESP_LOGI(TAG, "load slot %d, %u B: %s", slot + 1, (unsigned)n, ok ? "ok" : "bad");
    return ok;
}
static void save(void) { save_slot(s_slot); }          // autosave -> last-used
static bool load(void) { return load_slot(s_slot); }  // title CONTINUE -> last-used
static void slot_info(int slot, char *out, int n)
{
    if (slot < 0 || slot > 1 || !s_has[slot] || !s_nvs_ok) { snprintf(out, n, "EMPTY"); return; }
    size_t len = sizeof s_blob;
    if (nvs_get_blob(s_nvs, slot_key(slot), s_blob, &len) != ESP_OK) { snprintf(out, n, "EMPTY"); return; }
    uint32_t day = 0; uint8_t star = 0; int64_t funds = 0;
    tw_save_meta(s_blob, (int)len, &day, &star, &funds);
    if (funds >= 1000000) snprintf(out, n, "DAY %u *%u $%dM", (unsigned)(day + 1), star, (int)(funds / 1000000));
    else snprintf(out, n, "DAY %u *%u $%dK", (unsigned)(day + 1), star, (int)(funds / 1000));
}
static int last_slot(void) { return s_slot; }

static void sfx(int w)
{
    static const sfx_t map[4] = {SFX_SELECT, SFX_LAND, SFX_LOCKED, SFX_UNLOCK};
    sound_play(map[w & 3]);
}
static void sound_set(int mode)
{
    sound_set_mode((sound_mode_t)mode);
    if (s_nvs_ok) { nvs_set_u8(s_nvs, "snd", (uint8_t)mode); nvs_commit(s_nvs); }
}
static void send(int x, int y, int w, const uint16_t *buf, int rows) { display_send_rect(x, y, w, buf, rows); }

void game_run(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_OK && nvs_open("tower", NVS_READWRITE, &s_nvs) == ESP_OK) {
        s_nvs_ok = true;
        size_t n = 0;
        /* migrate the old single "save" key into slot 0 once */
        if (nvs_get_blob(s_nvs, "save", NULL, &n) == ESP_OK && n > 0 &&
            nvs_get_blob(s_nvs, "s0", NULL, &n) != ESP_OK) {
            n = sizeof s_blob;
            if (nvs_get_blob(s_nvs, "save", s_blob, &n) == ESP_OK) {
                nvs_set_blob(s_nvs, "s0", s_blob, n); nvs_erase_key(s_nvs, "save"); nvs_commit(s_nvs);
                ESP_LOGI(TAG, "migrated legacy save -> slot 1");
            }
        }
        for (int i = 0; i < 2; i++) { n = 0; s_has[i] = nvs_get_blob(s_nvs, slot_key(i), NULL, &n) == ESP_OK && n > 0; }
        nvs_get_u8(s_nvs, "slot", &s_slot); if (s_slot > 1) s_slot = 0;
        if (!s_has[s_slot]) s_slot = s_has[0] ? 0 : s_has[1] ? 1 : 0;
        nvs_stats_t st;
        if (nvs_get_stats(NULL, &st) == ESP_OK)
            ESP_LOGI(TAG, "nvs used %u free %u; slots %d/%d; last %d (full save ~%u entries)", (unsigned)st.used_entries,
                     (unsigned)st.free_entries, s_has[0], s_has[1], s_slot + 1, (unsigned)(SAVE_MAX / 32 + 8));
    } else ESP_LOGW(TAG, "nvs: %s; progress not kept", esp_err_to_name(err));
    uint8_t snd = 0;
    if (s_nvs_ok) nvs_get_u8(s_nvs, "snd", &snd);
    gfx_init();
    sound_init((sound_mode_t)(snd % 3));
    tw_new((uint32_t)esp_timer_get_time() ^ 0x9e3779b9u);
    g_ui.sound = (uint8_t)(snd % 3);
    ui_init();
    g_ui.load_cb = (s_has[0] || s_has[1]) ? load : NULL;
    g_ui.save_cb = save;
    g_ui.load_slot_cb = load_slot;
    g_ui.save_slot_cb = save_slot;
    g_ui.slot_info_cb = slot_info;
    g_ui.last_slot_cb = last_slot;
    g_ui.sfx_cb = sfx;
    g_ui.sound_cb = sound_set;
    int music = -1;
    ESP_LOGI(TAG, "SKYRISE: tower_t %u B, ui %u B, save max %u B, free heap %u", (unsigned)sizeof g_tw, (unsigned)sizeof g_ui,
             (unsigned)SAVE_MAX, (unsigned)esp_get_free_heap_size());

    int64_t next = esp_timer_get_time(), stat_t0 = next, sim_us = 0, draw_us = 0, draw_max = 0, px = 0;
    int frames = 0, late = 0, ticks = 0;
    bool full = true;
    for (;;) {
        input_state_t in = input_take();
        ui_keys(in.up, in.down, in.ok);
        int64_t t0 = esp_timer_get_time();
        ticks += ui_frame();
        tw_animate();
        int want = ui_music();   /* day / night loop; changes at 06:00 and 19:00 */
        if (want != music) { music = want; sound_music((music_t)want); }
        int64_t t1 = esp_timer_get_time();
        px += paint_frame(send, full);
        full = false;
        int64_t t2 = esp_timer_get_time();
        sim_us += t1 - t0; draw_us += t2 - t1;
        if (t2 - t1 > draw_max) draw_max = t2 - t1;
        frames++;
        next += FRAME_US;
        if (esp_timer_get_time() > next) late++;
        while (esp_timer_get_time() < next) vTaskDelay(1);
        if (esp_timer_get_time() - next > 100000) next = esp_timer_get_time();
        int64_t now = esp_timer_get_time();
        if (now - stat_t0 >= 10000000) {
            ESP_LOGI(TAG, "fps %.1f late %d | sim avg %d us (%d ticks) | paint avg %d us max %d us, %d px/frame | rooms %d shafts %d groups %d/%d peak %ld | free heap %u min %u",
                     frames * 1e6 / (double)(now - stat_t0), late, (int)(sim_us / frames), ticks / frames, (int)(draw_us / frames), (int)draw_max,
                     (int)(px / frames), g_tw.n_units, g_tw.n_shafts, g_tw.n_groups, MAX_GROUPS, (long)g_tw.st.peak_groups,
                     (unsigned)esp_get_free_heap_size(), (unsigned)esp_get_minimum_free_heap_size());
            stat_t0 = now; frames = late = ticks = 0; sim_us = draw_us = draw_max = px = 0;
        }
    }
}
