/* NetHack 5.0  androidconf.h */
/* Android platform configuration */

#ifdef ANDROID
#ifndef ANDROIDCONF_H
#define ANDROIDCONF_H

/* Map error() to our log function */
#ifdef error
#undef error
#endif
void debuglog(const char *, ...) __attribute__((format(printf, 1, 2)));
void and_error(const char *, ...) __attribute__((format(printf, 1, 2), noreturn));
#define error and_error

/* Port features */
#ifndef DLB
#define DLB                     /* bundle data files into nhdat */
#endif
#ifdef DUMPLOG
#undef DUMPLOG
#endif
#define DUMPLOG                 /* enable end-of-game dump logs */
#ifdef SELF_RECOVER
#undef SELF_RECOVER
#endif
#define SELF_RECOVER            /* allow in-game self-recovery */
#ifdef CHANGE_COLOR
#undef CHANGE_COLOR
#endif
#define CHANGE_COLOR            /* runtime color customization */
#ifdef SELECTSAVED
#undef SELECTSAVED
#endif
#define SELECTSAVED             /* saved game selection UI */
#ifdef USER_SOUNDS
#undef USER_SOUNDS
#endif
#define USER_SOUNDS             /* user-configurable sounds */
#ifdef ANDROID_GRAPHICS
#undef ANDROID_GRAPHICS
#endif
#define ANDROID_GRAPHICS        /* include Android window port */

/* The Android port renders pre-rendered bitmap tiles, so it needs
   glyphmap[].tileidx populated by the generated src/tile.c. */
#ifndef TILES_IN_GLYPHMAP
#define TILES_IN_GLYPHMAP
#endif

#ifdef SYSCF
#undef SYSCF
#endif

/* File system */
#ifdef USE_FCNTL
#undef USE_FCNTL
#endif
#ifdef HLOCK
#undef HLOCK
#endif
#define HLOCK "perm"

#define NO_FILE_LINKS           /* no hard links on Android */
#define LOCKDIR "."             /* lock files in current directory */
#ifdef CONFIG_ERROR_SECURE
#undef CONFIG_ERROR_SECURE
#endif
#define CONFIG_ERROR_SECURE FALSE
#ifndef DEV_RANDOM
#define DEV_RANDOM "/dev/urandom"
#endif
#ifdef COMPRESS
#undef COMPRESS
#endif
#define COMPRESS "/system/bin/gzip"
#ifdef COMPRESS_EXTENSION
#undef COMPRESS_EXTENSION
#endif
#define COMPRESS_EXTENSION ".gz"

/* FALLTHROUGH fix for Clang < 10 (NDK r21 uses Clang 9) */
#if defined(__clang__) && defined(__clang_major__) && __clang_major__ < 10
# ifdef FALLTHROUGH
#  undef FALLTHROUGH
# endif
# define FALLTHROUGH do { } while (0)
#endif

/* Replace standard I/O with our implementations */
#ifdef getchar
#  undef getchar
#endif
#define getchar nhgetch
#undef tgetch
#define tgetch nhgetch

/* Fake UID: Android processes don't have meaningful Unix UIDs */
#define getuid() 1

/* Disabled Unix features */
#undef SHELL                    /* no '!' shell escape */
#undef DEF_MAILREADER           /* no mail reader */
#define NO_SIGNAL               /* no signal handling */

/* Game data */
#define CONFIG_FILE "defaults.nh"

#ifdef DUMPLOG
#undef DUMPLOG_FILE
#define DUMPLOG_FILE "%n.%d.dumplog.txt"
#endif

/* Android-specific features */
#ifndef ASCIIGRAPH
#define ASCIIGRAPH              /* ASCII dungeon characters */
#endif
#ifndef SELECTSAVED
#define SELECTSAVED             /* saved game selection UI */
#endif
#ifndef USER_SOUNDS
#define USER_SOUNDS             /* user-configurable sounds */
#endif
#ifndef CHANGE_COLOR
#define CHANGE_COLOR            /* runtime color customization */
#endif

/* Register the Android window port */
#ifndef ANDROID_GRAPHICS
#define ANDROID_GRAPHICS
#endif
#undef DEFAULT_WINDOW_SYS
#define DEFAULT_WINDOW_SYS "android"

/* Disable tty/curses graphics — Android provides its own window port */
#undef TTY_GRAPHICS
#undef CURSES_GRAPHICS

#endif /* ANDROIDCONF_H */
#endif /* ANDROID */
