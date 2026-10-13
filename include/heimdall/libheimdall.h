/*****************************************************************************
 * libheimdall.h:  libheimdall external API
 *****************************************************************************
 * Copyright (C) 1998-2009 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Paul Saman <jpsaman@videolan.org>
 *          Pierre d'Herbemont <pdherbemont@videolan.org>
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

/**
 * \defgroup libheimdall LibHeimdall
 * LibHeimdall is the external programming interface of the Heimdall media player.
 * It is used to embed Heimdall into other applications or frameworks.
 * @{
 * \file
 * LibHeimdall core external API
 */

#ifndef HEIMDALL_LIBHEIMDALL_H
#define HEIMDALL_LIBHEIMDALL_H 1

#if (defined (_WIN32) || defined (__OS2__)) && defined (LIBHEIMDALL_DLL_EXPORT)
# define LIBHEIMDALL_API __declspec(dllexport)
#elif defined (__GNUC__) && (__GNUC__ >= 4)
# define LIBHEIMDALL_API __attribute__((visibility("default")))
#else
# define LIBHEIMDALL_API
#endif

#ifdef LIBHEIMDALL_INTERNAL_
/* Avoid unhelpful warnings from libheimdall with our deprecated APIs */
#   define LIBHEIMDALL_DEPRECATED
#elif defined(__GNUC__) && \
      (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
# define LIBHEIMDALL_DEPRECATED __attribute__((deprecated))
#else
# define LIBHEIMDALL_DEPRECATED
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

# ifdef __cplusplus
extern "C" {
# endif

/** \defgroup libheimdall_core LibHeimdall core
 * \ingroup libheimdall
 * Before it can do anything useful, LibHeimdall must be initialized.
 * You can create one (or more) instance(s) of LibHeimdall in a given process,
 * with libheimdall_new() and destroy them with libheimdall_release().
 *
 * \version Unless otherwise stated, these functions are available
 * from LibHeimdall versions numbered 1.1.0 or more.
 * Earlier versions (0.9.x and 1.0.x) are <b>not</b> compatible.
 * @{
 */

/** This structure is opaque. It represents a libheimdall instance */
typedef struct libheimdall_instance_t libheimdall_instance_t;

/** Represents a time value in microseconds */
typedef int64_t libheimdall_time_t;

/** \defgroup libheimdall_error LibHeimdall error handling
 * @{
 */

/**
 * A human-readable error message for the last LibHeimdall error in the calling
 * thread. The resulting string is valid until another error occurs (at least
 * until the next LibHeimdall call).
 *
 * @warning
 * This will be NULL if there was no error.
 */
LIBHEIMDALL_API const char *libheimdall_errmsg (void);

/**
 * Clears the LibHeimdall error status for the current thread. This is optional.
 * By default, the error status is automatically overridden when a new error
 * occurs, and destroyed when the thread exits.
 */
LIBHEIMDALL_API void libheimdall_clearerr (void);

/**
 * Sets the LibHeimdall error status and message for the current thread.
 * Any previous error is overridden.
 * \param fmt the format string
 * \param ...  the arguments for the format string
 * \return a nul terminated string in any case
 */
const char *libheimdall_printerr (const char *fmt, ...);

/**@} */

/**
 * Create and initialize a libheimdall instance.
 * This functions accept a list of "command line" arguments similar to the
 * main(). These arguments affect the LibHeimdall instance default configuration.
 *
 * \note
 * LibHeimdall may create threads. Therefore, any thread-unsafe process
 * initialization must be performed before calling libheimdall_new(). In particular
 * and where applicable:
 * - setlocale() and textdomain(),
 * - setenv(), unsetenv() and putenv(),
 * - with the X11 display system, XInitThreads()
 *   (see also libheimdall_media_player_set_xwindow()) and
 * - on Microsoft Windows, SetErrorMode().
 * - sigprocmask() shall never be invoked; pthread_sigmask() can be used.
 *
 * On POSIX systems, the SIGCHLD signal <b>must not</b> be ignored, i.e. the
 * signal handler must set to SIG_DFL or a function pointer, not SIG_IGN.
 * Also while LibHeimdall is active, the wait() function shall not be called, and
 * any call to waitpid() shall use a strictly positive value for the first
 * parameter (i.e. the PID). Failure to follow those rules may lead to a
 * deadlock or a busy loop.
 * Also on POSIX systems, it is recommended that the SIGPIPE signal be blocked,
 * even if it is not, in principles, necessary, e.g.:
 * @code
   sigset_t set;

   signal(SIGCHLD, SIG_DFL);
   sigemptyset(&set);
   sigaddset(&set, SIGPIPE);
   pthread_sigmask(SIG_BLOCK, &set, NULL);
 * @endcode
 *
 * On Microsoft Windows, setting the default DLL directories to SYSTEM32
 * exclusively is strongly recommended for security reasons:
 * @code
   SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
 * @endcode
 *
 * \version
 * Arguments are meant to be passed from the command line to LibHeimdall, just like
 * Heimdall media player does. The list of valid arguments depends on the LibHeimdall
 * version, the operating system and platform, and set of available LibHeimdall
 * plugins. Invalid or unsupported arguments will cause the function to fail
 * (i.e. return NULL). Also, some arguments may alter the behaviour or
 * otherwise interfere with other LibHeimdall functions.
 *
 * \warning
 * There is absolutely no warranty or promise of forward, backward and
 * cross-platform compatibility with regards to libheimdall_new() arguments.
 * We recommend that you do not use them, other than when debugging.
 *
 * \param argc the number of arguments (should be 0)
 * \param argv list of arguments (should be NULL)
 * \return the libheimdall instance or NULL in case of error
 */
LIBHEIMDALL_API libheimdall_instance_t *
libheimdall_new( int argc , const char *const *argv );

/**
 * Decrement the reference count of a libheimdall instance, and destroy it
 * if it reaches zero.
 *
 * \param p_instance the instance to destroy
 */
LIBHEIMDALL_API void libheimdall_release( libheimdall_instance_t *p_instance );

/**
 * Increments the reference count of a libheimdall instance.
 * The initial reference count is 1 after libheimdall_new() returns.
 *
 * \param p_instance the instance to reference
 * \return the same object
 */
LIBHEIMDALL_API libheimdall_instance_t *libheimdall_retain( libheimdall_instance_t *p_instance );

/**
 * Get the ABI version of the libheimdall library.
 *
 * This is different than the Heimdall version, which is the version of the whole
 * Heimdall package. The value is the same as LIBHEIMDALL_ABI_VERSION_INT used when
 * compiling.
 *
 * \return a value with the following mask in hexadecimal
 *  0xFF000000: major Heimdall version, similar to Heimdall major version,
 *  0x00FF0000: major ABI version, incremented incompatible changes are added,
 *  0x0000FF00: minor ABI version, incremented when new functions are added
 *  0x000000FF: micro ABI version, incremented with new release/builds
 *
 * \note This the same value as the .so version but cross platform.
 */
LIBHEIMDALL_API int libheimdall_abi_version(void);

/**
 * Sets the application name. LibHeimdall passes this as the user agent string
 * when a protocol requires it.
 *
 * \param p_instance LibHeimdall instance
 * \param name human-readable application name, e.g. "FooBar player 1.2.3"
 * \param http HTTP User Agent, e.g. "FooBar/1.2.3 Python/2.6.0"
 * \version LibHeimdall 1.1.1 or later
 */
LIBHEIMDALL_API
void libheimdall_set_user_agent( libheimdall_instance_t *p_instance,
                            const char *name, const char *http );

/**
 * Sets some meta-information about the application.
 * See also libheimdall_set_user_agent().
 *
 * \param p_instance LibHeimdall instance
 * \param id Java-style application identifier, e.g. "com.acme.foobar"
 * \param version application version numbers, e.g. "1.2.3"
 * \param icon application icon name, e.g. "foobar"
 * \version LibHeimdall 2.1.0 or later.
 */
LIBHEIMDALL_API
void libheimdall_set_app_id( libheimdall_instance_t *p_instance, const char *id,
                        const char *version, const char *icon );

/**
 * Retrieve libheimdall version.
 *
 * Example: "1.1.0-git The Luggage"
 *
 * \return a string containing the libheimdall version
 */
LIBHEIMDALL_API const char * libheimdall_get_version(void);

/**
 * Retrieve libheimdall compiler version.
 *
 * Example: "gcc version 4.2.3 (Ubuntu 4.2.3-2ubuntu6)"
 *
 * \return a string containing the libheimdall compiler version
 */
LIBHEIMDALL_API const char * libheimdall_get_compiler(void);

/**
 * Retrieve libheimdall changeset.
 *
 * Example: "aa9bce0bc4"
 *
 * \return a string containing the libheimdall changeset
 */
LIBHEIMDALL_API const char * libheimdall_get_changeset(void);

/**
 * Frees an heap allocation returned by a LibHeimdall function.
 * If you know you're using the same underlying C run-time as the LibHeimdall
 * implementation, then you can call ANSI C free() directly instead.
 *
 * \param ptr the pointer
 */
LIBHEIMDALL_API void libheimdall_free( void *ptr );

/** \defgroup libheimdall_log LibHeimdall logging
 * libheimdall_log_* functions provide access to the LibHeimdall messages log.
 * This is used for logging and debugging.
 * @{
 */

/**
 * Logging messages level.
 * \note Future LibHeimdall versions may define new levels.
 */
enum libheimdall_log_level
{
    LIBHEIMDALL_DEBUG=0,   /**< Debug message */
    LIBHEIMDALL_NOTICE=2,  /**< Important informational message */
    LIBHEIMDALL_WARNING=3, /**< Warning (potential error) message */
    LIBHEIMDALL_ERROR=4    /**< Error message */
};

typedef struct heimdall_log_t libheimdall_log_t;

/**
 * Gets log message debug infos.
 *
 * This function retrieves self-debug information about a log message:
 * - the name of the Heimdall module emitting the message,
 * - the name of the source code module (i.e. file) and
 * - the line number within the source code module.
 *
 * The returned module name and file name will be NULL if unknown.
 * The returned line number will similarly be zero if unknown.
 *
 * \param ctx message context (as passed to the @ref libheimdall_log_cb callback)
 * \param module module name storage (or NULL) [OUT]
 * \param file source code file name storage (or NULL) [OUT]
 * \param line source code file line number storage (or NULL) [OUT]
 * \warning The returned module name and source code file name, if non-NULL,
 * are only valid until the logging callback returns.
 *
 * \version LibHeimdall 2.1.0 or later
 */
LIBHEIMDALL_API void libheimdall_log_get_context(const libheimdall_log_t *ctx,
                       const char **module, const char **file, unsigned *line);

/**
 * Gets log message info.
 *
 * This function retrieves meta-information about a log message:
 * - the type name of the Heimdall object emitting the message,
 * - the object header if any, and
 * - a temporaly-unique object identifier.
 *
 * This information is mainly meant for <b>manual</b> troubleshooting.
 *
 * The returned type name may be "generic" if unknown, but it cannot be NULL.
 * The returned header will be NULL if unset; in current versions, the header
 * is used to distinguish for VLM inputs.
 * The returned object ID will be zero if the message is not associated with
 * any Heimdall object.
 *
 * \param ctx message context (as passed to the @ref libheimdall_log_cb callback)
 * \param name object name storage (or NULL) [OUT]
 * \param header object header (or NULL) [OUT]
 * \param id temporarily-unique object identifier (or 0) [OUT]
 * \warning The returned module name and source code file name, if non-NULL,
 * are only valid until the logging callback returns.
 *
 * \version LibHeimdall 2.1.0 or later
 */
LIBHEIMDALL_API void libheimdall_log_get_object(const libheimdall_log_t *ctx,
                        const char **name, const char **header, uintptr_t *id);

/**
 * Callback prototype for LibHeimdall log message handler.
 *
 * \param data data pointer as given to libheimdall_log_set()
 * \param level message level (@ref libheimdall_log_level)
 * \param ctx message context (meta-information about the message)
 * \param fmt printf() format string (as defined by ISO C11)
 * \param args variable argument list for the format
 * \note Log message handlers <b>must</b> be thread-safe.
 * \warning The message context pointer, the format string parameters and the
 *          variable arguments are only valid until the callback returns.
 */
typedef void (*libheimdall_log_cb)(void *data, int level, const libheimdall_log_t *ctx,
                              const char *fmt, va_list args);

/**
 * Unsets the logging callback.
 *
 * This function deregisters the logging callback for a LibHeimdall instance.
 * This is rarely needed as the callback is implicitly unset when the instance
 * is destroyed.
 *
 * \note This function will wait for any pending callbacks invocation to
 * complete (causing a deadlock if called from within the callback).
 *
 * \param p_instance libheimdall instance
 * \version LibHeimdall 2.1.0 or later
 */
LIBHEIMDALL_API void libheimdall_log_unset( libheimdall_instance_t *p_instance );

/**
 * Sets the logging callback for a LibHeimdall instance.
 *
 * This function is thread-safe: it will wait for any pending callbacks
 * invocation to complete.
 *
 * \param cb callback function pointer
 * \param data opaque data pointer for the callback function
 *
 * \note Some log messages (especially debug) are emitted by LibHeimdall while
 * is being initialized. These messages cannot be captured with this interface.
 *
 * \warning A deadlock may occur if this function is called from the callback.
 *
 * \param p_instance libheimdall instance
 * \version LibHeimdall 2.1.0 or later
 */
LIBHEIMDALL_API void libheimdall_log_set( libheimdall_instance_t *p_instance,
                                libheimdall_log_cb cb, void *data );


/**
 * Sets up logging to a file.
 * \param p_instance libheimdall instance
 * \param stream FILE pointer opened for writing
 *         (the FILE pointer must remain valid until libheimdall_log_unset())
 * \version LibHeimdall 2.1.0 or later
 */
LIBHEIMDALL_API void libheimdall_log_set_file( libheimdall_instance_t *p_instance, FILE *stream );

/** @} */

/**
 * Description of a module.
 */
typedef struct libheimdall_module_description_t
{
    char *psz_name;
    char *psz_shortname;
    char *psz_longname;
    char *psz_help;
    char *psz_help_html;
    struct libheimdall_module_description_t *p_next;
} libheimdall_module_description_t;

/**
 * Release a list of module descriptions.
 *
 * \param p_list the list to be released
 */
LIBHEIMDALL_API
void libheimdall_module_description_list_release( libheimdall_module_description_t *p_list );

/**
 * Returns a list of audio filters that are available.
 *
 * \param p_instance libheimdall instance
 *
 * \return a list of module descriptions. It should be freed with libheimdall_module_description_list_release().
 *         In case of an error, NULL is returned.
 *
 * \see libheimdall_module_description_t
 * \see libheimdall_module_description_list_release
 */
LIBHEIMDALL_API
libheimdall_module_description_t *libheimdall_audio_filter_list_get( libheimdall_instance_t *p_instance );

/**
 * Returns a list of video filters that are available.
 *
 * \param p_instance libheimdall instance
 *
 * \return a list of module descriptions. It should be freed with libheimdall_module_description_list_release().
 *         In case of an error, NULL is returned.
 *
 * \see libheimdall_module_description_t
 * \see libheimdall_module_description_list_release
 */
LIBHEIMDALL_API
libheimdall_module_description_t *libheimdall_video_filter_list_get( libheimdall_instance_t *p_instance );

/** @} */

/** \defgroup libheimdall_clock LibHeimdall time
 * These functions provide access to the LibHeimdall time/clock.
 * @{
 */

/**
 * Return the current time as defined by LibHeimdall. The unit is the microsecond.
 * Time increases monotonically (regardless of time zone changes and RTC
 * adjustments).
 * The origin is arbitrary but consistent across the whole system
 * (e.g. the system uptime, the time since the system was booted).
 * \note On systems that support it, the POSIX monotonic clock is used.
 */
LIBHEIMDALL_API
libheimdall_time_t libheimdall_clock(void);

/**
 * Return the delay (in microseconds) until a certain timestamp.
 * \param pts timestamp
 * \return negative if timestamp is in the past,
 * positive if it is in the future
 */
static inline libheimdall_time_t libheimdall_delay(libheimdall_time_t pts)
{
    return pts - libheimdall_clock();
}

/** @} */

# ifdef __cplusplus
}
# endif

#endif /** @} */
