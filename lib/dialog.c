/*****************************************************************************
 * dialog.c: libheimdall dialog API
 *****************************************************************************
 * Copyright © 2016 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <heimdall/libheimdall.h>
#include <heimdall/libheimdall_dialog.h>

#include <heimdall_common.h>
#include <heimdall_dialog.h>

#include "libheimdall_internal.h"

static libheimdall_dialog_question_type
heimdall_to_libheimdall_dialog_question_type(heimdall_dialog_question_type i_type)
{
    switch (i_type)
    {
    case HEIMDALL_DIALOG_QUESTION_NORMAL: return LIBHEIMDALL_DIALOG_QUESTION_NORMAL;
    case HEIMDALL_DIALOG_QUESTION_WARNING: return LIBHEIMDALL_DIALOG_QUESTION_WARNING;
    case HEIMDALL_DIALOG_QUESTION_CRITICAL: return LIBHEIMDALL_DIALOG_QUESTION_CRITICAL;
    default: heimdall_assert_unreachable();
    }
}

static void
display_login_cb(void *p_data, heimdall_dialog_id *p_id, const char *psz_title,
                 const char *psz_text, const char *psz_default_username,
                 bool b_ask_store)
{
    libheimdall_instance_t *p_instance = p_data;

    p_instance->dialog.cbs.pf_display_login(p_instance->dialog.data,
                                            (libheimdall_dialog_id *) p_id,
                                            psz_title, psz_text,
                                            psz_default_username, b_ask_store);
}

static void
display_question_cb(void *p_data, heimdall_dialog_id *p_id, const char *psz_title,
                    const char *psz_text, heimdall_dialog_question_type i_type,
                    const char *psz_cancel, const char *psz_action1,
                    const char *psz_action2)
{
    libheimdall_instance_t *p_instance = p_data;
    const libheimdall_dialog_question_type i_ltype =
        heimdall_to_libheimdall_dialog_question_type(i_type);

    p_instance->dialog.cbs.pf_display_question(p_instance->dialog.data,
                                               (libheimdall_dialog_id *) p_id,
                                               psz_title, psz_text, i_ltype,
                                               psz_cancel,
                                               psz_action1, psz_action2);
}

static void
display_progress_cb(void *p_data, heimdall_dialog_id *p_id, const char *psz_title,
                    const char *psz_text, bool b_indeterminate,
                    float f_position, const char *psz_cancel)
{
    libheimdall_instance_t *p_instance = p_data;

    p_instance->dialog.cbs.pf_display_progress(p_instance->dialog.data,
                                               (libheimdall_dialog_id *) p_id,
                                               psz_title, psz_text,
                                               b_indeterminate, f_position,
                                               psz_cancel);
}

static void
cancel_cb(void *p_data, heimdall_dialog_id *p_id)
{
    libheimdall_instance_t *p_instance = p_data;
    p_instance->dialog.cbs.pf_cancel(p_instance->dialog.data,
                                     (libheimdall_dialog_id *)p_id);
}

static void
update_progress_cb(void *p_data, heimdall_dialog_id *p_id, float f_position,
                   const char *psz_text)
{
    libheimdall_instance_t *p_instance = p_data;
    p_instance->dialog.cbs.pf_update_progress(p_instance->dialog.data,
                                              (libheimdall_dialog_id *) p_id,
                                              f_position, psz_text);
}

void
libheimdall_dialog_set_callbacks(libheimdall_instance_t *p_instance,
                            const libheimdall_dialog_cbs *p_cbs, void *p_data)
{
    libheimdall_int_t *p_libheimdall = p_instance->p_libheimdall_int;

    if (p_cbs != NULL)
    {
        /* No different versions to handle for now */
        assert(p_cbs->version <= 0);

        const heimdall_dialog_cbs dialog_cbs = {
            .pf_display_login = p_cbs->pf_display_login ?
                                display_login_cb : NULL,
            .pf_display_question = p_cbs->pf_display_question != NULL ?
                                   display_question_cb : NULL,
            .pf_display_progress = p_cbs->pf_display_progress != NULL ?
                                   display_progress_cb : NULL,
            .pf_cancel = p_cbs->pf_cancel != NULL ? cancel_cb : NULL,
            .pf_update_progress = p_cbs->pf_update_progress != NULL ?
                                  update_progress_cb : NULL,
        };

        p_instance->dialog.cbs = *p_cbs;
        p_instance->dialog.data = p_data;

        heimdall_dialog_provider_set_callbacks(p_libheimdall, &dialog_cbs, p_instance);
    }
    else
        heimdall_dialog_provider_set_callbacks(p_libheimdall, NULL, NULL);
}

void
libheimdall_dialog_set_error_callback(libheimdall_instance_t *p_instance,
                                 libheimdall_dialog_error_cbs p_cbs, void *p_data)
{
    libheimdall_int_t *p_libheimdall = p_instance->p_libheimdall_int;
    heimdall_dialog_provider_set_error_callback(p_libheimdall, p_cbs, p_data);
}

void
libheimdall_dialog_set_context(libheimdall_dialog_id *p_id, void *p_context)
{
    heimdall_dialog_id_set_context((heimdall_dialog_id *)p_id, p_context);
}

void *
libheimdall_dialog_get_context(libheimdall_dialog_id *p_id)
{
    return heimdall_dialog_id_get_context((heimdall_dialog_id *)p_id);
}

int
libheimdall_dialog_post_login(libheimdall_dialog_id *p_id, const char *psz_username,
                         const char *psz_password, bool b_store)
{
    int i_ret = heimdall_dialog_id_post_login((heimdall_dialog_id *)p_id, psz_username,
                                         psz_password, b_store);
    return i_ret == HEIMDALL_SUCCESS ? 0 : -1;
}

int
libheimdall_dialog_post_action(libheimdall_dialog_id *p_id, int i_action)
{
    int i_ret = heimdall_dialog_id_post_action((heimdall_dialog_id *)p_id, i_action);
    return i_ret == HEIMDALL_SUCCESS ? 0 : -1;
}

int
libheimdall_dialog_dismiss(libheimdall_dialog_id *p_id)
{
    int i_ret = heimdall_dialog_id_dismiss((heimdall_dialog_id *)p_id);
    return i_ret == HEIMDALL_SUCCESS ? 0 : -1;
}
