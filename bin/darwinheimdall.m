/*****************************************************************************
 * darwinheimdall.m: OS X specific main executable for Heimdall media player
 *****************************************************************************
 * Copyright (C) 2013-2015 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne at lycaria dot org>
 *          David Fuhrmann <dfuhrmann at lycaria dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall/heimdall.h>
#include <heimdall_common.h>
#include <heimdall_charset.h>

#include <stdlib.h>
#include <locale.h>
#include <signal.h>
#include <string.h>

#import <CoreFoundation/CoreFoundation.h>
#import <Cocoa/Cocoa.h>

#ifdef HAVE_BREAKPAD
#import <Breakpad/Breakpad.h>
#endif

#include "../lib/libheimdall_internal.h"

struct heimdall_context {
    libheimdall_instance_t *heimdall;
    dispatch_queue_t intf_queue;
    heimdall_sem_t wait_quit;

    bool quitting;
};

/**
 * Handler called when Heimdall asks to terminate the program.
 */
static void heimdall_terminate(void *data)
{
    struct heimdall_context *context = data;

    /* heimdall_terminate can be called multiple times, for instance:
     * - once when an interface like Qt (DialogProvider::quit) is exiting
     * - once when libheimdall_release() is called.
     * `context` can only be guaranteed to be valid for the first call to
     * heimdall_terminate, but others will have no more effects than the first
     * one, so we can ignore them. */
    __block bool quitting = false;
    static dispatch_once_t quitToken = 0;
    dispatch_once(&quitToken, ^{
        quitting = true;
    });

    if (!quitting)
        return;

    /* Release the libheimdall instance to clean up the interfaces. */
    dispatch_async(context->intf_queue, ^{
        libheimdall_release(context->heimdall);
        context->heimdall = NULL;
        heimdall_sem_post(&context->wait_quit);

        dispatch_async(dispatch_get_main_queue(), ^{
            /* Stop the main loop started in main(). */
            CFRunLoopStop(CFRunLoopGetCurrent());
        });
    });
}

#ifdef HAVE_BREAKPAD
BreakpadRef initBreakpad()
{
    BreakpadRef bp = nil;

    /* Create caches directory in case it does not exist */
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *cachePath = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) firstObject];
    NSString *bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleIdentifier"];
    NSString *cacheAppPath = [cachePath stringByAppendingPathComponent:bundleName];
    if (![fileManager fileExistsAtPath:cacheAppPath]) {
        [fileManager createDirectoryAtPath:cacheAppPath withIntermediateDirectories:NO attributes:nil error:nil];
    }

    /* Get Info.plist config */
    NSMutableDictionary *breakpad_config = [[[NSBundle mainBundle] infoDictionary] mutableCopy];

    /* Use in-process reporting */
    [breakpad_config setObject:[NSNumber numberWithBool:YES]
                        forKey:@BREAKPAD_IN_PROCESS];

    /* Set dump location */
    [breakpad_config setObject:cacheAppPath
                        forKey:@BREAKPAD_DUMP_DIRECTORY];

    bp = BreakpadCreate(breakpad_config);
    return bp;
}
#endif

/*****************************************************************************
 * main: parse command line, start interface and spawn threads.
 *****************************************************************************/
int main(int i_argc, const char *ppsz_argv[])
{
#ifdef HAVE_BREAKPAD
    BreakpadRef breakpad = NULL;

    if (!getenv("HEIMDALL_DISABLE_BREAKPAD"))
        breakpad = initBreakpad();
#endif

    /* The so-called POSIX-compliant MacOS X reportedly processes SIGPIPE even
     * if it is blocked in all thread.
     * Note: this is NOT an excuse for not protecting against SIGPIPE. If
     * LibHeimdall runs outside of Heimdall, we cannot rely on this code snippet. */
    signal(SIGPIPE, SIG_IGN);
    /* Restore SIGCHLD in case our parent process ignores it. */
    signal(SIGCHLD, SIG_DFL);

#ifndef NDEBUG
    /* Activate malloc checking routines to detect heap corruptions. */
    setenv("MALLOC_CHECK_", "2", 1);
#endif

#ifdef TOP_BUILDDIR
    setenv("HEIMDALL_PLUGIN_PATH", TOP_BUILDDIR"/modules", 1);
    setenv("HEIMDALL_DATA_PATH", TOP_SRCDIR"/share", 1);
    setenv("HEIMDALL_LIB_PATH", TOP_BUILDDIR"/modules", 1);
    setenv("HEIMDALL_LIBEXEC_PATH", TOP_BUILDDIR"/modules", 1);
#endif

#ifndef ALLOW_RUN_AS_ROOT
    if (geteuid() == 0)
    {
        fprintf(stderr, "Heimdall is not supposed to be run as root. Sorry.\n"
        "If you need to use real-time priorities and/or privileged TCP ports\n"
        "you can use %s-wrapper (make sure it is Set-UID root and\n"
        "cannot be run by non-trusted users first).\n", ppsz_argv[0]);
        return 1;
    }
#endif

    setlocale(LC_ALL, "");

    if (isatty(STDERR_FILENO))
        /* This message clutters error logs. It is printed only on a TTY.
         * Fortunately, LibHeimdall prints version info with -vv anyway. */
        fprintf(stderr, "Heimdall media player %s (revision %s)\n",
                 libheimdall_get_version(), libheimdall_get_changeset());

    sigset_t set;

    sigemptyset(&set);
    /*
     * The darwin version of Heimdall used GCD to dequeue interesting signals.
     * For this to work, those signals must be blocked.
     *
     * There are two advantages over traditional signal handlers:
     *  - handling is done on a separate thread: no need to worry about async-safety,
     *  - EINTR is not generated: other threads need not handle that error.
     * That being said, some LibHeimdall programs do not use sigwait(). Therefore
     * EINTR must still be handled cleanly, notably from poll() calls.
     *
     * Signals that request a clean shutdown.
     * We have to handle SIGTERM cleanly because of daemon mode. */
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);

    /* SIGPIPE can happen and would crash the process. On modern systems,
     * the MSG_NOSIGNAL flag protects socket write operations against SIGPIPE.
     * But we still need to block SIGPIPE when:
     *  - writing to pipes,
     *  - using write() instead of send() for code not specific to sockets.
     * LibHeimdall code assumes that SIGPIPE is blocked. Other LibHeimdall applications
     * shall block it (or handle it somehow) too.
     */
    sigaddset(&set, SIGPIPE);

    /* SIGCHLD must be dequeued to clean up zombie child processes.
     * Furthermore the handler must not be set to SIG_IGN (see above).
     * We cannot pragmatically handle EINTR, short reads and short writes
     * in every code paths (including underlying libraries). So we just
     * block SIGCHLD in all threads, and dequeue it below. */
    sigaddset(&set, SIGCHLD);

    /* Block all these signals */
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    /* Handle signals with GCD */
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_source_t sigIntSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGINT, 0, queue);
    dispatch_source_t sigTermSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, queue);

    if (!sigIntSource || !sigTermSource)
        abort();

    dispatch_resume(sigIntSource);
    dispatch_resume(sigTermSource);

    /* Handle parameters */
    const char **argv = calloc(i_argc + 2, sizeof (argv[0]));
    if (argv == NULL)
        exit(1);

    int argc = 0;

    argv[argc++] = "--no-ignore-config";
    argv[argc++] = "--media-library";

    /* Overwrite system language */
    CFPropertyListRef lang_pref = CFPreferencesCopyAppValue(CFSTR("language"),
        kCFPreferencesCurrentApplication);

    if (lang_pref) {
        if (CFGetTypeID(lang_pref) == CFStringGetTypeID()) {
            char *lang = FromCFString(lang_pref, kCFStringEncodingUTF8);
            if (strncmp(lang, "auto", 4)) {
                char tmp[11];
                snprintf(tmp, 11, "LANG=%s", lang);
                putenv(tmp);
            }
            free(lang);
        }
        CFRelease(lang_pref);
    }

    ppsz_argv++; i_argc--; /* skip executable path */

    /* When Heimdall.app is run by double clicking in Mac OS X < 10.9, the 2nd arg
     * is the PSN - process serial number (a unique PID-ish thingie)
     * still ok for real Darwin & when run from command line
     * for example -psn_0_9306113 */
    if (i_argc >= 1 && !strncmp(*ppsz_argv, "-psn" , 4))
        ppsz_argv++, i_argc--;

    memcpy (argv + argc, ppsz_argv, i_argc * sizeof(*argv));
    argc += i_argc;
    argv[argc] = NULL;

    dispatch_queue_t intf_queue = dispatch_queue_create("com.lycaria.heimdall", NULL);

    __block struct heimdall_context context = {
        .heimdall = NULL,
        .intf_queue = intf_queue,
    };
    heimdall_sem_init(&context.wait_quit, 0);

    dispatch_source_set_event_handler(sigIntSource, ^{
        heimdall_terminate(&context);
    });
    dispatch_source_set_event_handler(sigTermSource, ^{
        heimdall_terminate(&context);
    });

    __block bool intf_started = true;
    __block libheimdall_instance_t *heimdall = NULL;
    int ret = 1;
    dispatch_async(intf_queue, ^{
        /* Initialize libheimdall */
        heimdall = context.heimdall
            = libheimdall_new(argc, argv);
        if (heimdall == NULL)
        {
            dispatch_sync(dispatch_get_main_queue(), ^{
                intf_started = false;
                CFRunLoopStop(CFRunLoopGetMain());
            });
            return;
        }

        libheimdall_SetExitHandler(heimdall->p_libheimdall_int, heimdall_terminate, &context);
        libheimdall_set_app_id(heimdall, "com.lycaria.heimdall", PACKAGE_VERSION, PACKAGE_NAME);
        libheimdall_set_user_agent(heimdall, "Heimdall media player", "Heimdall/"PACKAGE_VERSION);


        if (libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, NULL)) {
            fprintf(stderr, "Heimdall cannot start any interface. Exiting.\n");
            libheimdall_SetExitHandler(heimdall->p_libheimdall_int, NULL, NULL);
            dispatch_sync(dispatch_get_main_queue(), ^{
                intf_started = false;
                CFRunLoopStop(CFRunLoopGetMain());
            });
            return;
        }

        libheimdall_InternalPlay(heimdall->p_libheimdall_int);
    });

    /*
     * Run the main loop. If the mac interface is not initialized, only the CoreFoundation
     * runloop is used. Otherwise, [NSApp run] needs to be called, which setups more stuff
     * before actually starting the loop.
     */
    @autoreleasepool {
        CFRunLoopRun();
    }

    if (!intf_started)
        goto out;

    ret = 0;
    /* Cleanup */
    heimdall_sem_wait(&context.wait_quit);

out:

    dispatch_release(sigIntSource);
    dispatch_release(sigTermSource);

    dispatch_release(intf_queue);
    free(argv);

    /* If no interface were created, we release libheimdall here instead. */
    if (context.heimdall)
        libheimdall_release(context.heimdall);

#ifdef HAVE_BREAKPAD
    if (breakpad)
        BreakpadRelease(breakpad);
#endif

    return ret;
}
