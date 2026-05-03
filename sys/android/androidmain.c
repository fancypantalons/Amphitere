/* NetHack 5.0  androidmain.c */
/* Android entry point - based on unixmain.c */

#include "hack.h"
#include "dlb.h"
#include <setjmp.h>
#include <sys/stat.h>
#include <pwd.h>
#ifndef O_RDONLY
#include <fcntl.h>
#endif

static jmp_buf nethack_exit_jmp;

static void process_options(int, char **);
static void wd_message(void);

/* Called by nethack_exit() / nh_terminate() to return control to JNI */
void
nethack_exit(int code)
{
    longjmp(nethack_exit_jmp, code + 1); /* +1 so 0 exit maps to nonzero jmp */
}

/* Remove the lock file for a given game file */
void
remove_lock_file(const char *filename)
{
    char locknambuf[BUFSZ];

#ifdef NO_FILE_LINKS
    Strcpy(locknambuf, LOCKDIR);
    Strcat(locknambuf, "/");
    Strcat(locknambuf, filename);
#else
    Strcpy(locknambuf, filename);
#endif
    Strcat(locknambuf, "_lock");
    unlink(locknambuf);
}

/* Write Android-specific save metadata sidecar for the Java UI */
static void
write_save_meta(void)
{
    FILE *fp;
    char metafile[BUFSZ];
    const char *role_name, *race_name, *gender, *alignment;

    Sprintf(metafile, "%s.meta", fqname(gs.SAVEF, SAVEPREFIX, 0));
    fp = fopen(metafile, "w");
    if (!fp)
        return;

    role_name = (flags.female && gu.urole.name.f) ? gu.urole.name.f : gu.urole.name.m;
    race_name = gu.urace.noun;
    gender = flags.female ? "female" : "male";

    switch (u.ualign.type) {
    case A_LAWFUL:  alignment = "Lawful";   break;
    case A_NEUTRAL: alignment = "Neutral";  break;
    case A_CHAOTIC: alignment = "Chaotic";  break;
    default:        alignment = "Unaligned"; break;
    }

    fprintf(fp, "role=%s\n",      role_name ? role_name : "");
    fprintf(fp, "race=%s\n",      race_name ? race_name : "");
    fprintf(fp, "gender=%s\n",    gender);
    fprintf(fp, "alignment=%s\n", alignment);
    fprintf(fp, "wizard=%d\n",    wizard ? 1 : 0);
    fclose(fp);
}

/* Main NetHack entry point, called from JNI */
int
NetHackMain(int argc, char **argv)
{
    NHFILE *nhfp;
    boolean resuming = FALSE;
    boolean plsel_once = FALSE;
    int jmpval;
    FILE *fp;

    jmpval = setjmp(nethack_exit_jmp);
    if (jmpval) {
        debuglog("NetHack exiting (code %d)", jmpval - 1);
        return 0;
    }

    early_init(argc, argv);

    gh.hname = argv[0];
    svh.hackpid = getpid();
    (void) umask(0777 & ~FCMASK);

    /* remove any dangling lock files from a previous crash */
    remove_lock_file(RECORD);
    remove_lock_file(HLOCK);

    /* ensure the score file exists */
    fp = fopen_datafile(RECORD, "a", SCOREPREFIX);
    if (fp)
        fclose(fp);

    choose_windows(DEFAULT_WINDOW_SYS);
    initoptions();
    init_nhwindows(&argc, argv);

    u.uhp = 1; /* prevent RIP on early quits */

    process_options(argc, argv);

    set_playmode();
    gp.plnamelen = 0;
    plnamesuffix(); /* strips suffix; calls askname() if plname[] empty */

    dlb_init(); /* must be before newgame() */
    vision_init();
    init_sound_disp_gamewindows();

 attempt_restore:
    if (*svp.plname)
        getlock();

    if (*svp.plname && (nhfp = restore_saved_game()) != 0) {
        const char *fq_save = fqname(gs.SAVEF, SAVEPREFIX, 1);

        pline("Restoring save file...");
        mark_synch();
        if (dorecover(nhfp)) {
            resuming = TRUE;
            write_save_meta();
            wd_message();
            if (discover || wizard) {
                if (y_n("Do you want to keep the save file?") == 'n')
                    (void) delete_savefile();
                else
                    nh_compress(fq_save);
            }
        }
    }

    if (!resuming) {
        boolean neednewlock = (!*svp.plname);

        if (!plsel_once)
            player_selection();
        plsel_once = TRUE;
        if (neednewlock && *svp.plname)
            goto attempt_restore;
        if (iflags.renameinprogress) {
            if (!gl.locknum) {
                delete_levelfile(0);
                goto attempt_restore;
            }
        }
        newgame();
        write_save_meta();
        wd_message();
    }

    moveloop(resuming);
    exit(EXIT_SUCCESS);
    return 0;
}

static void
process_options(int argc, char **argv)
{
    int i;

    while (argc > 1 && argv[1][0] == '-') {
        argv++;
        argc--;
        switch (argv[0][1]) {
        case 'D':
#ifdef WIZARD
            wizard = TRUE;
            break;
#endif
            /* fall through to discover */
        case 'X':
            discover = TRUE;
            break;
#ifdef NEWS
        case 'n':
            iflags.news = FALSE;
            break;
#endif
        case 'u':
            if (!*svp.plname) {
                if (argv[0][2])
                    (void) strncpy(svp.plname, argv[0] + 2,
                                   sizeof svp.plname - 1);
                else if (argc > 1) {
                    argc--;
                    argv++;
                    (void) strncpy(svp.plname, argv[0],
                                   sizeof svp.plname - 1);
                } else
                    raw_print("Player name expected after -u");
            }
            break;
        case 'p': /* profession (role) */
            if (argv[0][2]) {
                if ((i = str2role(&argv[0][2])) >= 0)
                    flags.initrole = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2role(argv[0])) >= 0)
                    flags.initrole = i;
            }
            break;
        case 'r': /* race */
            if (argv[0][2]) {
                if ((i = str2race(&argv[0][2])) >= 0)
                    flags.initrace = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2race(argv[0])) >= 0)
                    flags.initrace = i;
            }
            break;
        case '@':
            flags.randomall = 1;
            break;
        default:
            if ((i = str2role(&argv[0][1])) >= 0)
                flags.initrole = i;
            break;
        }
    }
}

static void
wd_message(void)
{
    if (discover)
        You("are in non-scoring discovery mode.");
}

/* Append a trailing slash to a path string if not already present */
void
append_slash(char *name)
{
    char *ptr;

    if (!*name)
        return;
    ptr = name + (strlen(name) - 1);
    if (*ptr != '/') {
        *++ptr = '/';
        *++ptr = '\0';
    }
}

unsigned long
sys_random_seed(void)
{
    unsigned long seed = 0L;
    unsigned long pid = (unsigned long) getpid();
    boolean no_seed = TRUE;
#ifdef DEV_RANDOM
    FILE *fptr;

    fptr = fopen(DEV_RANDOM, "r");
    if (fptr) {
        fread(&seed, sizeof(long), 1, fptr);
        has_strong_rngseed = TRUE;
        no_seed = FALSE;
        (void) fclose(fptr);
    } else {
        paniclog("sys_random_seed", "falling back to weak seed");
    }
#endif
    if (no_seed) {
        seed = (unsigned long) getnow();
        if (pid) {
            if (!(pid & 3L))
                pid -= 1L;
            seed *= pid;
        }
    }
    return seed;
}

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode(void)
{
    return TRUE; /* always allow on Android */
}

/* similar to above, validate explore mode access */
boolean
authorize_explore_mode(void)
{
    return TRUE; /* always allow on Android */
}

/* port-specific check for users in various lists */
boolean
check_user_string(const char *optstr UNUSED)
{
    return TRUE; /* always allow on Android */
}

/* resolve system username */
boolean
whoami(void)
{
    /* On Android, there's no meaningful system username for the app.
       NetHack will fall back to using plname from options. */
    return FALSE;
}

/* generate/retrieve unique instance ID */
void
get_nhuuid(void)
{
    /* Placeholder for Android - ideally this would use Android's
       unique ID or generate a random one and save it. */
    if (!svn.nhuuid[0]) {
        Snprintf(svn.nhuuid, sizeof svn.nhuuid, "android-session-%08lx",
                 sys_random_seed());
    }
}

void
free_nhuuid(void)
{
    int i;
    for (i = 0; i < SIZE(svn.nhuuid); i++)
        svn.nhuuid[i] = 0;
}

void
chdirx(const char *dir, boolean wr)
{
    if (dir && chdir(dir) < 0) {
        if (wr) {
            perror(dir);
            error("Cannot chdir to %s.", dir);
        }
    }
}

/* Stubs for functions not available or needed on Android */
void intron(void) { }
void introff(void) { }

int
backtrace(void **buffer UNUSED, int size UNUSED)
{
    return 0;
}

char **
backtrace_symbols(void *const *buffer UNUSED, int size UNUSED)
{
    return (char **) 0;
}
