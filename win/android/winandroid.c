/* NetHack 5.0  winandroid.c */
/* Android window port - JNI bridge to ForkFront Java UI */

#include <string.h>
#include <errno.h>
#include <jni.h>
#include <ctype.h>
#include <android/log.h>

#include "hack.h"
#include "func_tab.h"   /* for extended commands */
#include "dlb.h"

/*
 * Input buffer: Java pushes characters here; nhgetch() pops them.
 * Used for macro/script replay and for feeding strings to getlin().
 */
#define INPUT_BUFFER_SIZE 256
static char and_input_buffer[INPUT_BUFFER_SIZE];
static int  and_input_buffer_pos = 0;

static int
and_input_buffer_is_empty(void)
{
    return and_input_buffer_pos <= 0;
}

static void
and_input_buffer_push(char c)
{
    if (and_input_buffer_pos < INPUT_BUFFER_SIZE - 1)
        and_input_buffer[and_input_buffer_pos++] = c;
}

static char
and_input_buffer_pop(void)
{
    char c = '\0';
    if (and_input_buffer_pos > 0) {
        c = and_input_buffer[0];
        memmove(and_input_buffer, and_input_buffer + 1,
                (size_t) --and_input_buffer_pos);
    }
    return c;
}

static char *
and_input_buffer_pop_all(void)
{
    static char buf[INPUT_BUFFER_SIZE];
    memcpy(buf, and_input_buffer, (size_t) and_input_buffer_pos);
    buf[and_input_buffer_pos] = '\0';
    and_input_buffer_pos = 0;
    return buf;
}

/* Called from Java to push a UTF-8 string into the input buffer */
JNIEXPORT void JNICALL
Java_com_tbd_forkfront_engine_NetHackIO_pushInput(JNIEnv *env, jobject thiz,
                                                  jstring str)
{
    const char *pChars = (*env)->GetStringUTFChars(env, str, 0);
    if (pChars) {
        int i;
        for (i = 0; pChars[i]; i++)
            and_input_buffer_push(pChars[i]);
        (*env)->ReleaseStringUTFChars(env, str, pChars);
    }
}

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

static void and_init_nhwindows(int *, char **);
static void and_player_selection(void);
static void and_askname(void);
static void and_get_nh_event(void);
static void and_exit_nhwindows(const char *);
static void and_suspend_nhwindows(const char *);
static void and_resume_nhwindows(void);
static winid and_create_nhwindow(int);
static void and_clear_nhwindow(winid);
static void and_display_nhwindow(winid, boolean);
static void and_destroy_nhwindow(winid);
static void and_curs(winid, int, int);
static void and_putstr(winid, int, const char *);
static void and_putmixed(winid, int, const char *);
static void and_display_file(const char *, boolean);
static void and_start_menu(winid, unsigned long);
static void and_add_menu(winid, const glyph_info *, const ANY_P *,
                         char, char, int, int, const char *, unsigned int);
static void and_end_menu(winid, const char *);
static int  and_select_menu(winid, int, MENU_ITEM_P **);
static char and_message_menu(char, int, const char *);
static void and_mark_synch(void);
static void and_wait_synch(void);
#ifdef CLIPPING
static void and_cliparound(int, int);
#endif
#ifdef POSITIONBAR
static void and_update_positionbar(char *);
#endif
static void and_print_glyph(winid, coordxy, coordxy,
                            const glyph_info *, const glyph_info *);
static void and_raw_print(const char *);
static void and_raw_print_bold(const char *);
static int  and_nhgetch(void);
static int  and_nh_poskey(coordxy *, coordxy *, int *);
static void and_nhbell(void);
static int  and_doprev_message(void);
static char and_yn_function(const char *, const char *, char);
static void and_getlin(const char *, char *);
static int  and_get_ext_cmd(void);
static void and_number_pad(int);
static void and_delay_output(void);
#ifdef CHANGE_COLOR
static void  and_change_color(int, long, int);
static char *and_get_color_string(void);
#endif
static void  and_outrip(winid, int, time_t);
static char *and_getmsghistory(boolean);
static void  and_putmsghistory(const char *, boolean);
static void  and_status_init(void);
static void  and_status_finish(void);
static void  and_status_enablefield(int, const char *, const char *, boolean);
static void and_status_update(int, genericptr_t, int, int, int,
                               unsigned long *);
static void  and_update_inventory(int);
static win_request_info *and_ctrl_nhwindow(winid, int, win_request_info *);

/* Forward declarations for functions defined later or elsewhere */
void save_msg(const char *);
int NetHackMain(int, char **);
void and_putstr_ex(winid, int, const char *, int, int);

/* -------------------------------------------------------------------------
 * window_procs registration
 * ---------------------------------------------------------------------- */

struct window_procs android_procs = {
    WPID(android),
    /* wincap */
    WC_COLOR | WC_HILITE_PET | WC_INVERSE | WC_TILED_MAP
        | WC_ASCII_MAP | WC_EIGHT_BIT_IN | WC_MOUSE_SUPPORT,
    /* wincap2 */
    WC2_HILITE_STATUS | WC2_FLUSH_STATUS | WC2_SELECTSAVED
        | WC2_SUPPRESS_HIST | WC2_EXTRASTATUS | WC2_SOFTKEYBOARD
        | WC2_FULLSCREEN,
    /* has_color */
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    and_init_nhwindows,
    and_player_selection,
    and_askname,
    and_get_nh_event,
    and_exit_nhwindows,
    and_suspend_nhwindows,
    and_resume_nhwindows,
    and_create_nhwindow,
    and_clear_nhwindow,
    and_display_nhwindow,
    and_destroy_nhwindow,
    and_curs,
    and_putstr,
    and_putmixed,
    and_display_file,
    and_start_menu,
    and_add_menu,
    and_end_menu,
    and_select_menu,
    and_message_menu,
    and_mark_synch,
    and_wait_synch,
#ifdef CLIPPING
    and_cliparound,
#endif
#ifdef POSITIONBAR
    and_update_positionbar,
#endif
    and_print_glyph,
    and_raw_print,
    and_raw_print_bold,
    and_nhgetch,
    and_nh_poskey,
    and_nhbell,
    and_doprev_message,
    and_yn_function,
    and_getlin,
    and_get_ext_cmd,
    and_number_pad,
    and_delay_output,
#ifdef CHANGE_COLOR
    and_change_color,
    and_get_color_string,
#endif
    and_outrip,
    genl_preference_update,
    and_getmsghistory,
    and_putmsghistory,
    and_status_init,
    and_status_finish,
    and_status_enablefield,
    and_status_update,
    genl_can_suspend_no,
    and_update_inventory,
    and_ctrl_nhwindow,
};

/* -------------------------------------------------------------------------
 * JNI state — cached environment and method IDs
 * ---------------------------------------------------------------------- */

static JNIEnv *jEnv;
static jclass  jApp;
static jobject jAppInstance;

static jmethodID jDebugLog;
static jmethodID jReceiveKey;
static jmethodID jReceivePosKey;
static jmethodID jCreateWindow;
static jmethodID jDisplayWindow;
static jmethodID jClearWindow;
static jmethodID jDestroyWindow;
static jmethodID jPutString;
static jmethodID jSetHealthColor;
static jmethodID jRedrawStatus;
static jmethodID jRawPrint;
static jmethodID jSetCursorPos;
static jmethodID jPrintTile;
static jmethodID jYNFunction;
static jmethodID jGetLine;
static jmethodID jStartMenu;
static jmethodID jAddMenu;
static jmethodID jEndMenu;
static jmethodID jSelectMenu;
static jmethodID jCliparound;
static jmethodID jDelayOutput;
static jmethodID jHighlightDPad;
static jmethodID jShowLog;
static jmethodID jSetUsername;
static jmethodID jSetNumPadOption;
static jmethodID jAskName;
static jmethodID jLoadSound;
static jmethodID jPlaySound;
static jmethodID jGetDumplogDir;
static jmethodID jStatusInit;
static jmethodID jStatusEnableField;
static jmethodID jStatusUpdate;
static jmethodID jStatusFinish;

static boolean quit_if_possible;
static boolean restoring_msghistory;

/* -------------------------------------------------------------------------
 * Message history ring buffer
 * ---------------------------------------------------------------------- */

#define MSGHISTORY_SIZE 32
static char *msghistory[MSGHISTORY_SIZE];
static int   msghistory_idx;   /* next write position */
static int   msghistory_idx0;  /* current read position */

static int
add_msghistory_idx(int idx)
{
    return (idx + 1) % MSGHISTORY_SIZE;
}

/* -------------------------------------------------------------------------
 * Condition tracking (for coloured status display)
 * ---------------------------------------------------------------------- */

extern const char *status_fieldfmt[MAXBLSTATS];
extern char       *status_vals[MAXBLSTATS];
extern boolean     status_activefields[MAXBLSTATS];

static int           status_colors[MAXBLSTATS];
static unsigned long active_conditions;
static unsigned long *cond_hilites;

static const char *cond_names[] = {
    "Stone", "Slime", "Strngl", "FoodPois", "TermIll",
    "Blind", "Deaf", "Stun", "Conf", "Hallu",
    "Lev", "Fly", "Ride"
};
#define LEGACY_COND_COUNT 13 /* conditions the status flush loop knows about */

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

static jbyteArray
create_bytearray(const char *str)
{
    int len = str ? (int) strlen(str) : 0;
    jbyteArray a = (*jEnv)->NewByteArray(jEnv, len);
    jbyte *e = (*jEnv)->GetByteArrayElements(jEnv, a, 0);
    if (e) {
        memcpy(e, str, (size_t) len);
        (*jEnv)->ReleaseByteArrayElements(jEnv, a, e, 0);
    }
    return a;
}

static void
destroy_jobject(jobject obj)
{
    (*jEnv)->DeleteLocalRef(jEnv, obj);
}

#define JNICallV(func, ...) \
    (*jEnv)->CallVoidMethod(jEnv, jAppInstance, func, ## __VA_ARGS__)
#define JNICallI(func, ...) \
    (*jEnv)->CallIntMethod(jEnv, jAppInstance, func, ## __VA_ARGS__)
#define JNICallO(func, ...) \
    (*jEnv)->CallObjectMethod(jEnv, jAppInstance, func, ## __VA_ARGS__)

/* -------------------------------------------------------------------------
 * JNI entry points (called from Java)
 * ---------------------------------------------------------------------- */

/* Main game thread entry: called by NetHackIO.RunNetHack() */
JNIEXPORT void JNICALL
Java_com_tbd_forkfront_engine_NetHackIO_RunNetHack(JNIEnv *env, jobject thiz,
                                                   jstring path)
{
    char *params[10];
    const char *pChars;

    jEnv = env;
    jAppInstance = thiz;
    jApp = (*jEnv)->GetObjectClass(jEnv, jAppInstance);

    /* Cache all method IDs up front */
    jDebugLog       = (*jEnv)->GetMethodID(jEnv, jApp, "debugLog",        "([B)V");
    jReceiveKey     = (*jEnv)->GetMethodID(jEnv, jApp, "receiveKeyCmd",   "()I");
    jReceivePosKey  = (*jEnv)->GetMethodID(jEnv, jApp, "receivePosKeyCmd","(I[I)I");
    jCreateWindow   = (*jEnv)->GetMethodID(jEnv, jApp, "createWindow",    "(I)I");
    jClearWindow    = (*jEnv)->GetMethodID(jEnv, jApp, "clearWindow",     "(II)V");
    jDisplayWindow  = (*jEnv)->GetMethodID(jEnv, jApp, "displayWindow",   "(II)V");
    jDestroyWindow  = (*jEnv)->GetMethodID(jEnv, jApp, "destroyWindow",   "(I)V");
    jPutString      = (*jEnv)->GetMethodID(jEnv, jApp, "putString",       "(II[BII)V");
    jSetHealthColor = (*jEnv)->GetMethodID(jEnv, jApp, "setHealthColor",  "(I)V");
    jRedrawStatus   = (*jEnv)->GetMethodID(jEnv, jApp, "redrawStatus",    "()V");
    jRawPrint       = (*jEnv)->GetMethodID(jEnv, jApp, "rawPrint",        "(I[B)V");
    jSetCursorPos   = (*jEnv)->GetMethodID(jEnv, jApp, "setCursorPos",    "(III)V");
    jPrintTile      = (*jEnv)->GetMethodID(jEnv, jApp, "printTile",       "(IIIIIIII)V");
    jYNFunction     = (*jEnv)->GetMethodID(jEnv, jApp, "ynFunction",      "([B[BI)V");
    jGetLine        = (*jEnv)->GetMethodID(jEnv, jApp, "getLine",         "([BIII)Ljava/lang/String;");
    jStartMenu      = (*jEnv)->GetMethodID(jEnv, jApp, "startMenu",       "(I)V");
    jAddMenu        = (*jEnv)->GetMethodID(jEnv, jApp, "addMenu",         "(IIJIII[BII)V");
    jEndMenu        = (*jEnv)->GetMethodID(jEnv, jApp, "endMenu",         "(I[B)V");
    jSelectMenu     = (*jEnv)->GetMethodID(jEnv, jApp, "selectMenu",      "(III)[J");
    jCliparound     = (*jEnv)->GetMethodID(jEnv, jApp, "cliparound",      "(IIIIII)V");
    jDelayOutput    = (*jEnv)->GetMethodID(jEnv, jApp, "delayOutput",     "()V");
    jHighlightDPad  = (*jEnv)->GetMethodID(jEnv, jApp, "highlightDPad",   "()V");
    jShowLog        = (*jEnv)->GetMethodID(jEnv, jApp, "showLog",         "(I)V");
    jSetUsername    = (*jEnv)->GetMethodID(jEnv, jApp, "setUsername",     "([B)V");
    jSetNumPadOption= (*jEnv)->GetMethodID(jEnv, jApp, "setNumPadOption", "(I)V");
    jAskName        = (*jEnv)->GetMethodID(jEnv, jApp, "askName",         "(I[Ljava/lang/String;)Ljava/lang/String;");
    jLoadSound      = (*jEnv)->GetMethodID(jEnv, jApp, "loadSound",       "([B)V");
    jPlaySound      = (*jEnv)->GetMethodID(jEnv, jApp, "playSound",       "([BI)V");
    jGetDumplogDir  = (*jEnv)->GetMethodID(jEnv, jApp, "getDumplogDir",   "()Ljava/lang/String;");
    jStatusInit     = (*jEnv)->GetMethodID(jEnv, jApp, "statusInit",      "()V");
    jStatusEnableField = (*jEnv)->GetMethodID(jEnv, jApp, "statusEnableField",
                                              "(ILjava/lang/String;Ljava/lang/String;Z)V");
    jStatusUpdate   = (*jEnv)->GetMethodID(jEnv, jApp, "statusUpdate",
                                           "(ILjava/lang/String;JIII[J)V");
    jStatusFinish   = (*jEnv)->GetMethodID(jEnv, jApp, "statusFinish",    "()V");

    if (!(jReceiveKey && jReceivePosKey && jCreateWindow && jClearWindow
          && jDisplayWindow && jDestroyWindow && jPutString && jRawPrint
          && jSetCursorPos && jPrintTile && jYNFunction && jGetLine
          && jStartMenu && jAddMenu && jEndMenu && jSelectMenu
          && jCliparound && jDelayOutput && jHighlightDPad && jShowLog
          && jSetUsername && jSetNumPadOption && jAskName
          && jSetHealthColor && jRedrawStatus && jLoadSound && jPlaySound
          && jGetDumplogDir)) {
        debuglog("winandroid: failed to find required JNI methods");
        return;
    }

    pChars = (*jEnv)->GetStringUTFChars(jEnv, path, 0);
    if (chdir(pChars) != 0)
        debuglog("chdir to '%s' failed: %d", pChars, errno);
    (*jEnv)->ReleaseStringUTFChars(jEnv, path, pChars);

    params[0] = "nethack";
    params[1] = 0;

    NetHackMain(1, params);
}

/* Save current game state: called by Java on Activity.onPause() */
JNIEXPORT void JNICALL
Java_com_tbd_forkfront_engine_NetHackIO_SaveNetHackState(JNIEnv *env,
                                                         jobject thiz)
{
    if (!program_state.gameover && program_state.something_worth_saving)
        save_currentstate();
}

/* -------------------------------------------------------------------------
 * Lifecycle helpers used by Java
 * ---------------------------------------------------------------------- */

static boolean
SaveAndExit(void)
{
    if (!program_state.gameover && program_state.something_worth_saving) {
        program_state.done_hup = 0;
        clear_nhwindow(WIN_MESSAGE);
        pline("Saving...");
        if (dosave0()) {
            program_state.something_worth_saving = 0;
            u.uhp = -1;
            display_nhwindow(WIN_MESSAGE, TRUE);
            exit_nhwindows("Be seeing you...");
            nh_terminate(EXIT_SUCCESS);
        }
        return FALSE;
    }
    return TRUE;
}

void
quit_possible(void)
{
    if (quit_if_possible) {
        quit_if_possible = FALSE;
        if (!SaveAndExit()) {
            if (and_yn_function("Error saving game.  Quit anyway?",
                                ynchars, 'n') == 'y')
                nh_terminate(EXIT_SUCCESS);
        }
    }
}

void
set_username(void)
{
    jbyteArray username = create_bytearray(svp.plname);
    JNICallV(jSetUsername, username);
    destroy_jobject(username);
}

/* -------------------------------------------------------------------------
 * Debug logging
 * ---------------------------------------------------------------------- */

void
debuglog(const char *fmt, ...)
{
    char buf[256];

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof buf, fmt, args);
        va_end(args);
    } else {
        strcpy(buf, "(null)");
    }

    __android_log_print(ANDROID_LOG_INFO, "NetHackNative", "%s", buf);

    if (jEnv && jAppInstance && jDebugLog) {
        jbyteArray jstr = create_bytearray(buf);
        JNICallV(jDebugLog, jstr);
        destroy_jobject(jstr);
    }
}

void
and_error(const char *fmt, ...)
{
    char buf[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);

    debuglog("ERROR: %s", buf);
    nh_terminate(EXIT_FAILURE);
}

/* -------------------------------------------------------------------------
 * Window system init / teardown
 * ---------------------------------------------------------------------- */

static void
and_init_nhwindows(int *argcp UNUSED, char **argv UNUSED)
{
    iflags.window_inited = TRUE;
}

static void
and_exit_nhwindows(const char *str UNUSED)
{
    iflags.window_inited = FALSE;
}

static void
and_suspend_nhwindows(const char *str UNUSED)
{
}

static void
and_resume_nhwindows(void)
{
}

static void
and_get_nh_event(void)
{
}

/* -------------------------------------------------------------------------
 * Window lifecycle
 * ---------------------------------------------------------------------- */

static winid
and_create_nhwindow(int type)
{
    return JNICallI(jCreateWindow, type);
}

static void
and_clear_nhwindow(winid wid)
{
    JNICallV(jClearWindow, wid, Is_rogue_level(&u.uz));
}

static void
and_display_nhwindow(winid wid, boolean blocking)
{
    if (wid != WIN_MESSAGE && wid != WIN_MAP)
        blocking = TRUE;
    JNICallV(jDisplayWindow, wid, blocking);
    if (blocking)
        and_nhgetch();
}

static void
and_destroy_nhwindow(winid wid)
{
    JNICallV(jDestroyWindow, wid);
}

static void
and_curs(winid wid, int x, int y)
{
    JNICallV(jSetCursorPos, wid, x, y);
}

/* -------------------------------------------------------------------------
 * Text output
 * ---------------------------------------------------------------------- */

/* Track current text attributes so putstr(ATR_NONE) inherits them */
static int text_attribs = 0;
static int text_color   = CLR_WHITE;

void
term_start_attr(int attr)
{
    text_attribs |= 1 << attr;
}

void
term_end_attr(int attr)
{
    text_attribs &= ~(1 << attr);
}

void
term_start_color(int color)
{
    text_color = color;
}

void
term_end_color(void)
{
    text_color = CLR_WHITE;
}

/* Core putstr with explicit color — used internally for status rendering */
void
and_putstr_ex(winid wid, int attr, const char *str, int append, int nhcolor)
{
    if (!str || !*str)
        return;
    jbyteArray jstr = create_bytearray(str);
    JNICallV(jPutString, wid, attr, jstr, append, nhcolor);
    destroy_jobject(jstr);
}

static void
and_putstr(winid wid, int attr, const char *str)
{
    if (attr)
        attr = 1 << attr;
    else
        attr = text_attribs;

    and_putstr_ex(wid, attr, str, 0, text_color);

    if (wid == WIN_MESSAGE) {
        save_msg(str);
#ifdef USER_SOUNDS
        if (!restoring_msghistory)
            play_sound_for_message(str);
#endif
    }
}

static void
and_putmixed(winid wid, int attr, const char *str)
{
    genl_putmixed(wid, attr, str);
}

static void
and_display_file(const char *name, boolean complain UNUSED)
{
    dlb *f;
    char buf[BUFSZ];
    char *cr;

    and_clear_nhwindow(WIN_MESSAGE);
    f = dlb_fopen(name, "r");
    if (f) {
        winid datawin = and_create_nhwindow(NHW_TEXT);
        boolean empty = TRUE;

        while (dlb_fgets(buf, BUFSZ, f)) {
            if ((cr = strchr(buf, '\n')) != 0)
                *cr = '\0';
            if (strchr(buf, '\t') != 0)
                (void) tabexpand(buf);
            empty = FALSE;
            and_putstr(datawin, 0, buf);
        }
        (void) dlb_fclose(f);
        if (!empty)
            and_display_nhwindow(datawin, TRUE);
        and_destroy_nhwindow(datawin);
    }
}

/* -------------------------------------------------------------------------
 * Map rendering
 * ---------------------------------------------------------------------- */

static void
and_print_glyph(winid wid, coordxy x, coordxy y,
                const glyph_info *fg, const glyph_info *bg)
{
    /* Unexplored cells arrive with glyph == GLYPH_UNEXPLORED (full-screen
       redraw at display.c:1805 sends every cell, including unexplored
       ones). The Java side treats tile == -1 as "draw nothing" so the
       cell falls through to the background and the minimap shows it as
       unexplored. Without this check the unexplored sprite tiles across
       the entire map, making the dungeon look completely filled in. */
    int tile    = glyph_is_unexplored(fg->glyph) ? -1 : fg->gm.tileidx;
    int bktile  = (bg && !glyph_is_unexplored(bg->glyph) && bg->gm.tileidx >= 0)
                      ? bg->gm.tileidx : -1;
    int ch      = fg->ttychar;
    int col     = fg->gm.sym.color;
    unsigned int special = fg->gm.glyphflags;

    JNICallV(jPrintTile, wid, (int) x, (int) y, tile, bktile, ch, col,
             (int) special);
}

/* -------------------------------------------------------------------------
 * Raw output (used before window system is fully up)
 * ---------------------------------------------------------------------- */

static void
and_raw_print(const char *str)
{
    jbyteArray jstr = create_bytearray(str);
    JNICallV(jRawPrint, ATR_NONE, jstr);
    destroy_jobject(jstr);
}

static void
and_raw_print_bold(const char *str)
{
    jbyteArray jstr = create_bytearray(str);
    JNICallV(jRawPrint, ATR_BOLD, jstr);
    destroy_jobject(jstr);
}

static void
and_outrip(winid tmpwin UNUSED, int how UNUSED, time_t when UNUSED)
{
    /* RIP screen: ForkFront handles this via the game-over flow */
}

/* -------------------------------------------------------------------------
 * Menus
 * ---------------------------------------------------------------------- */

static void
and_start_menu(winid wid, unsigned long mbehavior UNUSED)
{
    JNICallV(jStartMenu, wid);
}

static void
and_add_menu(winid wid, const glyph_info *gi, const ANY_P *ident,
             char accelerator, char groupacc, int attr, int clr UNUSED,
             const char *str, unsigned int itemflags)
{
    int tile = (gi && gi->glyph != NO_GLYPH && gi->gm.tileidx >= 0)
                   ? gi->gm.tileidx : -1;
    boolean preselected = (itemflags & MENU_ITEMFLAGS_SELECTED) != 0;

    if (attr)
        attr = 1 << attr;

    jbyteArray jstr = create_bytearray(str);
    JNICallV(jAddMenu, wid, tile,
             (jlong)(ident ? ident->a_void : 0),
             (int) accelerator, (int) groupacc,
             attr, jstr, (int) preselected, -1 /* clr handled by core */);
    destroy_jobject(jstr);
}

static void
and_end_menu(winid wid, const char *prompt)
{
    jbyteArray jstr = create_bytearray(prompt ? prompt : "");
    JNICallV(jEndMenu, wid, jstr);
    destroy_jobject(jstr);
}

static int
and_select_menu_r(winid wid, int how, MENU_ITEM_P **selected, int reentry)
{
    jlongArray a;
    jlong *p, *q;
    int i, n;

    a = (jlongArray) JNICallO(jSelectMenu, wid, how, reentry);
    *selected = 0;

    if (a == 0)
        return -1;

    n = (*jEnv)->GetArrayLength(jEnv, a);

    if (n > 1) {
        n >>= 1; /* each selection is (id, count) pair */
        q = p = (*jEnv)->GetLongArrayElements(jEnv, a, 0);
        *selected = (MENU_ITEM_P *) malloc(sizeof(menu_item) * (size_t) n);
        for (i = 0; i < n; i++) {
            (*selected)[i].item.a_void = (genericptr_t) *p++;
            (*selected)[i].count       = (long) *p++;
            (*selected)[i].itemflags   = MENU_ITEMFLAGS_NONE;
        }
        (*jEnv)->ReleaseLongArrayElements(jEnv, a, q, 0);
    } else if (n == 1) {
        /* special signal: app wants to save-and-exit */
        if (!program_state.gameover && program_state.something_worth_saving) {
            n = 0;
        } else {
            n = and_select_menu_r(wid, how, selected, 1);
        }
    }

    destroy_jobject(a);
    return n;
}

static int
and_select_menu(winid wid, int how, MENU_ITEM_P **selected)
{
    return and_select_menu_r(wid, how, selected, 0);
}

static char
and_message_menu(char let UNUSED, int how UNUSED, const char *mesg)
{
    pline("%s", mesg);
    return 0;
}

/* -------------------------------------------------------------------------
 * Player character selection
 * ---------------------------------------------------------------------- */

static void
and_player_selection(void)
{
    int i, result;
    char thisch, lastch = 0;
    int state = 0;
    winid win;
    anything any;
    menu_item *selected = 0;

    rigid_role_checks();

    while (flags.initalign < 0) {
        if (state < 2) {
            if (!state)
                flags.initrole = -1;
            flags.initrace = -1;
            state = 0;
        } else {
            state &= 1;
        }
        flags.initgend  = -1;
        flags.initalign = -1;

        /* Role selection */
        result = 1;
        if (flags.initrole < 0) {
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any = cg.zeroany;
            any.a_int = randrole(TRUE) + 1;
            add_menu(win, &nul_glyphinfo, &any, '*', 0,
                     ATR_NONE, NO_COLOR, "Random",
                     MENU_ITEMFLAGS_NONE);
            lastch = 0;
            for (i = 0; roles[i].name.m; i++) {
                if (ok_role(i, flags.initrace, flags.initgend,
                            flags.initalign)) {
                    any.a_int = i + 1;
                    thisch = lowc(roles[i].name.m[0]);
                    if (thisch == lastch)
                        thisch = highc(thisch);
                    add_menu(win, &nul_glyphinfo, &any, thisch, 0,
                             ATR_NONE, NO_COLOR, roles[i].name.m,
                             MENU_ITEMFLAGS_NONE);
                    lastch = thisch;
                }
            }
            end_menu(win, "Pick a role");
            result = select_menu(win, PICK_ONE, &selected);
            destroy_nhwindow(win);
            if (result > 0)
                flags.initrole = selected[0].item.a_int - 1;
            if (selected)
                free((genericptr_t) selected), selected = 0;
        }

        if (result <= 0) {
            clearlocks();
            and_exit_nhwindows("bye");
            nh_terminate(EXIT_SUCCESS);
        }

        /* Race selection */
        if (flags.initrace < 0)
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RIGID);
        result = 1;
        if (flags.initrace < 0) {
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any = cg.zeroany;
            any.a_int = randrace(flags.initrole) + 1;
            add_menu(win, &nul_glyphinfo, &any, '*', 0,
                     ATR_NONE, NO_COLOR, "random", MENU_ITEMFLAGS_NONE);
            for (i = 0; races[i].noun; i++) {
                if (ok_race(flags.initrole, i, flags.initgend,
                            flags.initalign)) {
                    any.a_int = i + 1;
                    add_menu(win, &nul_glyphinfo, &any, races[i].noun[0], 0,
                             ATR_NONE, NO_COLOR, races[i].noun,
                             MENU_ITEMFLAGS_NONE);
                }
            }
            end_menu(win, "Pick a race");
            result = select_menu(win, PICK_ONE, &selected);
            destroy_nhwindow(win);
            if (result > 0) {
                flags.initrace = selected[0].item.a_int - 1;
                state |= 1;
            }
            if (selected)
                free((genericptr_t) selected), selected = 0;
        }
        if (result <= 0)
            continue;

        /* Gender selection */
        flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                   flags.initalign, PICK_RIGID);
        result = 1;
        if (flags.initgend < 0) {
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any = cg.zeroany;
            any.a_int = randgend(flags.initrole, flags.initrace) + 1;
            add_menu(win, &nul_glyphinfo, &any, '*', 0,
                     ATR_NONE, NO_COLOR, "random", MENU_ITEMFLAGS_NONE);
            for (i = 0; i < ROLE_GENDERS; i++) {
                if (ok_gend(flags.initrole, flags.initrace, i,
                            flags.initalign)) {
                    any.a_int = i + 1;
                    add_menu(win, &nul_glyphinfo, &any, genders[i].adj[0], 0,
                             ATR_NONE, NO_COLOR, genders[i].adj,
                             MENU_ITEMFLAGS_NONE);
                }
            }
            end_menu(win, "Pick a gender");
            result = select_menu(win, PICK_ONE, &selected);
            destroy_nhwindow(win);
            if (result > 0) {
                flags.initgend = selected[0].item.a_int - 1;
                state |= 2;
            }
            if (selected)
                free((genericptr_t) selected), selected = 0;
        }
        if (result <= 0)
            continue;

        /* Alignment selection */
        flags.initalign = pick_align(flags.initrole, flags.initrace,
                                     flags.initgend, PICK_RIGID);
        result = 1;
        if (flags.initalign < 0) {
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any = cg.zeroany;
            any.a_int = randalign(flags.initrole, flags.initrace) + 1;
            add_menu(win, &nul_glyphinfo, &any, '*', 0,
                     ATR_NONE, NO_COLOR, "random", MENU_ITEMFLAGS_NONE);
            for (i = 0; i < ROLE_ALIGNS; i++) {
                if (ok_align(flags.initrole, flags.initrace, flags.initgend,
                             i)) {
                    any.a_int = i + 1;
                    add_menu(win, &nul_glyphinfo, &any, aligns[i].adj[0], 0,
                             ATR_NONE, NO_COLOR, aligns[i].adj,
                             MENU_ITEMFLAGS_NONE);
                }
            }
            end_menu(win, "Pick an alignment");
            result = select_menu(win, PICK_ONE, &selected);
            destroy_nhwindow(win);
            if (result > 0)
                flags.initalign = selected[0].item.a_int - 1;
            if (selected)
                free((genericptr_t) selected), selected = 0;
        }
    }
}

/* -------------------------------------------------------------------------
 * Input
 * ---------------------------------------------------------------------- */

static int
and_nhgetch(void)
{
    int c;

    if (!and_input_buffer_is_empty())
        return (int) and_input_buffer_pop();

    c = JNICallI(jReceiveKey);

    quit_if_possible = FALSE;
    if (c == 0x80) {
        if (!program_state.gameover && program_state.something_worth_saving) {
            quit_if_possible = TRUE;
            return '\033';
        }
        return and_nhgetch();
    }
    return c;
}

static boolean bMouseLock;

void
lock_mouse_cursor(boolean bLock)
{
    bMouseLock = bLock;
}

static int
and_nh_poskey(coordxy *x, coordxy *y, int *mod)
{
    int c;

    if (!and_input_buffer_is_empty())
        return (int) and_input_buffer_pop();

    jintArray a = (*jEnv)->NewIntArray(jEnv, 2);
    c = JNICallI(jReceivePosKey, bMouseLock, a);
    if (!c) {
        int *e = (*jEnv)->GetIntArrayElements(jEnv, a, 0);
        *x   = (coordxy) e[0];
        *y   = (coordxy) e[1];
        *mod = CLICK_1;
        (*jEnv)->ReleaseIntArrayElements(jEnv, a, e, 0);
    }
    quit_if_possible = FALSE;
    if (c == 0x80) {
        if (!program_state.gameover && program_state.something_worth_saving) {
            quit_if_possible = TRUE;
            return '\033';
        }
        return and_nh_poskey(x, y, mod);
    }
    destroy_jobject(a);
    return c;
}

static void
and_nhbell(void)
{
}

static int
and_doprev_message(void)
{
    JNICallV(jShowLog, 1);
    and_nhgetch();
    return 0;
}

/* Show log window after death */
void
and_you_die(void)
{
    JNICallV(jShowLog, 1);
    and_nhgetch();
}

/* -------------------------------------------------------------------------
 * Prompt functions
 * ---------------------------------------------------------------------- */

static char
and_yn_function(const char *question, const char *choices, char def)
{
    char ch;
    char message[BUFSZ];
    char res_ch[2];
    boolean digit_ok, allow_num;
    int esc, nChoices = 0;

    if (choices) {
        char *p;
        nChoices = (int) strlen(choices);
        p = strchr(choices, '\033');
        if (p)
            esc = (int) (p - choices);
        else
            esc = -1;
    } else {
        esc = -1;
    }
    allow_num = choices && strchr(choices, '#');

    /* Short yes/no prompts with ≤4 choices: show a popup dialog.
       Some 5.0 prompts use "\033x" to hide extra choices in the text
       prompt (e.g. "ynq\033a").  Strip \033 for the popup — count only
       non-escape characters and send just those to Java so the user
       sees all options as tappable buttons. */
    if (iflags.force_invmenu && choices && !allow_num) {
        const char *cp;
        int vis = 0;

        for (cp = choices; *cp; cp++)
            if (*cp != '\033')
                vis++;

        if (vis <= 4) {
            jbyteArray jq = create_bytearray(question);
            jbyteArray jb = (*jEnv)->NewByteArray(jEnv, vis);
            jbyte *pTmp = (*jEnv)->GetByteArrayElements(jEnv, jb, 0);
            int idx = 0;

            for (cp = choices; *cp; cp++)
                if (*cp != '\033')
                    pTmp[idx++] = (jbyte) *cp;
            (*jEnv)->ReleaseByteArrayElements(jEnv, jb, pTmp, 0);
            JNICallV(jYNFunction, jq, jb, def);
            destroy_jobject(jq);
            destroy_jobject(jb);
            return and_nhgetch();
        }
    }

    if (choices) {
        char choicebuf[QBUFSZ];
        Strcpy(choicebuf, choices);
        if (esc >= 0)
            choicebuf[esc] = '\0';
        Sprintf(message, "%s [%s]", question, choicebuf);
        if (def)
            Sprintf(eos(message), "(%c) ", def);
    } else {
        Strcpy(message, question);
        Strcat(message, " ");
    }

    /* Directional prompts: delegate to poskey for touch input */
    if (strstr(question, "what direction")) {
        and_clear_nhwindow(WIN_MESSAGE);
        and_putstr(WIN_MESSAGE, ATR_BOLD, message);
        if (iflags.force_invmenu) {
            JNICallV(jHighlightDPad);
            return (char) and_nhgetch();
        } else {
            coordxy mx = (coordxy) u.ux, my = (coordxy) u.uy;
            int mmod = 0;
            int mc = and_nh_poskey(&mx, &my, &mmod);
            if (!mc) {
                int dx = (int) mx - (int) u.ux;
                int dy = (int) my - (int) u.uy;
                if      (dx >  2 * abs(dy)) { dx =  1; dy = 0; }
                else if (dy >  2 * abs(dx)) { dx =  0; dy = 1; }
                else if (dx < -2 * abs(dy)) { dx = -1; dy = 0; }
                else if (dy < -2 * abs(dx)) { dx =  0; dy = -1; }
                else { dx = sgn(dx); dy = sgn(dy); }
                return (dx == 0 && dy == 0) ? '.' : (char) xytodir(dx, dy);
            }
            return (char) mc;
        }
    }

    and_putstr(WIN_MESSAGE, ATR_BOLD, message);

    ch = 0;
    do {
        ch = (char) and_nhgetch();
        if (choices)
            ch = lowc(ch);
        else
            break;

        digit_ok = allow_num && digit(ch);
        if (ch == '\033') {
            if (strchr(choices, 'q'))
                ch = 'q';
            else if (strchr(choices, 'n'))
                ch = 'n';
            else
                ch = def;
            break;
        } else if (strchr(quitchars, ch)) {
            ch = def;
            break;
        } else if (!strchr(choices, ch) && !digit_ok) {
            and_nhbell();
            ch = (char) 0;
        } else if (ch == '#' || digit_ok) {
            char z, digit_string[2];
            int  n_len = 0;
            long value = 0;

            and_putstr_ex(WIN_MESSAGE, 1 << ATR_BOLD, "#", 1, CLR_WHITE);
            n_len++;
            digit_string[1] = '\0';
            if (ch != '#') {
                digit_string[0] = ch;
                and_putstr_ex(WIN_MESSAGE, 1 << ATR_BOLD, digit_string, 1,
                              CLR_WHITE);
                n_len++;
                value = ch - '0';
                ch = '#';
            }
            do {
                z = lowc((char) readchar());
                if (digit(z)) {
                    value = (10 * value) + (z - '0');
                    if (value < 0)
                        break;
                    digit_string[0] = z;
                    and_putstr_ex(WIN_MESSAGE, 1 << ATR_BOLD, digit_string, 0,
                                  CLR_WHITE);
                    n_len++;
                } else if (z == 'y' || strchr(quitchars, z)) {
                    if (z == '\033')
                        value = -1;
                    z = '\n';
                } else if (z == 0x7f) {
                    if (n_len <= 1) {
                        value = -1;
                        break;
                    }
                    value /= 10;
                    and_putstr_ex(WIN_MESSAGE, 1 << ATR_BOLD, digit_string,
                                  -2, CLR_WHITE);
                    n_len--;
                } else {
                    value = -1;
                    and_nhbell();
                    break;
                }
            } while (z != '\n');

            if (value > 0)
                yn_number = value;
            else if (value == 0)
                ch = 'n';
            else {
                and_putstr_ex(WIN_MESSAGE, 1 << ATR_BOLD, digit_string,
                              -n_len - 1, CLR_WHITE);
                n_len = 0;
                ch = (char) 0;
            }
        }
    } while (!ch);

    if (choices && isprint((unsigned char) ch) && ch != '#') {
        res_ch[0] = ch;
        res_ch[1] = '\0';
        and_putstr_ex(WIN_MESSAGE, 1 << ATR_BOLD, res_ch, 1, CLR_WHITE);
    } else if (!choices) {
        and_clear_nhwindow(WIN_MESSAGE);
    }

    return ch;
}

/* Internal getlin implementation with reentry for save-and-exit */
static void
and_n_getline_r(const char *question, char *buf, int nMax, int showLog,
                int reentry)
{
    jstring jstr;
    jbyteArray jq;
    const jchar *pChars;
    int i, n;

    jq   = create_bytearray(question);
    jstr = (jstring) JNICallO(jGetLine, jq, nMax, showLog, reentry);
    destroy_jobject(jq);

    n = (int) (*jEnv)->GetStringLength(jEnv, jstr);
    if (n >= nMax)
        n = nMax - 1;
    i = 0;
    if (n > 0) {
        pChars = (*jEnv)->GetStringChars(jEnv, jstr, 0);
        if (*pChars == 0x80) {
            if (!program_state.gameover
                && program_state.something_worth_saving) {
                buf[0] = '\033';
                i = 1;
            } else {
                (*jEnv)->ReleaseStringChars(jEnv, jstr, pChars);
                destroy_jobject(jstr);
                and_n_getline_r(question, buf, nMax, showLog, 1);
                return;
            }
        } else if (*pChars == '\033') {
            buf[0] = '\033';
            i = 1;
        } else {
            for (; i < n; i++)
                buf[i] = isprint((unsigned char) pChars[i])
                         ? (char) pChars[i] : '?';
        }
        (*jEnv)->ReleaseStringChars(jEnv, jstr, pChars);
    }
    destroy_jobject(jstr);
    buf[i] = '\0';
}

static void
and_getlin(const char *question, char *input)
{
    and_n_getline_r(question, input, BUFSZ, FALSE, 0);
}

void
and_getlin_log(const char *question, char *input)
{
    and_n_getline_r(question, input, BUFSZ, TRUE, 0);
}

/* -------------------------------------------------------------------------
 * askname — character / save selection
 * ---------------------------------------------------------------------- */

static void
and_askname(void)
{
    const jchar *pChars;
    jstring jstr;
    int i, n, w;

    char **saves = get_saved_games();
    int nSaves = 0;
    while (saves && saves[nSaves])
        nSaves++;

    jclass stringClass = (*jEnv)->FindClass(jEnv, "java/lang/String");
    jobjectArray strings = (*jEnv)->NewObjectArray(jEnv, nSaves, stringClass, 0);
    for (i = 0; i < nSaves; i++)
        (*jEnv)->SetObjectArrayElement(jEnv, strings, i,
                                       (*jEnv)->NewStringUTF(jEnv, saves[i]));

    jstr = (jstring) JNICallO(jAskName, PL_NSIZ, strings);

    for (i = 0; i < nSaves; i++)
        destroy_jobject((*jEnv)->GetObjectArrayElement(jEnv, strings, i));
    destroy_jobject(strings);

    n = (int) (*jEnv)->GetStringLength(jEnv, jstr) - 1;
    w = n;
    if (n >= PL_NSIZ)
        n = PL_NSIZ - 1;
    i = 0;
    if (n > 0) {
        pChars = (*jEnv)->GetStringChars(jEnv, jstr, 0);
        if (*pChars == 0x80 || *pChars == '\033') {
            clearlocks();
            and_exit_nhwindows("bye");
            nh_terminate(EXIT_SUCCESS);
        }
        if (pChars[w] == '1')
            wizard = TRUE;
        for (; i < n; i++)
            svp.plname[i] = isprint((unsigned char) pChars[i])
                            ? (char) pChars[i] : '?';
        (*jEnv)->ReleaseStringChars(jEnv, jstr, pChars);
    }
    svp.plname[i] = '\0';
    destroy_jobject(jstr);
}

/* -------------------------------------------------------------------------
 * Extended commands
 * ---------------------------------------------------------------------- */

static const char *
complete_ext_cmd(const char *base)
{
    int i, icmd = -1;

    for (i = 0; extcmdlist[i].ef_txt != (char *) 0; i++) {
        if (!strncmpi(base, extcmdlist[i].ef_txt, strlen(base))) {
            if (icmd == -1)
                icmd = i;
            else
                return 0; /* ambiguous */
        }
    }
    return (icmd >= 0) ? extcmdlist[icmd].ef_txt : 0;
}

static void
get_ext_cmd_auto(const char *query, char *bufp)
{
    int n = 0, nl = 0, c;
    const char *complete = 0;
    const int maxc = COLNO >= BUFSZ ? BUFSZ - 1 : COLNO;

    pline("%s ", query);
    bufp[n] = '\0';
    for (;;) {
        c = and_nhgetch();
        if (c == EOF || c == '\n') {
            bufp[n] = '\0';
            if (complete)
                Strcpy(bufp, complete);
            save_msg(bufp);
            break;
        }
        if (c == '\033') {
            bufp[0] = (char) c;
            bufp[1] = '\0';
            break;
        }
        if (c == 0x7f) {
            if (n > 0)
                bufp[--n] = '\0';
        } else if (' ' <= (unsigned char) c && n < maxc) {
            bufp[n]   = (char) c;
            bufp[++n] = '\0';
        }
        complete = complete_ext_cmd(bufp);
        and_putstr_ex(WIN_MESSAGE, 0, bufp, -nl - 1, CLR_WHITE);
        if (complete)
            and_putstr_ex(WIN_MESSAGE, 1 << ATR_INVERSE, complete + n, 1,
                          CLR_WHITE);
        nl = complete ? (int) strlen(complete) : n;
    }
    clear_nhwindow(WIN_MESSAGE);
}

static int
do_ext_cmd_text(void)
{
    int i;
    char buf[BUFSZ];

    get_ext_cmd_auto("#", buf);
    (void) mungspaces(buf);
    if (buf[0] == '\0' || buf[0] == '\033')
        return -1;

    for (i = 0; extcmdlist[i].ef_txt != (char *) 0; i++)
        if (!strcmpi(buf, extcmdlist[i].ef_txt))
            break;

    if (!gi.in_doagain) {
        int j;
        for (j = 0; buf[j]; j++)
            cmdq_add_key(CQ_REPEAT, buf[j]);
        cmdq_add_key(CQ_REPEAT, '\n');
    }

    if (extcmdlist[i].ef_txt == (char *) 0) {
        pline("%s: unknown extended command.", buf);
        return -1;
    }
    return i;
}

static int
do_ext_cmd_menu(boolean complete)
{
    winid win;
    int i, count, what;
    menu_item *selected = NULL;
    anything any;
    char accelerator = 'a';
    const char *ptr;

    win = and_create_nhwindow(NHW_MENU);
    and_start_menu(win, MENU_BEHAVE_STANDARD);
    for (i = 0; (ptr = extcmdlist[i].ef_txt); i++) {
        int flgs = extcmdlist[i].flags;
        if ((flgs & WIZMODECMD) && !wizard)
            continue;
        if (!complete && !(flgs & AUTOCOMPLETE) && !(flgs & WIZMODECMD))
            continue;
        any = cg.zeroany;
        any.a_int = i + 1;
        and_add_menu(win, &nul_glyphinfo, &any, accelerator, 0,
                     ATR_NONE, NO_COLOR, ptr, MENU_ITEMFLAGS_NONE);
        if      (accelerator == 'z') accelerator = 'A';
        else if (accelerator == 'Z') accelerator = 0;
        else                         accelerator++;
    }
    any = cg.zeroany;
    any.a_int = i + 1;
    if (!complete)
        and_add_menu(win, &nul_glyphinfo, &any, '*', 0,
                     ATR_NONE, NO_COLOR, "(list everything)",
                     MENU_ITEMFLAGS_NONE);
    and_end_menu(win, "Extended command");
    count = and_select_menu(win, PICK_ONE, &selected);
    what  = (count > 0) ? selected->item.a_int - 1 : -1;
    if (selected)
        free(selected);
    and_destroy_nhwindow(win);

    return (what == any.a_int - 1) ? do_ext_cmd_menu(TRUE) : what;
}

static int
and_get_ext_cmd(void)
{
    if (!and_input_buffer_is_empty()) {
        int i;
        char *buffer = and_input_buffer_pop_all();
        for (i = 0; extcmdlist[i].ef_txt != (char *) 0; i++)
            if (!strcmpi(extcmdlist[i].ef_txt, buffer))
                return i;
    }
    if (iflags.extmenu)
        return do_ext_cmd_menu(FALSE);
    return do_ext_cmd_text();
}

/* -------------------------------------------------------------------------
 * Miscellaneous
 * ---------------------------------------------------------------------- */

static void
and_number_pad(int state)
{
    JNICallV(jSetNumPadOption, state);
}

static void
and_delay_output(void)
{
    JNICallV(jDelayOutput);
}

static void
and_mark_synch(void)
{
}

static void
and_wait_synch(void)
{
}

static void
and_update_inventory(int mode UNUSED)
{
    /* Persistent inventory not yet implemented; no-op */
}

static win_request_info *
and_ctrl_nhwindow(winid window UNUSED, int request UNUSED,
                  win_request_info *wri UNUSED)
{
    return (win_request_info *) 0;
}

#ifdef CLIPPING
static void
and_cliparound(int x, int y)
{
    int objFlags = 0;
    int monMask  = 0;
    int dx, dy;
    stairway *stway;

    struct obj *otmp = svl.level.objects[u.ux][u.uy];
    if (otmp) {
        objFlags |= 1;
        if (Is_container(otmp) || otmp->otyp == STATUE) objFlags |= 2;
        if (is_edible(otmp)) objFlags |= 4;
    }

    stway = stairway_at(u.ux, u.uy);
    if (stway) {
        if (stway->up) objFlags |= 8;
        else objFlags |= 16;
    }

    {
        struct rm *lev = &levl[u.ux][u.uy];
        if (IS_ALTAR(lev->typ))   objFlags |= 32;
        if (IS_FOUNTAIN(lev->typ) || IS_SINK(lev->typ)) objFlags |= 64;
        if (IS_THRONE(lev->typ))  objFlags |= 128;
    }

    for (dx = -1; dx <= 1; dx++) {
        for (dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = u.ux + dx, ny = u.uy + dy;
            if (isok(nx, ny) && svl.level.monsters[nx][ny])
                monMask |= (1 << ((dx + 1) + (dy + 1) * 3));
        }
    }

    JNICallV(jCliparound, x, y, (int) u.ux, (int) u.uy, objFlags, monMask);
}
#endif /* CLIPPING */

#ifdef POSITIONBAR
static void
and_update_positionbar(char *features UNUSED)
{
}
#endif

/* -------------------------------------------------------------------------
 * Color support
 * ---------------------------------------------------------------------- */

#ifdef CHANGE_COLOR
static void
and_change_color(int color_number UNUSED, long rgb UNUSED, int reverse UNUSED)
{
}

static char *
and_get_color_string(void)
{
    return (char *) 0;
}
#endif /* CHANGE_COLOR */

/* -------------------------------------------------------------------------
 * Status window
 * ---------------------------------------------------------------------- */

static void
and_set_health_color(int nhcolor)
{
    JNICallV(jSetHealthColor, nhcolor);
}

static void
and_bot_updated(void)
{
    JNICallV(jRedrawStatus);
}

static int
hl_attridx_to_attrmask(int idx)
{
    switch (idx) {
    case HL_ATTCLR_DIM:     return 1 << ATR_DIM;
    case HL_ATTCLR_BLINK:   return 1 << ATR_BLINK;
    case HL_ATTCLR_ULINE:   return 1 << ATR_ULINE;
    case HL_ATTCLR_INVERSE: return 1 << ATR_INVERSE;
    case HL_ATTCLR_BOLD:    return 1 << ATR_BOLD;
    default:                return 0;
    }
}

static int
hl_attrmask_to_attrmask(int mask)
{
    int attr = 0;
    if (mask & HL_DIM)     attr |= 1 << ATR_DIM;
    if (mask & HL_BLINK)   attr |= 1 << ATR_BLINK;
    if (mask & HL_ULINE)   attr |= 1 << ATR_ULINE;
    if (mask & HL_INVERSE) attr |= 1 << ATR_INVERSE;
    if (mask & HL_BOLD)    attr |= 1 << ATR_BOLD;
    return attr;
}

static void
and_status_init(void)
{
    (*jEnv)->CallVoidMethod(jEnv, jAppInstance, jStatusInit);
}

static void
and_status_finish(void)
{
    (*jEnv)->CallVoidMethod(jEnv, jAppInstance, jStatusFinish);
}

static void
and_status_enablefield(int fieldidx, const char *nm, const char *fmt,
                       boolean enable)
{
    status_activefields[fieldidx] = enable;

    jstring jName = (*jEnv)->NewStringUTF(jEnv, nm);
    jstring jFmt  = (*jEnv)->NewStringUTF(jEnv, fmt ? fmt : "");
    (*jEnv)->CallVoidMethod(jEnv, jAppInstance, jStatusEnableField,
                            fieldidx, jName, jFmt, (jboolean) enable);
    (*jEnv)->DeleteLocalRef(jEnv, jName);
    (*jEnv)->DeleteLocalRef(jEnv, jFmt);
}

static void
and_status_update(int idx, genericptr_t ptr, int chg, int percent, int color,
                  unsigned long *colormasks)
{
    long conditionMask = 0;
    char *text = (char *) ptr;
    jstring jValue = NULL;
    jlongArray jColorMasks = NULL;

    if (idx == BL_FLUSH || idx == BL_RESET) {
        (*jEnv)->CallVoidMethod(jEnv, jAppInstance, jStatusUpdate,
                                idx, NULL, (jlong) 0L, chg, percent, color,
                                NULL);
        return;
    }

    if (idx == BL_CONDITION) {
        long *condptr = (long *) ptr;
        conditionMask = condptr ? *condptr : 0L;
        active_conditions = (unsigned long) conditionMask;
        cond_hilites = colormasks;

        if (colormasks) {
            jColorMasks = (*jEnv)->NewLongArray(jEnv, BL_ATTCLR_MAX);
            if (jColorMasks) {
                jlong temp[BL_ATTCLR_MAX];
                int i;
                for (i = 0; i < BL_ATTCLR_MAX; i++)
                    temp[i] = (jlong) colormasks[i];
                (*jEnv)->SetLongArrayRegion(jEnv, jColorMasks, 0,
                                           BL_ATTCLR_MAX, temp);
            }
        }
        (*jEnv)->CallVoidMethod(jEnv, jAppInstance, jStatusUpdate,
                                idx, NULL, (jlong) conditionMask,
                                chg, percent, color, jColorMasks);
        if (jColorMasks)
            (*jEnv)->DeleteLocalRef(jEnv, jColorMasks);
        return;
    }

    /* BL_WEAPON, BL_ARMOR, BL_TERRAIN and all regular fields: pass as string */
    if (text && status_activefields[idx]) {
        /* BL_GOLD: skip \GXXXXNNNN glyph encoding prefix */
        if (idx == BL_GOLD && text[0] == '\\')
            text += 10;

        /* Cache color for the C-side status rendering path */
        status_colors[idx] = color;

        jValue = (*jEnv)->NewStringUTF(jEnv, text);
        (*jEnv)->CallVoidMethod(jEnv, jAppInstance, jStatusUpdate,
                                idx, jValue, (jlong) 0L,
                                chg, percent, color, NULL);
        (*jEnv)->DeleteLocalRef(jEnv, jValue);
    }
}

/* -------------------------------------------------------------------------
 * C-side status rendering (fallback / used by and_bot_updated)
 * ---------------------------------------------------------------------- */

static int
get_condition_color(unsigned long cond_mask)
{
    int i;
    if (!cond_hilites) return CLR_WHITE;
    for (i = 0; i < CLR_MAX; i++)
        if (cond_hilites[i] & cond_mask)
            return i;
    return CLR_WHITE;
}

static int
get_condition_attr(unsigned long cond_mask)
{
    int i, attr = 0;
    if (!cond_hilites) return 0;
    for (i = CLR_MAX; i < BL_ATTCLR_MAX; i++)
        if (cond_hilites[i] & cond_mask)
            attr |= hl_attridx_to_attrmask(i);
    return attr;
}

static void
print_conditions(const char **names)
{
    int i;
    for (i = 0; i < LEGACY_COND_COUNT; i++) {
        unsigned long cond_mask = 1UL << i;
        if (active_conditions & cond_mask) {
            int color = get_condition_color(cond_mask);
            int attr  = get_condition_attr(cond_mask);
            and_putstr_ex(WIN_STATUS, ATR_NONE, " ", 0, CLR_WHITE);
            and_putstr_ex(WIN_STATUS, attr, names[i], 0, color);
        }
    }
}

static void
print_status_field(int idx, boolean first_field)
{
    const char *val;

    if (!status_activefields[idx])
        return;
    val = status_vals[idx];
    if (!val)
        return;

    if (first_field && *val == ' ')
        val++;
    else if (idx == BL_LEVELDESC && !first_field)
        and_putstr_ex(WIN_STATUS, ATR_NONE, " ", 0, CLR_WHITE);

    while (*val == ' ') {
        and_putstr_ex(WIN_STATUS, ATR_NONE, " ", 0, CLR_WHITE);
        val++;
    }

    if (idx == BL_CONDITION) {
        print_conditions(cond_names);
    } else {
        int attr  = (status_colors[idx] >> 8) & 0xFF;
        int color = status_colors[idx] & 0xFF;

        if (idx == BL_HP) {
            and_set_health_color(color);
        } else if (idx == BL_HPMAX && color == NO_COLOR
                   && attr == ATR_NONE && status_activefields[BL_HP]) {
            attr  = (status_colors[BL_HP] >> 8) & 0xFF;
            color = status_colors[BL_HP] & 0xFF;
        } else if (idx == BL_ENEMAX && color == NO_COLOR
                   && attr == ATR_NONE && status_activefields[BL_ENE]) {
            attr  = (status_colors[BL_ENE] >> 8) & 0xFF;
            color = status_colors[BL_ENE] & 0xFF;
        }
        and_putstr_ex(WIN_STATUS, hl_attrmask_to_attrmask(attr), val, 0,
                      color);
    }
}

/* Redraw status lines using C-side putstr (backup path) */
void
and_status_flush(void)
{
    static enum statusfields fieldorder_line1[] = {
        BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
        BL_ALIGN, BL_SCORE, BL_FLUSH
    };
    static enum statusfields fieldorder_line2[] = {
        BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
        BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER, BL_CAP,
        BL_CONDITION, BL_FLUSH
    };

    int i;
    enum statusfields idx;

    curs(WIN_STATUS, 1, 0);
    for (i = 0; (idx = fieldorder_line1[i]) != BL_FLUSH; ++i)
        print_status_field(idx, i == 0);

    curs(WIN_STATUS, 1, 1);
    for (i = 0; (idx = fieldorder_line2[i]) != BL_FLUSH; ++i)
        print_status_field(idx, i == 0);

    and_bot_updated();
}

/* -------------------------------------------------------------------------
 * Message history
 * ---------------------------------------------------------------------- */

void
save_msg(const char *msg)
{
    if (!msg || !*msg || !strcmp("Restoring save file...", msg))
        return;
    if (msghistory[msghistory_idx])
        free(msghistory[msghistory_idx]);
    msghistory[msghistory_idx] = strdup(msg);
    msghistory_idx = add_msghistory_idx(msghistory_idx);
}

static char *
and_getmsghistory(boolean init)
{
    if (init) {
        msghistory_idx0 = msghistory_idx;
        for (;;) {
            if (msghistory[msghistory_idx0])
                return msghistory[msghistory_idx0];
            msghistory_idx0 = add_msghistory_idx(msghistory_idx0);
            if (msghistory_idx0 == msghistory_idx)
                return 0;
        }
    } else {
        msghistory_idx0 = add_msghistory_idx(msghistory_idx0);
        if (msghistory_idx0 == msghistory_idx)
            return 0;
        return msghistory[msghistory_idx0];
    }
}

static void
and_putmsghistory(const char *msg, boolean restoring)
{
    if (!msg) return;
    if (restoring) {
        restoring_msghistory = TRUE;
        and_putstr(WIN_MESSAGE, ATR_NONE, msg);
        restoring_msghistory = FALSE;
    }
}

/* -------------------------------------------------------------------------
 * Sound support
 * ---------------------------------------------------------------------- */

int
doshowlog(void)
{
    JNICallV(jShowLog, 0);
    return 0;
}

#ifdef USER_SOUNDS
void
load_usersound(const char *filename)
{
    jbyteArray jstr = create_bytearray(filename);
    JNICallV(jLoadSound, jstr);
    destroy_jobject(jstr);
}

void
play_usersound(const char *filename, int volume)
{
    jbyteArray jstr = create_bytearray(filename);
    JNICallV(jPlaySound, jstr, volume);
    destroy_jobject(jstr);
}
#endif /* USER_SOUNDS */

/* -------------------------------------------------------------------------
 * Dumplog directory
 * ---------------------------------------------------------------------- */

#ifdef DUMPLOG
void
and_get_dumplog_dir(char *buf)
{
    jstring jstr;
    const jchar *pChars;
    int i, n;

    jstr = (jstring) JNICallO(jGetDumplogDir);
    n = (int) (*jEnv)->GetStringLength(jEnv, jstr);

    if (n > 0 && n < BUFSZ - 1) {
        pChars = (*jEnv)->GetStringChars(jEnv, jstr, 0);
        for (i = 0; i < n; i++)
            buf[i] = (char) pChars[i];
        (*jEnv)->ReleaseStringChars(jEnv, jstr, pChars);
        if (buf[n - 1] != '/')
            buf[n++] = '/';
    } else {
        n = 0;
    }
    buf[n] = '\0';
    destroy_jobject(jstr);
}
#endif /* DUMPLOG */
