/* NetHack 5.0  androidunix.c */
/* Android Unix compatibility layer - based on unixunix.c */

#include "hack.h"

#include <errno.h>
#include <sys/stat.h>
#if defined(NO_FILE_LINKS) || defined(POSIX_TYPES)
#include <fcntl.h>
#endif

/* normalize file name: replace '.', '/', ' ', ':' with '_' */
void
regularize(char *s)
{
    char *lp;

    while ((lp = strchr(s, '.')) || (lp = strchr(s, '/'))
           || (lp = strchr(s, ' ')) || (lp = strchr(s, ':')))
        *lp = '_';
}

boolean
file_exists(const char *path)
{
    struct stat buf;

    if (stat(path, &buf) < 0)
        return FALSE;
    return TRUE;
}

#include <time.h>

void
msleep(unsigned int msec)
{
    struct timespec req;

    msec = (msec == 0) ? 1 : msec; /* time to sleep in milliseconds */
    req.tv_sec = (time_t) (msec / 1000);
    req.tv_nsec = (long) (msec % 1000) * 1000000L;
    (void) nanosleep(&req, (struct timespec *) 0);
}

/* Erase any leftover lock files for this character. Mirrors the
   eraseoldlocks() in sys/unix/unixunix.c. */
static int
eraseoldlocks(void)
{
    int i;

    for (i = 1; i <= MAXDUNGEON * MAXLEVEL + 1; i++) {
        set_levelfile_name(gl.lock, i);
        (void) unlink(fqname(gl.lock, LEVELPREFIX, 0));
    }
    set_levelfile_name(gl.lock, 0);
    if (unlink(fqname(gl.lock, LEVELPREFIX, 0)))
        return 0;
    return 1;
}

/* Acquire the per-character lock and create the level-0 file that NetHack
   uses to mark a game as active. Without this file the engine fails on the
   very first level transition with "Cannot open file <lock>.0". The
   Android app is single-instance, so we don't bother prompting the user
   about pre-existing locks: try self-recovery first, fall back to wiping
   them, then create a fresh level-0 file. Modeled on getlock() in
   sys/unix/unixunix.c with the multi-game / TTY / signal logic stripped. */
void
getlock(void)
{
    int fd;
    const char *fq_lock;

    if (!lock_file(HLOCK, LOCKPREFIX, 10)) {
        wait_synch();
        error("%s", "");
    }

    /* Build <uid><charname> as the lock base. getuid() is stubbed to 1 in
       androidconf.h so this is effectively just the player name. */
    if (!gl.locknum)
        Sprintf(gl.lock, "%u%s", (unsigned) getuid(), svp.plname);

    regularize(gl.lock);
    set_levelfile_name(gl.lock, 0);
    fq_lock = fqname(gl.lock, LEVELPREFIX, 0);

    if ((fd = open(fq_lock, 0)) >= 0) {
        (void) close(fd);
#ifdef SELF_RECOVER
        if (recover_savefile() && program_state.in_self_recover) {
            set_levelfile_name(gl.lock, 0);
            fq_lock = fqname(gl.lock, LEVELPREFIX, 0);
        } else
#endif
        {
            (void) eraseoldlocks();
        }
    }

    fd = creat(fq_lock, FCMASK);
    unlock_file(HLOCK);
    if (fd == -1) {
        error("cannot creat lock file (%s).", fq_lock);
        /*NOTREACHED*/
    }
    if (write(fd, (genericptr_t) &svh.hackpid, sizeof svh.hackpid)
        != sizeof svh.hackpid) {
        (void) close(fd);
        error("cannot write lock (%s)", fq_lock);
        /*NOTREACHED*/
    }
    if (close(fd) == -1) {
        error("cannot close lock (%s)", fq_lock);
        /*NOTREACHED*/
    }
}
