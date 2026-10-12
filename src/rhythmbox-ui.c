#include <sys/mount.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/soundcard.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#define W 240
#define H 432

static uint32_t canvas[W * H];
static int fb_fd = -1;
static uint8_t *fb_mem = NULL;
static size_t fb_size = 0;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;

static const uint8_t font5x7[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5f,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7f,0x14,0x7f,0x14}, {0x24,0x2a,0x7f,0x2a,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, {0x00,0x1c,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1c,0x00}, {0x14,0x08,0x3e,0x08,0x14}, {0x08,0x08,0x3e,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02}, {0x3e,0x51,0x49,0x45,0x3e}, {0x00,0x42,0x7f,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4b,0x31}, {0x18,0x14,0x12,0x7f,0x10},
    {0x27,0x45,0x45,0x45,0x39}, {0x3c,0x4a,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1e}, {0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00}, {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, {0x32,0x49,0x79,0x41,0x3e},
    {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36}, {0x3e,0x41,0x41,0x41,0x22},
    {0x7f,0x41,0x41,0x22,0x1c}, {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01},
    {0x3e,0x41,0x49,0x49,0x7a}, {0x7f,0x08,0x08,0x08,0x7f}, {0x00,0x41,0x7f,0x41,0x00},
    {0x20,0x40,0x41,0x3f,0x01}, {0x7f,0x08,0x14,0x22,0x41}, {0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f}, {0x3e,0x41,0x41,0x41,0x3e},
    {0x7f,0x09,0x09,0x09,0x06}, {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7f,0x01,0x01}, {0x3f,0x40,0x40,0x40,0x3f},
    {0x1f,0x20,0x40,0x20,0x1f}, {0x3f,0x40,0x38,0x40,0x3f}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7f,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7f,0x00}, {0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40}, {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78},
    {0x7f,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20}, {0x38,0x44,0x44,0x48,0x7f},
    {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7e,0x09,0x01,0x02}, {0x0c,0x52,0x52,0x52,0x3e},
    {0x7f,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7d,0x40,0x00}, {0x20,0x40,0x44,0x3d,0x00},
    {0x7f,0x10,0x28,0x44,0x00}, {0x00,0x41,0x7f,0x40,0x00}, {0x7c,0x04,0x18,0x04,0x78},
    {0x7c,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38}, {0x7c,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7c}, {0x7c,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3f,0x44,0x40,0x20}, {0x3c,0x40,0x40,0x20,0x7c}, {0x1c,0x20,0x40,0x20,0x1c},
    {0x3c,0x40,0x30,0x40,0x3c}, {0x44,0x28,0x10,0x28,0x44}, {0x0c,0x50,0x50,0x50,0x3c},
    {0x44,0x64,0x54,0x4c,0x44}, {0x00,0x08,0x36,0x41,0x00}, {0x00,0x00,0x7f,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00}, {0x10,0x08,0x08,0x10,0x08}
};

static inline void put_px(int x, int y, uint32_t c) {
    if ((unsigned)x < W && (unsigned)y < H)
        canvas[y * W + x] = c;
}

static void fill_rect(int x, int y, int w, int h, uint32_t c) {
    for (int j = y; j < y + h; j++) {
        if ((unsigned)j >= H) continue;
        for (int i = x; i < x + w; i++) {
            if ((unsigned)i < W)
                canvas[j * W + i] = c;
        }
    }
}

static void draw_rect(int x, int y, int w, int h, uint32_t c) {
    fill_rect(x, y, w, 1, c);
    fill_rect(x, y + h - 1, w, 1, c);
    fill_rect(x, y, 1, h, c);
    fill_rect(x + w - 1, y, 1, h, c);
}

static void draw_char(int x, int y, char ch, uint32_t c, int scale) {
    if (ch < 32 || ch > 126) ch = '?';
    const uint8_t *glyph = font5x7[ch - 32];
    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row))
                fill_rect(x + col * scale, y + row * scale, scale, scale, c);
        }
    }
}

static void draw_str(int x, int y, const char *s, uint32_t c, int scale) {
    while (*s) {
        draw_char(x, y, *s++, c, scale);
        x += 6 * scale;
    }
}

static void flush_fb(void) {
    if (!fb_mem) return;
    int bpp = vinfo.bits_per_pixel;
    int stride = finfo.line_length;
    for (int y = 0; y < H && y < (int)vinfo.yres; y++) {
        uint8_t *row = fb_mem + y * stride;
        for (int x = 0; x < W && x < (int)vinfo.xres; x++) {
            uint32_t c = canvas[y * W + x];
            if (bpp == 32) {
                ((uint32_t *)row)[x] = c;
            } else if (bpp == 24) {
                row[x * 3 + 0] = c & 0xff;
                row[x * 3 + 1] = (c >> 8) & 0xff;
                row[x * 3 + 2] = (c >> 16) & 0xff;
            } else if (bpp == 16) {
                uint16_t r = (c >> 19) & 0x1f;
                uint16_t g = (c >> 10) & 0x3f;
                uint16_t b = (c >> 3) & 0x1f;
                ((uint16_t *)row)[x] = (r << 11) | (g << 5) | b;
            }
        }
    }
}

struct track {
    const char *title;
    const char *artist;
    const char *path;
    int duration_sec;
};

static struct track tracks[5] = {
    {"Astronauts (Piano)", "siinamota", "/usr/share/music/astronauts_piano.mp3", 188},
    {"Astronauts (Original)", "siinamota", "/usr/share/music/astronauts_orig.mp3", 210},
    {"Synthwave Chiptune", "Alpine Synth", NULL, 120},
    {"Cyberpunk Arp 48kHz", "S5L8740 DSP", NULL, 95},
    {"Relaxing Lullaby", "CS42L81 DAC", NULL, 150}
};

static volatile bool playing = true;
static volatile int cur_track = 0;
static volatile int cur_sec = 0;
static volatile int volume_pct = 82;
static volatile int track_change_seq = 0;
static volatile int vu_levels[12];

static void set_hw_volume(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    volume_pct = pct;
    int mfd = open("/dev/mixer", O_RDWR);
    if (mfd >= 0) {
        int v = (pct << 8) | pct;
        ioctl(mfd, SOUND_MIXER_WRITE_VOLUME, &v);
        ioctl(mfd, SOUND_MIXER_WRITE_PCM, &v);
        close(mfd);
    }
}

static void *audio_thread_fn(void *arg) {
    (void)arg;
    mp3dec_t mp3d;
    mp3dec_init(&mp3d);
    int16_t pcm_buf[MINIMP3_MAX_SAMPLES_PER_FRAME];
    int16_t synth_buf[2048];
    int dsp_fd = -1;
    int cur_rate = 48000;

    set_hw_volume(volume_pct);

    while (1) {
        if (!playing) {
            for (int i = 0; i < 12; i++) vu_levels[i] = 2;
            usleep(40000);
            continue;
        }

        if (dsp_fd < 0) {
            dsp_fd = open("/dev/dsp", O_WRONLY);
            if (dsp_fd >= 0) {
                int fmt = AFMT_S16_LE;
                int ch = 2;
                int rate = 48000;
                ioctl(dsp_fd, SNDCTL_DSP_SETFMT, &fmt);
                ioctl(dsp_fd, SNDCTL_DSP_CHANNELS, &ch);
                ioctl(dsp_fd, SNDCTL_DSP_SPEED, &rate);
                cur_rate = rate;
            } else {
                usleep(100000);
                continue;
            }
        }

        int my_seq = track_change_seq;
        int t = cur_track;
        const char *path = tracks[t].path;

        if (path) {
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                struct stat st;
                fstat(fd, &st);
                size_t sz = st.st_size;
                uint8_t *data = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
                if (data != MAP_FAILED) {
                    size_t off = 0;
                    uint64_t samples_played = 0;
                    mp3dec_init(&mp3d);
                    while (off < sz && track_change_seq == my_seq) {
                        if (!playing) {
                            usleep(30000);
                            continue;
                        }
                        mp3dec_frame_info_t info;
                        int samples = mp3dec_decode_frame(&mp3d, data + off, sz - off, pcm_buf, &info);
                        if (info.frame_bytes == 0) break;
                        off += info.frame_bytes;
                        if (samples > 0) {
                            if (info.channels == 1) {
                                for (int i = samples - 1; i >= 0; i--) {
                                    pcm_buf[i * 2] = pcm_buf[i];
                                    pcm_buf[i * 2 + 1] = pcm_buf[i];
                                }
                            }
                            for (int b = 0; b < 12; b++) {
                                int idx = (b * samples * 2) / 12;
                                int v = abs(pcm_buf[idx]) >> 9;
                                if (v > 28) v = 28;
                                vu_levels[b] = v + 2;
                            }
                            write(dsp_fd, pcm_buf, samples * 2 * sizeof(int16_t));
                            samples_played += samples;
                            cur_sec = (int)(samples_played / (cur_rate ? cur_rate : 48000));
                        }
                    }
                    munmap(data, sz);
                }
                close(fd);
                if (track_change_seq == my_seq && playing) {
                    cur_track = (cur_track + 1) % 5;
                    cur_sec = 0;
                    track_change_seq++;
                }
                continue;
            }
        }

        /* Fallback synth melody */
        static const float notes[8] = {261.63f, 329.63f, 392.00f, 523.25f, 440.00f, 349.23f, 392.00f, 293.66f};
        uint64_t total_frames = 0;
        float phase = 0.0f;
        while (track_change_seq == my_seq) {
            if (!playing) {
                usleep(30000);
                continue;
            }
            int note_idx = (total_frames / 12000) % 8;
            float freq = notes[note_idx] * (1.0f + 0.15f * (t - 2));
            float step = 2.0f * 3.14159265f * freq / 48000.0f;
            for (int i = 0; i < 1024; i++) {
                int16_t s = (int16_t)(sinf(phase) * 6000.0f);
                phase += step;
                if (phase > 6.2831853f) phase -= 6.2831853f;
                synth_buf[i * 2] = s;
                synth_buf[i * 2 + 1] = s;
            }
            for (int b = 0; b < 12; b++)
                vu_levels[b] = 6 + ((note_idx * 5 + b * 3) % 20);
            write(dsp_fd, synth_buf, 1024 * 2 * sizeof(int16_t));
            total_frames += 1024;
            cur_sec = (int)(total_frames / 48000);
            if (cur_sec >= tracks[t].duration_sec) {
                cur_track = (cur_track + 1) % 5;
                cur_sec = 0;
                track_change_seq++;
                break;
            }
        }
    }
    return NULL;
}

/* UI State */
static int ui_mode = 0; /* 0 = Rhythmbox, 1 = Keyboard/Terminal, 2 = Fastfetch/System */
static int rb_focus = 1; /* 0..4 = transport, 5..9 = tracks, 10..12 = bottom tabs */
static int kb_focus = 0; /* 0..39 = keys, 40=SPACE, 41=BACK, 42=CLEAR, 43=RUN, 44=SALIR */
static char cmd_buf[64] = "uname -a";
static char term_out[8][42] = {
    "Alpine Linux v3.21.8 (armhf)",
    "Kernel: Linux 6.14.0-s5l87xx",
    "Audio: CS42L81 + S5L8740 I2S OK",
    "Touch: Apple Grape SPI2 @ 12MHz",
    "Escribe un comando y pulsa RUN",
    "", "", ""
};

/* Touch tracking */
static int last_touch_x = -1;
static int last_touch_y = -1;
static int touch_ripple_frames = 0;
static int cur_abs_x = 120;
static int cur_abs_y = 216;
static bool touch_dev_found = false;
static char grape_status_line[40] = "Grape: probing...";

static void update_grape_status(void) {
    /* Check if Apple Grape Touchscreen is in /proc/bus/input/devices */
    FILE *fp = fopen("/proc/bus/input/devices", "r");
    if (fp) {
        char line[128];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "Grape") || strstr(line, "Touch")) {
                touch_dev_found = true;
                break;
            }
        }
        fclose(fp);
    }
    const char *paths[] = {
        "/sys/bus/spi/devices/spi1.0/state",
        "/sys/bus/spi/devices/spi0.0/state",
        "/sys/devices/platform/soc/3d200000.spi/spi_master/spi1/spi1.0/state",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        FILE *f = fopen(paths[i], "r");
        if (f) {
            char buf[128] = {0};
            fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            int fw_up = 0, cal_up = 0, exec_s = 0, run_r = 0;
            char *p;
            if ((p = strstr(buf, "fw_uploaded="))) fw_up = atoi(p + 12);
            if ((p = strstr(buf, "cal_uploaded="))) cal_up = atoi(p + 13);
            if ((p = strstr(buf, "exec_sent="))) exec_s = atoi(p + 10);
            if ((p = strstr(buf, "runtime_ready="))) run_r = atoi(p + 14);
            snprintf(grape_status_line, sizeof(grape_status_line),
                     "Grape fw=%d cal=%d ex=%d rdy=%d", fw_up, cal_up, exec_s, run_r);
            return;
        }
    }
    snprintf(grape_status_line, sizeof(grape_status_line),
             touch_dev_found ? "Touch: Apple Grape OK" : "Touch: evdev active");
}

static void run_command(const char *cmd) {
    for (int i = 0; i < 8; i++) term_out[i][0] = '\0';
    snprintf(term_out[0], 42, "$ %s", cmd);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        snprintf(term_out[1], 42, "Error ejecutando comando");
        return;
    }
    int row = 1;
    char buf[128];
    while (row < 8 && fgets(buf, sizeof(buf), fp)) {
        size_t l = strlen(buf);
        while (l > 0 && (buf[l-1] == '\n' || buf[l-1] == '\r')) buf[--l] = '\0';
        snprintf(term_out[row++], 42, "%s", buf);
    }
    pclose(fp);
}

static const char kb_rows[4][10] = {
    {'1','2','3','4','5','6','7','8','9','0'},
    {'q','w','e','r','t','y','u','i','o','p'},
    {'a','s','d','f','g','h','j','k','l','-'},
    {'z','x','c','v','b','n','m','.','/',' '}
};

static void trigger_rb_action(int focus) {
    if (focus == 0) {
        cur_track = (cur_track + 4) % 5;
        cur_sec = 0;
        playing = true;
        track_change_seq++;
    } else if (focus == 1) {
        playing = !playing;
    } else if (focus == 2) {
        cur_track = (cur_track + 1) % 5;
        cur_sec = 0;
        playing = true;
        track_change_seq++;
    } else if (focus == 3) {
        set_hw_volume(volume_pct - 8);
    } else if (focus == 4) {
        set_hw_volume(volume_pct + 8);
    } else if (focus >= 5 && focus <= 9) {
        cur_track = focus - 5;
        cur_sec = 0;
        playing = true;
        track_change_seq++;
    } else if (focus == 10) {
        ui_mode = 1;
    } else if (focus == 11) {
        update_grape_status();
        ui_mode = 2;
    } else if (focus == 12) {
        sync();
        system("poweroff -f &");
    }
}

static void trigger_kb_action(int focus) {
    size_t len = strlen(cmd_buf);
    if (focus >= 0 && focus < 40) {
        char ch = kb_rows[focus / 10][focus % 10];
        if (len + 1 < sizeof(cmd_buf)) {
            cmd_buf[len] = ch;
            cmd_buf[len + 1] = '\0';
        }
    } else if (focus == 40) {
        if (len + 1 < sizeof(cmd_buf)) {
            cmd_buf[len] = ' ';
            cmd_buf[len + 1] = '\0';
        }
    } else if (focus == 41) {
        if (len > 0) cmd_buf[len - 1] = '\0';
    } else if (focus == 42) {
        cmd_buf[0] = '\0';
    } else if (focus == 43) {
        run_command(cmd_buf);
    } else if (focus == 44) {
        ui_mode = 0;
    }
}

static void handle_touch_tap(int x, int y) {
    if (x < 0) x = 0;
    if (x >= W) x = W - 1;
    if (y < 0) y = 0;
    if (y >= H) y = H - 1;
    last_touch_x = x;
    last_touch_y = y;
    touch_ripple_frames = 10;

    if (ui_mode == 0) {
        /* Transport buttons: y in 130..162 */
        if (y >= 130 && y <= 162) {
            int idx = (x - 6) / 46;
            if (idx < 0) idx = 0;
            if (idx > 4) idx = 4;
            rb_focus = idx;
            trigger_rb_action(idx);
            return;
        }
        /* Now playing card tap toggles play/pause */
        if (y >= 28 && y < 130) {
            rb_focus = 1;
            trigger_rb_action(1);
            return;
        }
        /* Playlist rows: y in 188..342 (5 rows of 30px) */
        if (y >= 188 && y < 342) {
            int row = (y - 188) / 30;
            if (row >= 0 && row < 5) {
                rb_focus = 5 + row;
                trigger_rb_action(rb_focus);
                return;
            }
        }
        /* Bottom tabs: y in 350..420 (3 rows of 22px) */
        if (y >= 350 && y <= 425) {
            int tab = (y - 350) / 22;
            if (tab < 0) tab = 0;
            if (tab > 2) tab = 2;
            rb_focus = 10 + tab;
            trigger_rb_action(rb_focus);
            return;
        }
    } else if (ui_mode == 1) {
        /* Top bar tap exits keyboard */
        if (y < 26) {
            ui_mode = 0;
            return;
        }
        /* 10x4 QWERTY grid: y in 196..308, x in 4..234 */
        if (y >= 196 && y < 308) {
            int row = (y - 196) / 28;
            int col = (x - 4) / 23;
            if (row >= 0 && row < 4 && col >= 0 && col < 10) {
                kb_focus = row * 10 + col;
                trigger_kb_action(kb_focus);
                return;
            }
        }
        /* Action row: y in 310..346 */
        if (y >= 310 && y <= 348) {
            int act = (x - 4) / 46;
            if (act < 0) act = 0;
            if (act > 4) act = 4;
            kb_focus = 40 + act;
            trigger_kb_action(kb_focus);
            return;
        }
        /* Bottom bar tap returns to Rhythmbox */
        if (y >= 395) {
            ui_mode = 0;
            return;
        }
    } else if (ui_mode == 2) {
        ui_mode = 0;
    }
}

static void render_ui(void) {
    fill_rect(0, 0, W, H, 0x0f141d);

    /* Top Header Bar */
    fill_rect(0, 0, W, 22, 0x1b2434);
    draw_str(6, 7, "RHYTHMBOX", 0x38bdf8, 1);
    char hdr_r[24];
    if (last_touch_x >= 0)
        snprintf(hdr_r, sizeof(hdr_r), "T:%d,%d V:%d%%", last_touch_x, last_touch_y, volume_pct);
    else
        snprintf(hdr_r, sizeof(hdr_r), "Touch+Btn V:%d%%", volume_pct);
    draw_str(W - (int)strlen(hdr_r) * 6 - 5, 7, hdr_r, 0xa7f3d0, 1);

    if (ui_mode == 0) {
        /* Now Playing Card */
        fill_rect(6, 26, W - 12, 100, 0x172030);
        draw_rect(6, 26, W - 12, 100, 0x2d3d56);

        /* Album Art & VU Visualizer */
        fill_rect(12, 32, 54, 54, 0x2563eb);
        draw_rect(12, 32, 54, 54, 0x60a5fa);
        for (int b = 0; b < 8; b++) {
            int h = vu_levels[b] * 2;
            if (h > 46) h = 46;
            fill_rect(16 + b * 6, 82 - h, 4, h, 0x38bdf8);
        }

        draw_str(72, 34, playing ? "REPRODUCIENDO" : "EN PAUSA", playing ? 0x34d399 : 0xfbbf24, 1);
        draw_str(72, 48, tracks[cur_track].title, 0xffffff, 1);
        draw_str(72, 62, tracks[cur_track].artist, 0x94a3b8, 1);

        int dur = tracks[cur_track].duration_sec;
        int sec = cur_sec;
        if (sec > dur) sec = dur;
        char tbuf[32];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d / %02d:%02d", sec / 60, sec % 60, dur / 60, dur % 60);
        draw_str(72, 76, tbuf, 0x38bdf8, 1);

        /* Progress Bar */
        fill_rect(12, 96, W - 24, 8, 0x0b0f17);
        int pw = dur > 0 ? ((W - 24) * sec) / dur : 0;
        fill_rect(12, 96, pw, 8, 0x38bdf8);
        draw_str(12, 110, grape_status_line, 0x94a3b8, 1);

        /* Transport Row (0..4) */
        const char *tlabels[5] = {"|<<", playing ? "||" : ">", ">>|", "V-", "V+"};
        for (int i = 0; i < 5; i++) {
            int bx = 6 + i * 46;
            bool sel = (rb_focus == i);
            fill_rect(bx, 132, 42, 26, sel ? 0x2563eb : 0x1e293b);
            draw_rect(bx, 132, 42, 26, sel ? 0x93c5fd : 0x334155);
            int tw = (int)strlen(tlabels[i]) * 6;
            draw_str(bx + (42 - tw) / 2, 141, tlabels[i], 0xffffff, 1);
        }

        /* Playlist Header */
        draw_str(8, 168, "BIBLIOTECA MUSICAL (TOCA O USA VOL+-)", 0x60a5fa, 1);
        fill_rect(6, 180, W - 12, 1, 0x334155);

        for (int i = 0; i < 5; i++) {
            int ry = 186 + i * 30;
            bool sel = (rb_focus == 5 + i);
            bool active = (cur_track == i);
            fill_rect(6, ry, W - 12, 27, sel ? 0x1d4ed8 : (active ? 0x172554 : 0x151d2a));
            draw_rect(6, ry, W - 12, 27, sel ? 0x93c5fd : 0x263246);
            draw_str(12, ry + 5, active ? (playing ? ">" : "=") : "-", active ? 0x34d399 : 0x64748b, 1);
            draw_str(24, ry + 5, tracks[i].title, 0xffffff, 1);
            draw_str(24, ry + 16, tracks[i].artist, 0x94a3b8, 1);
        }

        /* Bottom Action Buttons (10..12) */
        const char *blabels[3] = {
            "[ TECLADO / TERMINAL LINUX ]",
            "[ SISTEMA / TOUCH DIAG ]",
            "[ APAGAR IPOD NANO 7G ]"
        };
        for (int i = 0; i < 3; i++) {
            int by = 344 + i * 22;
            bool sel = (rb_focus == 10 + i);
            fill_rect(6, by, W - 12, 19, sel ? 0x0284c7 : 0x1e293b);
            draw_rect(6, by, W - 12, 19, sel ? 0x7dd3fc : 0x334155);
            int tw = (int)strlen(blabels[i]) * 6;
            draw_str((W - tw) / 2, by + 6, blabels[i], 0xffffff, 1);
        }
        draw_str(8, 416, "TACTIL ACTIVO | VOL+-:Mover HOME:OK", 0x64748b, 1);

    } else if (ui_mode == 1) {
        /* Terminal Output Box */
        fill_rect(6, 26, W - 12, 128, 0x090d14);
        draw_rect(6, 26, W - 12, 128, 0x334155);
        for (int i = 0; i < 8; i++) {
            draw_str(10, 32 + i * 14, term_out[i], i == 0 ? 0x38bdf8 : 0xe2e8f0, 1);
        }

        /* Command Input Box */
        fill_rect(6, 160, W - 12, 28, 0x172030);
        draw_rect(6, 160, W - 12, 28, 0x38bdf8);
        char ibuf[72];
        snprintf(ibuf, sizeof(ibuf), "> %s_", cmd_buf);
        draw_str(10, 170, ibuf, 0x34d399, 1);

        /* 10x4 Keyboard Grid */
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 10; c++) {
                int idx = r * 10 + c;
                int kx = 5 + c * 23;
                int ky = 196 + r * 28;
                bool sel = (kb_focus == idx);
                fill_rect(kx, ky, 21, 25, sel ? 0x2563eb : 0x1e293b);
                draw_rect(kx, ky, 21, 25, sel ? 0x93c5fd : 0x334155);
                draw_char(kx + 8, ky + 9, kb_rows[r][c], 0xffffff, 1);
            }
        }

        /* Action Row (40..44) */
        const char *alabels[5] = {"SPC", "DEL", "CLR", "RUN", "BACK"};
        for (int i = 0; i < 5; i++) {
            int ax = 5 + i * 46;
            bool sel = (kb_focus == 40 + i);
            fill_rect(ax, 312, 44, 28, sel ? 0x059669 : 0x1e293b);
            draw_rect(ax, 312, 44, 28, sel ? 0x6ee7b7 : 0x334155);
            int tw = (int)strlen(alabels[i]) * 6;
            draw_str(ax + (44 - tw) / 2, 322, alabels[i], 0xffffff, 1);
        }

        draw_str(8, 352, "Toca las teclas o usa VOL+- y HOME", 0x94a3b8, 1);
        draw_str(8, 368, grape_status_line, 0x38bdf8, 1);
        fill_rect(6, 392, W - 12, 28, 0x1e293b);
        draw_rect(6, 392, W - 12, 28, 0x38bdf8);
        draw_str(24, 402, "TOCA AQUI O PULSA POWER: VOLVER", 0xffffff, 1);

    } else if (ui_mode == 2) {
        fill_rect(6, 28, W - 12, H - 36, 0x131c2b);
        draw_rect(6, 28, W - 12, H - 36, 0x38bdf8);
        draw_str(14, 40, "IPOD NANO 7G - ALPINE LINUX", 0x38bdf8, 1);
        draw_str(14, 60, "OS: Alpine Linux v3.21.8 armhf", 0xffffff, 1);
        draw_str(14, 78, "Kernel: Linux 6.14.0-s5l87xx", 0xffffff, 1);
        draw_str(14, 96, "SoC: Apple S5L8740 (Cortex-A5)", 0xffffff, 1);
        draw_str(14, 114, "Display: 240x432 32bpp LCDIF", 0x34d399, 1);
        draw_str(14, 132, "Audio: Cirrus Logic CS42L81 DAC", 0x34d399, 1);
        draw_str(14, 150, "Touch: TI 343S0538 Apple Grape", 0x34d399, 1);
        draw_str(14, 168, grape_status_line, 0xfbbf24, 1);
        char tpos[40];
        snprintf(tpos, sizeof(tpos), "Ultimo toque: X=%d Y=%d", last_touch_x, last_touch_y);
        draw_str(14, 190, tpos, 0x38bdf8, 1);
        draw_str(14, 380, "Toca la pantalla o pulsa HOME", 0x94a3b8, 1);
        draw_str(14, 396, "para volver a Rhythmbox", 0x94a3b8, 1);
    }

    /* Draw touch indicator ripple if recent tap */
    if (touch_ripple_frames > 0 && last_touch_x >= 0 && last_touch_y >= 0) {
        int r = (11 - touch_ripple_frames) * 2;
        draw_rect(last_touch_x - r, last_touch_y - r, r * 2 + 1, r * 2 + 1, 0x38bdf8);
        touch_ripple_frames--;
    }

    flush_fb();
}

int main(void) {
    /* Mount basic pseudo-filesystems if running as /init or early */
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);

    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd >= 0) {
        ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
        ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
        fb_size = finfo.line_length * vinfo.yres;
        fb_mem = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    }

    update_grape_status();

    pthread_t aud_th;
    pthread_create(&aud_th, NULL, audio_thread_fn, NULL);
    pthread_detach(aud_th);

    int ev_fds[8];
    for (int i = 0; i < 8; i++) ev_fds[i] = -1;

    bool touch_down = false;
    bool abs_updated = false;
    int tick = 0;

    while (1) {
        /* Re-scan input devices every ~1 second so late-probed touch is picked up */
        if ((tick % 25) == 0) {
            for (int i = 0; i < 8; i++) {
                if (ev_fds[i] < 0) {
                    char p[32];
                    snprintf(p, sizeof(p), "/dev/input/event%d", i);
                    ev_fds[i] = open(p, O_RDONLY | O_NONBLOCK);
                }
            }
            if ((tick % 50) == 0)
                update_grape_status();
        }
        tick++;

        for (int i = 0; i < 8; i++) {
            if (ev_fds[i] < 0) continue;
            struct input_event ev;
            while (read(ev_fds[i], &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_ABS) {
                    if (ev.code == ABS_MT_POSITION_X || ev.code == ABS_X) {
                        cur_abs_x = ev.value;
                        abs_updated = true;
                    } else if (ev.code == ABS_MT_POSITION_Y || ev.code == ABS_Y) {
                        cur_abs_y = ev.value;
                        abs_updated = true;
                    } else if (ev.code == ABS_MT_TRACKING_ID) {
                        if (ev.value >= 0) {
                            touch_down = true;
                        } else if (touch_down) {
                            touch_down = false;
                        }
                    }
                } else if (ev.type == EV_KEY) {
                    if (ev.code == BTN_TOUCH) {
                        if (ev.value == 1) {
                            touch_down = true;
                            handle_touch_tap(cur_abs_x, cur_abs_y);
                            abs_updated = false;
                        } else if (ev.value == 0) {
                            touch_down = false;
                        }
                    } else if (ev.value == 1) {
                        /* Physical button press */
                        if (ev.code == KEY_VOLUMEUP) {
                            if (ui_mode == 0) rb_focus = (rb_focus + 12) % 13;
                            else if (ui_mode == 1) kb_focus = (kb_focus + 44) % 45;
                            else ui_mode = 0;
                        } else if (ev.code == KEY_VOLUMEDOWN) {
                            if (ui_mode == 0) rb_focus = (rb_focus + 1) % 13;
                            else if (ui_mode == 1) kb_focus = (kb_focus + 1) % 45;
                            else ui_mode = 0;
                        } else if (ev.code == KEY_HOMEPAGE || ev.code == KEY_PLAYPAUSE || ev.code == KEY_ENTER) {
                            if (ui_mode == 0) trigger_rb_action(rb_focus);
                            else if (ui_mode == 1) trigger_kb_action(kb_focus);
                            else ui_mode = 0;
                        } else if (ev.code == KEY_POWER) {
                            if (ui_mode != 0) ui_mode = 0;
                            else playing = !playing;
                        }
                    }
                } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                    if (touch_down && abs_updated) {
                        handle_touch_tap(cur_abs_x, cur_abs_y);
                        abs_updated = false;
                    }
                }
            }
        }

        render_ui();
        usleep(40000);
    }
    return 0;
}
