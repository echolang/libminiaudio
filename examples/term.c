#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

static int g_saved;
static int g_has_keyup;

static void restore_atexit(void);

#if defined(_WIN32)

static HANDLE g_in;
static DWORD g_orig;
static uint8_t g_down[256];

int32_t eco_term_raw(void)
{
    DWORD mode;

    g_in = GetStdHandle(STD_INPUT_HANDLE);
    if (g_in == INVALID_HANDLE_VALUE) {
        return -1;
    }

    if (!GetConsoleMode(g_in, &g_orig)) {
        return -1;
    }

    mode = g_orig;
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    if (!SetConsoleMode(g_in, mode)) {
        return -1;
    }

    memset(g_down, 0, sizeof(g_down));
    g_has_keyup = 1;
    g_saved = 1;
    atexit(restore_atexit);
    return 0;
}

void eco_term_restore(void)
{
    if (!g_saved) {
        return;
    }

    SetConsoleMode(g_in, g_orig);
    g_saved = 0;
}

int32_t eco_term_poll(int32_t *key, int32_t *kind)
{
    INPUT_RECORD rec;
    DWORD n;
    KEY_EVENT_RECORD ke;
    unsigned char ch;

    if (key == NULL || kind == NULL) {
        return 0;
    }

    for (;;) {
        if (!PeekConsoleInput(g_in, &rec, 1, &n) || n == 0) {
            return 0;
        }

        if (!ReadConsoleInput(g_in, &rec, 1, &n) || n == 0) {
            return 0;
        }

        if (rec.EventType != KEY_EVENT) {
            continue;
        }

        ke = rec.Event.KeyEvent;
        ch = (unsigned char)ke.uChar.AsciiChar;
        if (ch == 0) {
            continue;
        }

        *key = (int32_t)ch;
        if (ke.bKeyDown) {
            if (g_down[ch]) {
                *kind = 2;
            } else {
                g_down[ch] = 1;
                *kind = 1;
            }
        } else {
            g_down[ch] = 0;
            *kind = 3;
        }

        return 1;
    }
}

#else

static struct termios g_orig;
static unsigned char g_buf[64];
static int g_len;

static void keyboard_enable(void)
{
    const char seq[] = "\x1b[>11u";

    write(STDOUT_FILENO, seq, sizeof(seq) - 1);
}

static void keyboard_disable(void)
{
    const char seq[] = "\x1b[<u";

    write(STDOUT_FILENO, seq, sizeof(seq) - 1);
}

static int parse_csi_u(const unsigned char *s, int n, int32_t *key, int32_t *kind)
{
    int i;
    int32_t code;
    int32_t event;

    i = 0;
    code = 0;
    event = 1;

    while (i < n && s[i] >= '0' && s[i] <= '9') {
        code = code * 10 + (s[i] - '0');
        i++;
    }

    while (i < n && s[i] == ':') {
        i++;
        while (i < n && s[i] >= '0' && s[i] <= '9') {
            i++;
        }
    }

    if (i < n && s[i] == ';') {
        i++;
        while (i < n && s[i] >= '0' && s[i] <= '9') {
            i++;
        }

        if (i < n && s[i] == ':') {
            i++;
            event = 0;
            while (i < n && s[i] >= '0' && s[i] <= '9') {
                event = event * 10 + (s[i] - '0');
                i++;
            }

            if (event == 0) {
                event = 1;
            }
        }
    }

    if (code <= 0) {
        return -1;
    }

    *key = code;
    *kind = event;
    g_has_keyup = 1;
    return 0;
}

static int consume_event(int32_t *key, int32_t *kind)
{
    int i;
    int final;

    if (g_len <= 0) {
        return 0;
    }

    if (g_buf[0] != 0x1b) {
        *key = (int32_t)g_buf[0];
        *kind = 1;
        memmove(g_buf, g_buf + 1, (size_t)(g_len - 1));
        g_len--;
        return 1;
    }

    if (g_len == 1) {
        return 0;
    }

    if (g_buf[1] != '[') {
        memmove(g_buf, g_buf + 1, (size_t)(g_len - 1));
        g_len--;
        return 0;
    }

    final = -1;
    for (i = 2; i < g_len; i++) {
        if (g_buf[i] >= 0x40 && g_buf[i] <= 0x7e) {
            final = i;
            break;
        }
    }

    if (final < 0) {
        if (g_len == (int)sizeof(g_buf)) {
            g_len = 0;
        }
        return 0;
    }

    if (g_buf[final] == 'u') {
        if (parse_csi_u(g_buf + 2, final - 2, key, kind) == 0) {
            memmove(g_buf, g_buf + final + 1, (size_t)(g_len - final - 1));
            g_len -= final + 1;
            return 1;
        }
    }

    memmove(g_buf, g_buf + final + 1, (size_t)(g_len - final - 1));
    g_len -= final + 1;
    return 0;
}

int32_t eco_term_raw(void)
{
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &g_orig) != 0) {
        return -1;
    }

    raw = g_orig;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        return -1;
    }

    g_len = 0;
    g_has_keyup = 0;
    g_saved = 1;
    keyboard_enable();
    atexit(restore_atexit);
    return 0;
}

void eco_term_restore(void)
{
    if (!g_saved) {
        return;
    }

    keyboard_disable();
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig);
    g_saved = 0;
}

int32_t eco_term_poll(int32_t *key, int32_t *kind)
{
    unsigned char byte;
    ssize_t n;
    int got;

    if (key == NULL || kind == NULL) {
        return 0;
    }

    got = consume_event(key, kind);
    if (got) {
        return 1;
    }

    for (;;) {
        n = read(STDIN_FILENO, &byte, 1);
        if (n != 1) {
            break;
        }

        if (g_len < (int)sizeof(g_buf)) {
            g_buf[g_len] = byte;
            g_len++;
        } else {
            g_len = 0;
            g_buf[0] = byte;
            g_len = 1;
        }

        got = consume_event(key, kind);
        if (got) {
            return 1;
        }
    }

    return 0;
}

#endif

int32_t eco_term_has_keyup(void)
{
    return g_has_keyup;
}

static void restore_atexit(void)
{
    eco_term_restore();
}
