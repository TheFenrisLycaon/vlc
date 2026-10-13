/*****************************************************************************
 * dialog.c: User dialog functions
 *****************************************************************************
 * Copyright © 2009 Rémi Denis-Courmont
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

/** @ingroup heimdall_dialog */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>

#include <heimdall_common.h>
#include <heimdall_arrays.h>
#include <heimdall_dialog.h>
#include <heimdall_interrupt.h>
#include <heimdall_extensions.h>
#include <assert.h>
#include "../libheimdall.h"

struct heimdall_dialog_provider
{
    heimdall_mutex_t                 lock;
    heimdall_array_t                 dialog_array;
    heimdall_dialog_cbs              cbs;
    void *                      p_cbs_data;

    heimdall_dialog_error_cbs        err_cbs;
    void *                      p_err_cbs_data;

    heimdall_dialog_ext_update_cb    pf_ext_update;
    void *                      p_ext_data;
};

enum dialog_type
{
    HEIMDALL_DIALOG_ERROR,
    HEIMDALL_DIALOG_LOGIN,
    HEIMDALL_DIALOG_QUESTION,
    HEIMDALL_DIALOG_PROGRESS,
};

struct dialog_answer
{
    enum dialog_type i_type;
    union
    {
        struct
        {
            char *psz_username;
            char *psz_password;
            bool b_store;
        } login;
        struct
        {
            int i_action;
        } question;
    } u;
};

struct dialog
{
    enum dialog_type i_type;
    const char *psz_title;
    const char *psz_text;

    union
    {
        struct
        {
            const char *psz_default_username;
            bool b_ask_store;
        } login;
        struct
        {
            heimdall_dialog_question_type i_type;
            const char *psz_cancel;
            const char *psz_action1;
            const char *psz_action2;
        } question;
        struct
        {
            bool b_indeterminate;
            float f_position;
            const char *psz_cancel;
        } progress;
    } u;
};

struct heimdall_dialog_id
{
    heimdall_mutex_t             lock;
    heimdall_cond_t              wait;
    enum dialog_type        i_type;
    void *                  p_context;
    int                     i_refcount;
    bool                    b_cancelled;
    bool                    b_answered;
    bool                    b_progress_indeterminate;
    char *                  psz_progress_text;
    struct dialog_answer    answer;
};

struct dialog_i11e_context
{
    heimdall_dialog_provider *   p_provider;
    heimdall_dialog_id *         p_id;
};

static inline heimdall_dialog_provider *
get_dialog_provider(heimdall_object_t *p_obj, bool b_check_interact)
{
    if ((b_check_interact && p_obj->no_interact) || heimdall_killed())
        return NULL;

    heimdall_dialog_provider *p_provider =
        libheimdall_priv(heimdall_object_instance(p_obj))->p_dialog_provider;
    assert(p_provider != NULL);
    return p_provider;
}

static void
dialog_id_release(heimdall_dialog_id *p_id)
{
    if (p_id->answer.i_type == HEIMDALL_DIALOG_LOGIN)
    {
        free(p_id->answer.u.login.psz_username);
        free(p_id->answer.u.login.psz_password);
    }
    free(p_id->psz_progress_text);
    free(p_id);
}

int
libheimdall_InternalDialogInit(libheimdall_int_t *p_libheimdall)
{
    assert(p_libheimdall != NULL);
    heimdall_dialog_provider *p_provider = malloc(sizeof(*p_provider));
    if (p_provider == NULL)
        return HEIMDALL_EGENERIC;

    heimdall_mutex_init(&p_provider->lock);
    heimdall_array_init(&p_provider->dialog_array);

    memset(&p_provider->cbs, 0, sizeof(p_provider->cbs));
    p_provider->p_cbs_data = NULL;

    p_provider->err_cbs = NULL;
    p_provider->p_err_cbs_data = NULL;

    p_provider->pf_ext_update = NULL;
    p_provider->p_ext_data = NULL;
    libheimdall_priv(p_libheimdall)->p_dialog_provider = p_provider;

    return HEIMDALL_SUCCESS;
}

static void
dialog_cancel_locked(heimdall_dialog_provider *p_provider, heimdall_dialog_id *p_id)
{
    heimdall_mutex_lock(&p_id->lock);
    if (p_id->b_cancelled || p_id->b_answered)
    {
        heimdall_mutex_unlock(&p_id->lock);
        return;
    }
    p_id->b_cancelled = true;
    heimdall_mutex_unlock(&p_id->lock);

    p_provider->cbs.pf_cancel(p_provider->p_cbs_data, p_id);
}

static heimdall_dialog_id *
dialog_add_locked(heimdall_dialog_provider *p_provider, enum dialog_type i_type)
{
    heimdall_dialog_id *p_id = calloc(1, sizeof(*p_id));

    if (p_id == NULL)
        return NULL;

    if(heimdall_array_append(&p_provider->dialog_array, p_id))
    {
        free(p_id);
        return NULL;
    }

    heimdall_mutex_init(&p_id->lock);
    heimdall_cond_init(&p_id->wait);

    p_id->i_type = i_type;
    p_id->i_refcount = 2; /* provider and callbacks */

    return p_id;
}

static void
dialog_remove_locked(heimdall_dialog_provider *p_provider, heimdall_dialog_id *p_id)
{
    ssize_t i_idx = heimdall_array_index_of_item(&p_provider->dialog_array, p_id);
    assert(i_idx >= 0);
    heimdall_array_remove(&p_provider->dialog_array, i_idx);

    heimdall_mutex_lock(&p_id->lock);
    p_id->i_refcount--;
    if (p_id->i_refcount == 0)
    {
        heimdall_mutex_unlock(&p_id->lock);
        dialog_id_release(p_id);
    }
    else
        heimdall_mutex_unlock(&p_id->lock);
}

static void
dialog_clear_all_locked(heimdall_dialog_provider *p_provider)
{
    for (size_t i = 0; i < heimdall_array_count(&p_provider->dialog_array); ++i)
    {
        heimdall_dialog_id *p_id =
            heimdall_array_item_at_index(&p_provider->dialog_array, i);
        dialog_cancel_locked(p_provider, p_id);
    }
}

void
libheimdall_InternalDialogClean(libheimdall_int_t *p_libheimdall)
{
    assert(p_libheimdall != NULL);
    heimdall_dialog_provider *p_provider = libheimdall_priv(p_libheimdall)->p_dialog_provider;

    if (p_provider == NULL)
        return;
    heimdall_mutex_lock(&p_provider->lock);
    dialog_clear_all_locked(p_provider);
    heimdall_mutex_unlock(&p_provider->lock);

    free(p_provider);
    libheimdall_priv(p_libheimdall)->p_dialog_provider = NULL;
}

#undef heimdall_dialog_provider_set_callbacks
void
heimdall_dialog_provider_set_callbacks(heimdall_object_t *p_obj,
                                  const heimdall_dialog_cbs *p_cbs, void *p_data)
{
    assert(p_obj != NULL);
    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, false);

    heimdall_mutex_lock(&p_provider->lock);
    dialog_clear_all_locked(p_provider);

    if (p_cbs == NULL)
    {
        memset(&p_provider->cbs, 0, sizeof(p_provider->cbs));
        p_provider->p_cbs_data = NULL;
    }
    else
    {
        p_provider->cbs = *p_cbs;
        p_provider->p_cbs_data = p_data;
    }
    heimdall_mutex_unlock(&p_provider->lock);
}

#undef heimdall_dialog_provider_set_error_callback
void
heimdall_dialog_provider_set_error_callback(heimdall_object_t *p_obj,
                                  heimdall_dialog_error_cbs p_cbs, void *p_data)
{
    assert(p_obj != NULL);
    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, false);

    heimdall_mutex_lock(&p_provider->lock);

    if (p_cbs == NULL)
    {
        p_provider->err_cbs  = NULL;
        p_provider->p_err_cbs_data = NULL;
    }
    else
    {
        p_provider->err_cbs = p_cbs;
        p_provider->p_err_cbs_data = p_data;
    }
    heimdall_mutex_unlock(&p_provider->lock);
}

static void
dialog_wait_interrupted(void *p_data)
{
    struct dialog_i11e_context *p_context = p_data;
    heimdall_dialog_provider *p_provider = p_context->p_provider;
    heimdall_dialog_id *p_id = p_context->p_id;

    heimdall_mutex_lock(&p_provider->lock);
    dialog_cancel_locked(p_provider, p_id);
    heimdall_mutex_unlock(&p_provider->lock);

    heimdall_mutex_lock(&p_id->lock);
    heimdall_cond_signal(&p_id->wait);
    heimdall_mutex_unlock(&p_id->lock);
}

static int
dialog_wait(heimdall_dialog_provider *p_provider, heimdall_dialog_id *p_id,
            enum dialog_type i_type, struct dialog_answer *p_answer)
{
    struct dialog_i11e_context context = {
        .p_provider = p_provider,
        .p_id = p_id,
    };
    heimdall_interrupt_register(dialog_wait_interrupted, &context);

    heimdall_mutex_lock(&p_id->lock);
    /* Wait for the dialog to be dismissed, interrupted or answered */
    while (!p_id->b_cancelled && !p_id->b_answered)
        heimdall_cond_wait(&p_id->wait, &p_id->lock);

    int i_ret;
    if (p_id->b_cancelled)
        i_ret = 0;
    else if (p_id->answer.i_type != i_type)
        i_ret = HEIMDALL_EGENERIC;
    else
    {
        i_ret = 1;
        memcpy(p_answer, &p_id->answer, sizeof(p_id->answer));
        memset(&p_id->answer, 0, sizeof(p_id->answer));
    }

    heimdall_mutex_unlock(&p_id->lock);
    heimdall_interrupt_unregister();

    heimdall_mutex_lock(&p_provider->lock);
    dialog_remove_locked(p_provider, p_id);
    heimdall_mutex_unlock(&p_provider->lock);
    return i_ret;
}

static int
dialog_display_error_va(heimdall_dialog_provider *p_provider, const char *psz_title,
                        const char *psz_fmt, va_list ap)
{
    heimdall_mutex_lock(&p_provider->lock);
    if (p_provider->err_cbs == NULL)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_EGENERIC;
    }

    char *psz_text;
    if (vasprintf(&psz_text, psz_fmt, ap) == -1)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_ENOMEM;
    }

    p_provider->err_cbs(p_provider->p_err_cbs_data, psz_title, psz_text);
    free(psz_text);
    heimdall_mutex_unlock(&p_provider->lock);

    return HEIMDALL_SUCCESS;
}

int
heimdall_dialog_display_error_va(heimdall_object_t *p_obj, const char *psz_title,
                            const char *psz_fmt, va_list ap)
{
    assert(p_obj != NULL && psz_title != NULL && psz_fmt != NULL);
    int i_ret;
    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, true);

    if (p_provider != NULL)
        i_ret = dialog_display_error_va(p_provider, psz_title, psz_fmt, ap);
    else
        i_ret = HEIMDALL_EGENERIC;

    if (i_ret != HEIMDALL_SUCCESS)
    {
        msg_Err(p_obj, "%s", psz_title);
        msg_GenericVa(p_obj, HEIMDALL_MSG_ERR, psz_fmt, ap);
    }
    return i_ret;
}


#undef heimdall_dialog_display_error
int
heimdall_dialog_display_error(heimdall_object_t *p_obj, const char *psz_title,
                         const char *psz_fmt, ...)
{
    assert(psz_fmt != NULL);
    va_list ap;
    va_start(ap, psz_fmt);
    int i_ret = heimdall_dialog_display_error_va(p_obj, psz_title, psz_fmt, ap);
    va_end(ap);
    return i_ret;
}

static int
dialog_display_login_va(heimdall_dialog_provider *p_provider, heimdall_dialog_id **pp_id,
                        const char *psz_default_username, bool b_ask_store,
                        const char *psz_title, const char *psz_fmt, va_list ap)
{
    heimdall_mutex_lock(&p_provider->lock);
    if (p_provider->cbs.pf_display_login == NULL
     || p_provider->cbs.pf_cancel == NULL)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_EGENERIC;
    }

    char *psz_text;
    if (vasprintf(&psz_text, psz_fmt, ap) == -1)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_ENOMEM;
    }

    heimdall_dialog_id *p_id = dialog_add_locked(p_provider, HEIMDALL_DIALOG_LOGIN);
    if (p_id == NULL)
    {
        free(psz_text);
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_ENOMEM;
    }
    p_provider->cbs.pf_display_login(p_provider->p_cbs_data, p_id, psz_title,
                                     psz_text, psz_default_username, b_ask_store);
    free(psz_text);
    heimdall_mutex_unlock(&p_provider->lock);
    *pp_id = p_id;

    return HEIMDALL_SUCCESS;
}

int
heimdall_dialog_wait_login_va(heimdall_object_t *p_obj,  char **ppsz_username,
                         char **ppsz_password, bool *p_store,
                         const char *psz_default_username,
                         const char *psz_title, const char *psz_fmt, va_list ap)
{
    assert(p_obj != NULL && ppsz_username != NULL && ppsz_password != NULL
        && psz_fmt != NULL && psz_title != NULL);

    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, true);
    if (p_provider == NULL)
        return HEIMDALL_EGENERIC;

    heimdall_dialog_id *p_id;
    int i_ret = dialog_display_login_va(p_provider, &p_id, psz_default_username,
                                        p_store != NULL, psz_title, psz_fmt, ap);
    if (i_ret < 0 || p_id == NULL)
        return i_ret;

    struct dialog_answer answer;
    i_ret = dialog_wait(p_provider, p_id, HEIMDALL_DIALOG_LOGIN, &answer);
    if (i_ret <= 0)
        return i_ret;

    *ppsz_username = answer.u.login.psz_username;
    *ppsz_password = answer.u.login.psz_password;
    if (p_store != NULL)
        *p_store = answer.u.login.b_store;

    return 1;
}

#undef heimdall_dialog_wait_login
int
heimdall_dialog_wait_login(heimdall_object_t *p_obj,  char **ppsz_username,
                      char **ppsz_password, bool *p_store,
                      const char *psz_default_username, const char *psz_title,
                      const char *psz_fmt, ...)
{
    assert(psz_fmt != NULL);
    va_list ap;
    va_start(ap, psz_fmt);
    int i_ret = heimdall_dialog_wait_login_va(p_obj, ppsz_username, ppsz_password,
                                         p_store,psz_default_username,
                                         psz_title, psz_fmt, ap);
    va_end(ap);
    return i_ret;
}

static int
dialog_display_question_va(heimdall_dialog_provider *p_provider, heimdall_dialog_id **pp_id,
                           heimdall_dialog_question_type i_type,
                           const char *psz_cancel, const char *psz_action1,
                           const char *psz_action2, const char *psz_title,
                           const char *psz_fmt, va_list ap)
{
    heimdall_mutex_lock(&p_provider->lock);
    if (p_provider->cbs.pf_display_question == NULL
     || p_provider->cbs.pf_cancel == NULL)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_EGENERIC;
    }

    char *psz_text;
    if (vasprintf(&psz_text, psz_fmt, ap) == -1)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_ENOMEM;
    }

    heimdall_dialog_id *p_id = dialog_add_locked(p_provider, HEIMDALL_DIALOG_QUESTION);
    if (p_id == NULL)
    {
        free(psz_text);
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_ENOMEM;
    }
    p_provider->cbs.pf_display_question(p_provider->p_cbs_data, p_id, psz_title,
                                        psz_text, i_type, psz_cancel, psz_action1,
                                        psz_action2);
    free(psz_text);
    heimdall_mutex_unlock(&p_provider->lock);
    *pp_id = p_id;

    return HEIMDALL_SUCCESS;
}

int
heimdall_dialog_wait_question_va(heimdall_object_t *p_obj,
                            heimdall_dialog_question_type i_type,
                            const char *psz_cancel, const char *psz_action1,
                            const char *psz_action2, const char *psz_title,
                            const char *psz_fmt, va_list ap)
{
    assert(p_obj != NULL && psz_fmt != NULL && psz_title != NULL
        && psz_cancel != NULL);

    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, true);
    if (p_provider == NULL)
        return HEIMDALL_EGENERIC;

    heimdall_dialog_id *p_id;
    int i_ret = dialog_display_question_va(p_provider, &p_id, i_type,
                                           psz_cancel, psz_action1,
                                           psz_action2, psz_title, psz_fmt, ap);
    if (i_ret < 0 || p_id == NULL)
        return i_ret;

    struct dialog_answer answer;
    i_ret = dialog_wait(p_provider, p_id, HEIMDALL_DIALOG_QUESTION, &answer);
    if (i_ret <= 0)
        return i_ret;

    if (answer.u.question.i_action != 1 && answer.u.question.i_action != 2)
        return HEIMDALL_EGENERIC;

    return answer.u.question.i_action;
}

#undef heimdall_dialog_wait_question
int
heimdall_dialog_wait_question(heimdall_object_t *p_obj,
                         heimdall_dialog_question_type i_type,
                         const char *psz_cancel, const char *psz_action1,
                         const char *psz_action2, const char *psz_title,
                         const char *psz_fmt, ...)
{
    assert(psz_fmt != NULL);
    va_list ap;
    va_start(ap, psz_fmt);
    int i_ret = heimdall_dialog_wait_question_va(p_obj, i_type, psz_cancel,
                                            psz_action1, psz_action2, psz_title,
                                            psz_fmt, ap);
    va_end(ap);
    return i_ret;
}

static int
display_progress_va(heimdall_dialog_provider *p_provider, heimdall_dialog_id **pp_id,
                    bool b_indeterminate, float f_position,
                    const char *psz_cancel, const char *psz_title,
                    const char *psz_fmt, va_list ap)
{
    heimdall_mutex_lock(&p_provider->lock);
    if (p_provider->cbs.pf_display_progress == NULL
     || p_provider->cbs.pf_update_progress == NULL
     || p_provider->cbs.pf_cancel == NULL)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_EGENERIC;
    }

    char *psz_text;
    if (vasprintf(&psz_text, psz_fmt, ap) == -1)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_ENOMEM;
    }

    heimdall_dialog_id *p_id = dialog_add_locked(p_provider, HEIMDALL_DIALOG_PROGRESS);
    if (p_id == NULL)
    {
        free(psz_text);
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_ENOMEM;
    }
    p_id->b_progress_indeterminate = b_indeterminate;
    p_id->psz_progress_text = psz_text;
    p_provider->cbs.pf_display_progress(p_provider->p_cbs_data, p_id, psz_title,
                                        psz_text, b_indeterminate, f_position,
                                        psz_cancel);
    heimdall_mutex_unlock(&p_provider->lock);
    *pp_id = p_id;

    return HEIMDALL_SUCCESS;
}

heimdall_dialog_id *
heimdall_dialog_display_progress_va(heimdall_object_t *p_obj, bool b_indeterminate,
                               float f_position, const char *psz_cancel,
                               const char *psz_title, const char *psz_fmt,
                               va_list ap)
{
    assert(p_obj != NULL && psz_title != NULL && psz_fmt != NULL);

    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, true);
    if (p_provider == NULL)
        return NULL;
    heimdall_dialog_id *p_id;
    int i_ret = display_progress_va(p_provider, &p_id, b_indeterminate,
                                    f_position, psz_cancel, psz_title, psz_fmt,
                                    ap);
    return i_ret == HEIMDALL_SUCCESS ? p_id : NULL;
}

#undef heimdall_dialog_display_progress
heimdall_dialog_id *
heimdall_dialog_display_progress(heimdall_object_t *p_obj, bool b_indeterminate,
                            float f_position, const char *psz_cancel,
                            const char *psz_title, const char *psz_fmt, ...)
{
    assert(psz_fmt != NULL);
    va_list ap;
    va_start(ap, psz_fmt);
    heimdall_dialog_id *p_id =
        heimdall_dialog_display_progress_va(p_obj, b_indeterminate, f_position,
                                       psz_cancel, psz_title, psz_fmt, ap);
    va_end(ap);
    return p_id;
}

static int
dialog_update_progress(heimdall_object_t *p_obj, heimdall_dialog_id *p_id, float f_value,
                       char *psz_text)
{
    assert(p_obj != NULL && p_id != NULL);
    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, false);

    heimdall_mutex_lock(&p_provider->lock);
    if (p_provider->cbs.pf_update_progress == NULL ||
        heimdall_dialog_is_cancelled(p_obj, p_id))
    {
        heimdall_mutex_unlock(&p_provider->lock);
        free(psz_text);
        return HEIMDALL_EGENERIC;
    }

    if (p_id->b_progress_indeterminate)
        f_value = 0.0f;

    if (psz_text != NULL)
    {
        free(p_id->psz_progress_text);
        p_id->psz_progress_text = psz_text;
    }
    p_provider->cbs.pf_update_progress(p_provider->p_cbs_data, p_id, f_value,
                                       p_id->psz_progress_text);

    heimdall_mutex_unlock(&p_provider->lock);
    return HEIMDALL_SUCCESS;
}

#undef heimdall_dialog_update_progress
int
heimdall_dialog_update_progress(heimdall_object_t *p_obj, heimdall_dialog_id *p_id,
                           float f_value)
{
    return dialog_update_progress(p_obj, p_id, f_value, NULL);
}

int
heimdall_dialog_update_progress_text_va(heimdall_object_t *p_obj, heimdall_dialog_id *p_id,
                                   float f_value, const char *psz_fmt,
                                   va_list ap)
{
    assert(psz_fmt != NULL);

    char *psz_text;
    if (vasprintf(&psz_text, psz_fmt, ap) == -1)
        return HEIMDALL_ENOMEM;
    return dialog_update_progress(p_obj, p_id, f_value, psz_text);
}

#undef heimdall_dialog_update_progress_text
int
heimdall_dialog_update_progress_text(heimdall_object_t *p_obj, heimdall_dialog_id *p_id,
                                float f_value, const char *psz_fmt, ...)
{
    assert(psz_fmt != NULL);
    va_list ap;
    va_start(ap, psz_fmt);
    int i_ret = heimdall_dialog_update_progress_text_va(p_obj, p_id, f_value,
                                                   psz_fmt, ap);
    va_end(ap);
    return i_ret;
}

#undef heimdall_dialog_release
void
heimdall_dialog_release(heimdall_object_t *p_obj, heimdall_dialog_id *p_id)
{
    assert(p_obj != NULL && p_id != NULL);
    /* We can't use get_dialog_provider here because when heimdall_killed is true,
     * get_dialog_provider returns NULL, and we can't release the dialog
     * properly. Moreover, since the dialog has been created by the provider,
     * we can be sure that one exists. */
    heimdall_dialog_provider *p_provider =
        libheimdall_priv(heimdall_object_instance(p_obj))->p_dialog_provider;
    assert(p_provider != NULL);

    heimdall_mutex_lock(&p_provider->lock);
    dialog_cancel_locked(p_provider, p_id);
    dialog_remove_locked(p_provider, p_id);
    heimdall_mutex_unlock(&p_provider->lock);
}

#undef heimdall_dialog_is_cancelled
bool
heimdall_dialog_is_cancelled(heimdall_object_t *p_obj, heimdall_dialog_id *p_id)
{
    (void) p_obj;
    assert(p_id != NULL);

    heimdall_mutex_lock(&p_id->lock);
    bool b_cancelled = p_id->b_cancelled;
    heimdall_mutex_unlock(&p_id->lock);
    return b_cancelled;
}

void
heimdall_dialog_id_set_context(heimdall_dialog_id *p_id, void *p_context)
{
    heimdall_mutex_lock(&p_id->lock);
    p_id->p_context = p_context;
    heimdall_mutex_unlock(&p_id->lock);
}

void *
heimdall_dialog_id_get_context(heimdall_dialog_id *p_id)
{
    assert(p_id != NULL);
    heimdall_mutex_lock(&p_id->lock);
    void *p_context = p_id->p_context;
    heimdall_mutex_unlock(&p_id->lock);
    return p_context;
}

static int
dialog_id_post(heimdall_dialog_id *p_id, struct dialog_answer *p_answer)
{
    heimdall_mutex_lock(&p_id->lock);
    if (p_answer == NULL)
    {
        p_id->b_cancelled = true;
    }
    else
    {
        p_id->answer = *p_answer;
        p_id->b_answered = true;
    }
    p_id->i_refcount--;
    if (p_id->i_refcount > 0)
    {
        heimdall_cond_signal(&p_id->wait);
        heimdall_mutex_unlock(&p_id->lock);
    }
    else
    {
        heimdall_mutex_unlock(&p_id->lock);
        dialog_id_release(p_id);
    }
    return HEIMDALL_SUCCESS;
}

int
heimdall_dialog_id_post_login(heimdall_dialog_id *p_id, const char *psz_username,
                         const char *psz_password, bool b_store)
{
    assert(p_id != NULL && psz_username != NULL && psz_password != NULL);

    struct dialog_answer answer = {
        .i_type = HEIMDALL_DIALOG_LOGIN,
        .u.login = {
            .b_store = b_store,
            .psz_username = strdup(psz_username),
            .psz_password = strdup(psz_password),
        },
    };
    if (answer.u.login.psz_username == NULL
     || answer.u.login.psz_password == NULL)
    {
        free(answer.u.login.psz_username);
        free(answer.u.login.psz_password);
        dialog_id_post(p_id, NULL);
        return HEIMDALL_ENOMEM;
    }

    return dialog_id_post(p_id, &answer);
}

int
heimdall_dialog_id_post_action(heimdall_dialog_id *p_id, int i_action)
{
    assert(p_id != NULL);

    struct dialog_answer answer = {
        .i_type = HEIMDALL_DIALOG_QUESTION,
        .u.question = { .i_action = i_action },
    };

    return dialog_id_post(p_id, &answer);
}

int
heimdall_dialog_id_dismiss(heimdall_dialog_id *p_id)
{
    return dialog_id_post(p_id, NULL);
}

#undef heimdall_dialog_provider_set_ext_callback
void
heimdall_dialog_provider_set_ext_callback(heimdall_object_t *p_obj,
                                     heimdall_dialog_ext_update_cb pf_update,
                                     void *p_data)
{
    assert(p_obj != NULL);
    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, false);

    heimdall_mutex_lock(&p_provider->lock);

    p_provider->pf_ext_update = pf_update;
    p_provider->p_ext_data = p_data;

    heimdall_mutex_unlock(&p_provider->lock);
}

#undef heimdall_ext_dialog_update
int
heimdall_ext_dialog_update(heimdall_object_t *p_obj, extension_dialog_t *p_ext_dialog)
{
    assert(p_obj != NULL);
    heimdall_dialog_provider *p_provider = get_dialog_provider(p_obj, false);

    heimdall_mutex_lock(&p_provider->lock);
    if (p_provider->pf_ext_update == NULL)
    {
        heimdall_mutex_unlock(&p_provider->lock);
        return HEIMDALL_EGENERIC;
    }
    p_provider->pf_ext_update(p_ext_dialog, p_provider->p_ext_data);
    heimdall_mutex_unlock(&p_provider->lock);
    return HEIMDALL_SUCCESS;
}
