/*****************************************************************************
 * xdg-screensaver.c
 *****************************************************************************
 * Copyright (C) 2008 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_plugin.h>
#include <heimdall_inhibit.h>
#include <heimdall_spawn.h>

static int Open (heimdall_object_t *);
static void Close (heimdall_object_t *);

heimdall_module_begin ()
    set_shortname (N_("XDG-screensaver") )
    set_description (N_("XDG screen saver inhibition") )
    set_capability ("inhibit", 10 )
    set_callbacks (Open, Close)
    set_subcategory (SUBCAT_ADVANCED_MISC)
heimdall_module_end ()

static void Timer (void *data)
{
    static const char *const argv[] = { "xdg-screensaver", "reset", NULL };
    static const int fdv[] = { -1, STDERR_FILENO, STDERR_FILENO, -1 };
    heimdall_inhibit_t *ih = data;
    pid_t pid;

    int err = heimdall_spawnp(&pid, argv[0], fdv, argv);

    if (err == 0)
        heimdall_waitpid(pid);
    else
        msg_Warn (ih, "error starting xdg-screensaver: %s",
                  heimdall_strerror_c(err));
}

static void Inhibit (heimdall_inhibit_t *ih, unsigned mask)
{
    heimdall_timer_t timer = (void *)ih->p_sys;
    bool suspend = (mask & HEIMDALL_INHIBIT_DISPLAY) != 0;
    heimdall_tick_t delay = suspend ? HEIMDALL_TICK_FROM_SEC(30): HEIMDALL_TIMER_DISARM;

    heimdall_timer_schedule(timer, false, delay, delay);
}

static int Open (heimdall_object_t *obj)
{
    heimdall_inhibit_t *ih = (heimdall_inhibit_t *)obj;
    heimdall_timer_t timer;

    if (heimdall_timer_create(&timer, Timer, ih))
        return HEIMDALL_ENOMEM;

    ih->p_sys = (void *)timer;
    ih->inhibit = Inhibit;
    return HEIMDALL_SUCCESS;
}

static void Close (heimdall_object_t *obj)
{
    heimdall_inhibit_t *ih = (heimdall_inhibit_t *)obj;
    heimdall_timer_t timer = (void *)ih->p_sys;

    heimdall_timer_destroy(timer);
}
