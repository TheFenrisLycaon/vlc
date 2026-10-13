/* licence WTFPL */
/* Test driver for checking vout window behaviour */
/* Copyright © 2021 Alexandre Janniaux <ajanni@videolabs.io> */
/* Copyright (C) 2026 TheFenrisLycaon

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>

#include <heimdall/heimdall.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <heimdall_common.h>
#include <heimdall_window.h>

#include "../lib/libheimdall_internal.h"

static char *window_name = NULL;

int verbosity = 0;

static enum {
    OPEN_CLOSE,
    LIST_OUTPUT,
} current_mode = OPEN_CLOSE;

static void usage(const char *name, int ret)
{
    fprintf(stderr, "Usage: %s [-w window_name] -l\n", name);
    exit(ret);
}

/* extracts options from command line */
static void cmdline(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "hlvw:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage(argv[0], 0);
                break;

            case 'l':
                current_mode = LIST_OUTPUT;
                break;

            case 'v':
                verbosity++;
                if (verbosity > 2)
                    verbosity = 2;
                break;

            case 'w':
                window_name = strdup(optarg);
                break;

            default:
                usage(argv[0], 1);
                break;
        }
    }
}

static libheimdall_instance_t *create_libheimdall(void)
{
    char verbose_flag[2] = "0";
    verbose_flag[0] = '0' + verbosity;
    const char* const args[] = {
        "--verbose", verbose_flag,
    };

    return libheimdall_new(sizeof args / sizeof *args, args);
}

static void ReportOutput(
        struct heimdall_window *wnd,
        const char *id,
        const char *desc)
{
    (void)wnd;
    if (desc)
        printf(" - output added %s: %s\n", id, desc);
    else
        printf(" - output removed %s\n", id);
}

static void ReportResized(
        struct heimdall_window *wnd,
        unsigned width, unsigned height,
        heimdall_window_ack_cb ack_cb, void *opaque)
{
    if (ack_cb)
        ack_cb(wnd, width, height, opaque);
    printf(" - window resized to %ux%u\n", width, height);
}

int main(int argc, char *argv[])
{
#ifdef TOP_BUILDDIR
    setenv ("HEIMDALL_PLUGIN_PATH", TOP_BUILDDIR"/modules", 1);
    setenv ("HEIMDALL_DATA_PATH", TOP_SRCDIR"/share", 1);
    setenv ("HEIMDALL_LIB_PATH", TOP_BUILDDIR"/modules", 1);
#endif

    /* mandatory to support UTF-8 filenames (provided the locale is well set)*/
    setlocale(LC_ALL, "");

    cmdline(argc, argv);

    /* starts heimdall */
    libheimdall_instance_t *libheimdall = create_libheimdall();
    assert(libheimdall);

    heimdall_object_t *root = &libheimdall->p_libheimdall_int->obj;
    const struct heimdall_window_callbacks list_cbs = {
        .output_event = ReportOutput,
        .resized = ReportResized,
    };

    const struct heimdall_window_callbacks win_cbs = {
        .resized = ReportResized,
    };

    const heimdall_window_owner_t owner = {
        .sys = NULL,
        .cbs = (current_mode == LIST_OUTPUT) ? &list_cbs : &win_cbs,
    };

    const struct heimdall_window_cfg cfg = {
        .width = 800, .height = 600,
    };
    heimdall_window_t *wnd = heimdall_window_New(root, window_name, &owner, &cfg);

    int ret = HEIMDALL_SUCCESS;
    if (current_mode == OPEN_CLOSE)
    {
        ret = heimdall_window_Enable(wnd);

        if (ret == HEIMDALL_SUCCESS)
            heimdall_window_Disable(wnd);
    }

    heimdall_window_Delete(wnd);

    libheimdall_release(libheimdall);

    free(window_name);
    return (ret == HEIMDALL_SUCCESS) ? 0 : -1;
}
