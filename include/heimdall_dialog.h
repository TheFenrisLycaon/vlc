/*****************************************************************************
 * heimdall_dialog.h: user interaction dialogs
 *****************************************************************************
 * Copyright (C) 2009 Rémi Denis-Courmont
 * Copyright (C) 2016 VLC authors and VideoLAN
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

#ifndef HEIMDALL_DIALOG_H_
#define HEIMDALL_DIALOG_H_
# include <stdarg.h>

typedef struct heimdall_dialog_provider heimdall_dialog_provider;
typedef struct heimdall_dialog_id heimdall_dialog_id;
typedef struct extension_dialog_t extension_dialog_t;

/* Called from src/libheimdall.c */
int
libheimdall_InternalDialogInit(libheimdall_int_t *p_libheimdall);

/* Called from src/libheimdall.c */
void
libheimdall_InternalDialogClean(libheimdall_int_t *p_libheimdall);

/**
 * @defgroup heimdall_dialog Heimdall dialog
 * @ingroup interface
 * @{
 * @file
 * This file declares Heimdall dialog functions
 * @defgroup heimdall_dialog_api Heimdall dialog functions
 * In order to interact with the user
 * @{
 */

/**
 * Dialog question type, see heimdall_dialog_wait_question()
 */
typedef enum heimdall_dialog_question_type
{
    HEIMDALL_DIALOG_QUESTION_NORMAL,
    HEIMDALL_DIALOG_QUESTION_WARNING,
    HEIMDALL_DIALOG_QUESTION_CRITICAL,
} heimdall_dialog_question_type;

/**
 * Sends an error message
 *
 * This function returns immediately
 *
 * @param p_obj the Heimdall object emitting the error
 * @param psz_title title of the error dialog
 * @param psz_fmt format string for the error message
 * @return HEIMDALL_SUCCESS on success, or a Heimdall error code on error
 */
HEIMDALL_API int
heimdall_dialog_display_error(heimdall_object_t *p_obj, const char *psz_title,
                         const char *psz_fmt, ...) HEIMDALL_FORMAT(3,4);
#define heimdall_dialog_display_error(a, b, c, ...) \
    heimdall_dialog_display_error(HEIMDALL_OBJECT(a), b, c, ##__VA_ARGS__)

/**
 * Sends an error message
 *
 * Equivalent to heimdall_dialog_display_error() expect that it's called with a
 * va_list.
 */
HEIMDALL_API int
heimdall_dialog_display_error_va(heimdall_object_t *p_obj, const char *psz_title,
                            const char *psz_fmt, va_list ap);

/**
 * Requests an user name and a password
 *
 * This function waits until the user dismisses the dialog or responds. It's
 * interruptible via heimdall_interrupt. In that case, heimdall_dialog_cbs.pf_cancel()
 * will be invoked. If p_store is not NULL, the user will be asked to store the
 * password or not.
 *
 * @param p_obj the Heimdall object emitting the dialog
 * @param ppsz_username a pointer to the user name provided by the user, it
 * must be freed with free() on success
 * @param ppsz_password a pointer to the password provided by the user, it must
 * be freed with free() on success
 * @param p_store a pointer to the store answer provided by the user (optional)
 * @param psz_default_username default user name proposed
 * @param psz_title title of the login dialog
 * @param psz_fmt format string for the login message
 * @return < 0 on error, 0 if the user cancelled it, and 1 if ppsz_username and
 * ppsz_password are valid.
 */
HEIMDALL_API int
heimdall_dialog_wait_login(heimdall_object_t *p_obj, char **ppsz_username,
                      char **ppsz_password, bool *p_store,
                      const char *psz_default_username,
                      const char *psz_title, const char *psz_fmt, ...)
                      HEIMDALL_FORMAT(7,8);
#define heimdall_dialog_wait_login(a, b, c, d, e, f, g, ...) \
    heimdall_dialog_wait_login(HEIMDALL_OBJECT(a), b, c, d, e, f, g, ##__VA_ARGS__)

/**
 * Requests an user name and a password
 *
 * Equivalent to heimdall_dialog_wait_login() expect that it's called with a
 * va_list.
 */
HEIMDALL_API int
heimdall_dialog_wait_login_va(heimdall_object_t *p_obj, char **ppsz_username,
                         char **ppsz_password, bool *p_store,
                         const char *psz_default_username,
                         const char *psz_title, const char *psz_fmt, va_list ap);

/**
 * Asks a total (Yes/No/Cancel) question
 *
 * This function waits until the user dismisses the dialog or responds. It's
 * interruptible via heimdall_interrupt. In that case, heimdall_dialog_cbs.pf_cancel()
 * will be invoked. The psz_cancel is mandatory since this dialog is always
 * cancellable by the user.
 *
 * @param p_obj the Heimdall object emitting the dialog
 * @param i_type question type (severity of the question)
 * @param psz_cancel text of the cancel button
 * @param psz_action1 first choice/button text (optional)
 * @param psz_action2 second choice/button text (optional)
 * @param psz_title title of the question dialog
 * @param psz_fmt format string for the question message
 * @return < 0 on error, 0 if the user cancelled it, 1 on action1, 2 on action2
 */
HEIMDALL_API int
heimdall_dialog_wait_question(heimdall_object_t *p_obj,
                         heimdall_dialog_question_type i_type,
                         const char *psz_cancel, const char *psz_action1,
                         const char *psz_action2, const char *psz_title,
                         const char *psz_fmt, ...) HEIMDALL_FORMAT(7,8);
#define heimdall_dialog_wait_question(a, b, c, d, e, f, g, ...) \
    heimdall_dialog_wait_question(HEIMDALL_OBJECT(a), b, c, d, e, f, g, ##__VA_ARGS__)

/**
 * Asks a total (Yes/No/Cancel) question
 *
 * Equivalent to heimdall_dialog_wait_question() expect that it's called with a
 * va_list.
 */
HEIMDALL_API int
heimdall_dialog_wait_question_va(heimdall_object_t *p_obj,
                            heimdall_dialog_question_type i_type,
                            const char *psz_cancel, const char *psz_action1,
                            const char *psz_action2, const char *psz_title,
                            const char *psz_fmt, va_list ap);

/**
 * Display a progress dialog
 *
 * This function returns immediately
 *
 * @param p_obj the Heimdall object emitting the dialog
 * @param b_indeterminate true if the progress dialog is indeterminate
 * @param f_position initial position of the progress bar (between 0.0 and 1.0)
 * @param psz_cancel text of the cancel button, if NULL the dialog is not
 * cancellable (optional)
 * @param psz_title title of the progress dialog
 * @param psz_fmt format string for the progress message
 * @return a valid heimdall_dialog_id on success, must be released with
 * heimdall_dialog_id_release()
 */
HEIMDALL_API heimdall_dialog_id *
heimdall_dialog_display_progress(heimdall_object_t *p_obj, bool b_indeterminate,
                            float f_position, const char *psz_cancel,
                            const char *psz_title, const char *psz_fmt, ...)
                            HEIMDALL_FORMAT(6,7);
#define heimdall_dialog_display_progress(a, b, c, d, e, f, ...) \
    heimdall_dialog_display_progress(HEIMDALL_OBJECT(a), b, c, d, e, f, ##__VA_ARGS__)

/**
 * Display a progress dialog
 *
 * Equivalent to heimdall_dialog_display_progress() expect that it's called with a
 * va_list.
 */
HEIMDALL_API heimdall_dialog_id *
heimdall_dialog_display_progress_va(heimdall_object_t *p_obj, bool b_indeterminate,
                               float f_position, const char *psz_cancel,
                               const char *psz_title, const char *psz_fmt,
                               va_list ap);

/**
 * Update the position of the progress dialog
 *
 * @param p_obj the Heimdall object emitting the dialog
 * @param p_id id of the dialog to update
 * @param f_position position of the progress bar (between 0.0 and 1.0)
 * @return HEIMDALL_SUCCESS on success, or a Heimdall error code on error
 */
HEIMDALL_API int
heimdall_dialog_update_progress(heimdall_object_t *p_obj, heimdall_dialog_id *p_id,
                           float f_position);
#define heimdall_dialog_update_progress(a, b, c) \
    heimdall_dialog_update_progress(HEIMDALL_OBJECT(a), b, c)

/**
 * Update the position and the message of the progress dialog
 *
 * @param p_obj the Heimdall object emitting the dialog
 * @param p_id id of the dialog to update
 * @param f_position position of the progress bar (between 0.0 and 1.0)
 * @param psz_fmt format string for the progress message
 * @return HEIMDALL_SUCCESS on success, or a Heimdall error code on error
 */
HEIMDALL_API int
heimdall_dialog_update_progress_text(heimdall_object_t *p_obj, heimdall_dialog_id *p_id,
                                float f_position, const char *psz_fmt, ...)
                                HEIMDALL_FORMAT(4, 5);
#define heimdall_dialog_update_progress_text(a, b, c, d, ...) \
    heimdall_dialog_update_progress_text(HEIMDALL_OBJECT(a), b, c, d, ##__VA_ARGS__)

/**
 * Update the position and the message of the progress dialog
 *
 * Equivalent to heimdall_dialog_update_progress_text() expect that it's called
 * with a va_list.
 */
HEIMDALL_API int
heimdall_dialog_update_progress_text_va(heimdall_object_t *p_obj, heimdall_dialog_id *p_id,
                                   float f_position, const char *psz_fmt,
                                   va_list ap);

/**
 * Release the dialog id returned by heimdall_dialog_display_progress()
 *
 * It causes the heimdall_dialog_cbs.pf_cancel() callback to be invoked.
 *
 * @param p_obj the Heimdall object emitting the dialog
 * @param p_id id of the dialog to release
 */
HEIMDALL_API void
heimdall_dialog_release(heimdall_object_t *p_obj, heimdall_dialog_id *p_id);
#define heimdall_dialog_release(a, b) \
    heimdall_dialog_release(HEIMDALL_OBJECT(a), b)

/**
 * Return true if the dialog id is cancelled
 *
 * @param p_obj the Heimdall object emitting the dialog
 * @param p_id id of the dialog
 */
HEIMDALL_API bool
heimdall_dialog_is_cancelled(heimdall_object_t *p_obj, heimdall_dialog_id *p_id);
#define heimdall_dialog_is_cancelled(a, b) \
    heimdall_dialog_is_cancelled(HEIMDALL_OBJECT(a), b)

/**
 * @}
 * @defgroup heimdall_dialog_impl Heimdall dialog callbacks
 * Need to be implemented by GUI modules or libheimdall
 * @{
 */

/**
 * Dialog callbacks to be implemented
 */
typedef struct heimdall_dialog_cbs
{
    /**
     * Called when a login dialog needs to be displayed
     *
     * You can interact with this dialog by calling heimdall_dialog_id_post_login()
     * to post an answer or heimdall_dialog_id_dismiss() to cancel this dialog.
     *
     * @note to receive this callback, heimdall_dialog_cbs.pf_cancel should not be
     * NULL.
     *
     * @param p_data opaque pointer for the callback
     * @param p_id id used to interact with the dialog
     * @param psz_title title of the dialog
     * @param psz_text text of the dialog
     * @param psz_default_username user name that should be set on the user form
     * @param b_ask_store if true, ask the user if he wants to save the
     * credentials
     */
    void (*pf_display_login)(void *p_data, heimdall_dialog_id *p_id,
                             const char *psz_title, const char *psz_text,
                             const char *psz_default_username,
                             bool b_ask_store);

    /**
     * Called when a question dialog needs to be displayed
     *
     * You can interact with this dialog by calling heimdall_dialog_id_post_action()
     * to post an answer or heimdall_dialog_id_dismiss() to cancel this dialog.
     *
     * @note to receive this callback, heimdall_dialog_cbs.pf_cancel should not be
     * NULL.
     *
     * @param p_data opaque pointer for the callback
     * @param p_id id used to interact with the dialog
     * @param psz_title title of the dialog
     * @param psz_text text of the dialog
     * @param i_type question type (or severity) of the dialog
     * @param psz_cancel text of the cancel button
     * @param psz_action1 text of the first button, if NULL, don't display this
     * button
     * @param psz_action2 text of the second button, if NULL, don't display
     * this button
     */
    void (*pf_display_question)(void *p_data, heimdall_dialog_id *p_id,
                                const char *psz_title, const char *psz_text,
                                heimdall_dialog_question_type i_type,
                                const char *psz_cancel, const char *psz_action1,
                                const char *psz_action2);

    /**
     * Called when a progress dialog needs to be displayed
     *
     * If cancellable (psz_cancel != NULL), you can cancel this dialog by
     * calling heimdall_dialog_id_dismiss()
     *
     * @note to receive this callback, heimdall_dialog_cbs.pf_cancel and
     * heimdall_dialog_cbs.pf_update_progress should not be NULL.
     *
     * @param p_data opaque pointer for the callback
     * @param p_id id used to interact with the dialog
     * @param psz_title title of the dialog
     * @param psz_text text of the dialog
     * @param b_indeterminate true if the progress dialog is indeterminate
     * @param f_position initial position of the progress bar (between 0.0 and
     * 1.0)
     * @param psz_cancel text of the cancel button, if NULL the dialog is not
     * cancellable
     */
    void (*pf_display_progress)(void *p_data, heimdall_dialog_id *p_id,
                                const char *psz_title, const char *psz_text,
                                bool b_indeterminate, float f_position,
                                const char *psz_cancel);

    /**
     * Called when a displayed dialog needs to be cancelled
     *
     * The implementation must call heimdall_dialog_id_dismiss() to really release
     * the dialog.
     *
     * @param p_data opaque pointer for the callback
     * @param p_id id of the dialog
     */
    void (*pf_cancel)(void *p_data, heimdall_dialog_id *p_id);

    /**
     * Called when a progress dialog needs to be updated
     *
     * @param p_data opaque pointer for the callback
     * @param p_id id of the dialog
     * @param f_position osition of the progress bar (between 0.0 and 1.0)
     * @param psz_text new text of the progress dialog
     */
    void (*pf_update_progress)(void *p_data, heimdall_dialog_id *p_id,
                               float f_position, const char *psz_text);
} heimdall_dialog_cbs;

/**
 * Called when an error message needs to be displayed
 *
 * @param p_data opaque pointer for the callback
 * @param psz_title title of the dialog
 * @param psz_text text of the dialog
 */
typedef void (*heimdall_dialog_error_cbs)(void *p_data, const char *psz_title, const char *psz_text);

/**
 * Register callbacks to handle Heimdall dialogs
 *
 * @param p_obj the Heimdall object to get the dialog provider from
 * @param p_cbs a pointer to callbacks, or NULL to unregister callbacks.
 * @param p_data opaque pointer for the callback
 */
HEIMDALL_API void
heimdall_dialog_provider_set_callbacks(heimdall_object_t *p_obj,
                                  const heimdall_dialog_cbs *p_cbs, void *p_data);
#define heimdall_dialog_provider_set_callbacks(a, b, c) \
    heimdall_dialog_provider_set_callbacks(HEIMDALL_OBJECT(a), b, c)

/**
 * Register callbacks to handle Heimdall error messages
 *
 * @version LibHeimdall 4.0.0 and later.
 *
 * @param p_obj the Heimdall object to get the dialog provider from
 * @param p_cbs a pointer to the callback, or NULL to unregister the callback.
 * @param p_data opaque pointer for the callback
 */
HEIMDALL_API void
heimdall_dialog_provider_set_error_callback(heimdall_object_t *p_obj,
                                       heimdall_dialog_error_cbs p_cbs, void *p_data);
#define heimdall_dialog_provider_set_error_callback(a, b, c) \
    heimdall_dialog_provider_set_error_callback(HEIMDALL_OBJECT(a), b, c)

/**
 * Associate an opaque pointer with the dialog id
 */
HEIMDALL_API void
heimdall_dialog_id_set_context(heimdall_dialog_id *p_id, void *p_context);

/**
 * Return the opaque pointer associated with the dialog id
 */
HEIMDALL_API void *
heimdall_dialog_id_get_context(heimdall_dialog_id *p_id);

/**
 * Post a login answer
 *
 * After this call, p_id won't be valid anymore
 *
 * @see heimdall_dialog_cbs.pf_display_login
 *
 * @param p_id id of the dialog
 * @param psz_username valid and non empty string
 * @param psz_password valid string (can be empty)
 * @param b_store if true, store the credentials
 * @return HEIMDALL_SUCCESS on success, or a Heimdall error code on error
 */
HEIMDALL_API int
heimdall_dialog_id_post_login(heimdall_dialog_id *p_id, const char *psz_username,
                         const char *psz_password, bool b_store);

/**
 * Post a question answer
 *
 * After this call, p_id won't be valid anymore
 *
 * @see heimdall_dialog_cbs.pf_display_question
 *
 * @param p_id id of the dialog
 * @param i_action 1 for action1, 2 for action2
 * @return HEIMDALL_SUCCESS on success, or a Heimdall error code on error
 */
HEIMDALL_API int
heimdall_dialog_id_post_action(heimdall_dialog_id *p_id, int i_action);

/**
 * Dismiss a dialog
 *
 * After this call, p_id won't be valid anymore
 *
 * @see heimdall_dialog_cbs.pf_cancel
 *
 * @param p_id id of the dialog
 * @return HEIMDALL_SUCCESS on success, or a Heimdall error code on error
 */
HEIMDALL_API int
heimdall_dialog_id_dismiss(heimdall_dialog_id *p_id);

/**
 * @}
 * @defgroup heimdall_dialog_ext Heimdall extension dialog functions
 * @{
 */

HEIMDALL_API int
heimdall_ext_dialog_update(heimdall_object_t *p_obj, extension_dialog_t *dialog);
#define heimdall_ext_dialog_update(a, b) \
    heimdall_ext_dialog_update(HEIMDALL_OBJECT(a), b)

/**
 * Dialog extension callback to be implemented
 */
typedef void (*heimdall_dialog_ext_update_cb)(extension_dialog_t *p_ext_dialog,
                                         void *p_data);

/**
 * Register a callback for Heimdall extension dialog
 *
 * @param p_obj a Heimdall object to get the libheimdall instance from
 * @param pf_update a pointer to the update callback, or NULL to unregister
 * callback
 * @param p_data opaque pointer for the callback
 */
HEIMDALL_API void
heimdall_dialog_provider_set_ext_callback(heimdall_object_t *p_obj,
                                     heimdall_dialog_ext_update_cb pf_update,
                                     void *p_data);
#define heimdall_dialog_provider_set_ext_callback(a, b, c) \
    heimdall_dialog_provider_set_ext_callback(HEIMDALL_OBJECT(a), b, c)

/** @} */
/** @} */

#endif
