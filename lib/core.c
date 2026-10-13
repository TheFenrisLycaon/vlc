/*****************************************************************************
 * core.c: Core libheimdall new API functions : initialization
 *****************************************************************************
 * Copyright (C) 2005 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
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

#include "libheimdall_internal.h"
#include <heimdall_modules.h>
#include <heimdall/heimdall.h>

#include <heimdall_interface.h>

#include <stdarg.h>
#include <limits.h>
#include <assert.h>

static_assert(LIBHEIMDALL_VERSION_MAJOR    == PACKAGE_VERSION_MAJOR, "Major Heimdall version mismatch");
static_assert(LIBHEIMDALL_VERSION_MINOR    == PACKAGE_VERSION_MINOR, "Minor Heimdall version mismatch");
static_assert(LIBHEIMDALL_VERSION_REVISION == PACKAGE_VERSION_REVISION, "Heimdall Revision version mismatch");
static_assert(LIBHEIMDALL_VERSION_EXTRA    == PACKAGE_VERSION_EXTRA, "Heimdall Extra version mismatch");
static_assert(LIBHEIMDALL_ABI_VERSION_MAJOR == LIBHEIMDALL_ABI_MAJOR, "Major LibHeimdall version mismatch");
static_assert(LIBHEIMDALL_ABI_VERSION_MINOR == LIBHEIMDALL_ABI_MINOR, "Minor LibHeimdall version mismatch");
static_assert(LIBHEIMDALL_ABI_VERSION_MICRO == LIBHEIMDALL_ABI_MICRO, "Micro LibHeimdall version mismatch");

int libheimdall_abi_version(void)
{
    return LIBHEIMDALL_ABI_VERSION_INT;
}

libheimdall_instance_t * libheimdall_new( int argc, const char *const *argv )
{
    libheimdall_threads_init ();

    libheimdall_instance_t *p_new = malloc (sizeof (*p_new));
    if (unlikely(p_new == NULL))
        return NULL;

    const char *my_argv[argc + 2];
    my_argv[0] = "libheimdall"; /* dummy arg0, skipped by getopt() et al */
    for( int i = 0; i < argc; i++ )
         my_argv[i + 1] = argv[i];
    my_argv[argc + 1] = NULL; /* C calling conventions require a NULL */

    libheimdall_int_t *p_libheimdall_int = libheimdall_InternalCreate();
    if (unlikely (p_libheimdall_int == NULL))
        goto error;

    const int ret = libheimdall_InternalInit( p_libheimdall_int, argc + 1, my_argv );
    if (ret != HEIMDALL_SUCCESS)
    {
        libheimdall_InternalDestroy( p_libheimdall_int );
        const char *error = (ret == HEIMDALL_EGENERIC) ? _( "Generic Heimdall error" )
                                                  : heimdall_strerror_c( -ret );
        libheimdall_printerr( "%s", error );
        goto error;
    }

    p_new->p_libheimdall_int = p_libheimdall_int;
    heimdall_atomic_rc_init( &p_new->ref_count );
    p_new->p_callback_list = NULL;

    return p_new;

error:
    free (p_new);
    libheimdall_threads_deinit ();
    return NULL;
}

libheimdall_instance_t *libheimdall_retain( libheimdall_instance_t *p_instance )
{
    assert( p_instance != NULL );

    heimdall_atomic_rc_inc( &p_instance->ref_count );
    return p_instance;
}

void libheimdall_release( libheimdall_instance_t *p_instance )
{
    if(heimdall_atomic_rc_dec( &p_instance->ref_count ))
    {
        libheimdall_Quit( p_instance->p_libheimdall_int );

        libheimdall_InternalCleanup( p_instance->p_libheimdall_int );
        libheimdall_InternalDestroy( p_instance->p_libheimdall_int );
        free( p_instance );
        libheimdall_threads_deinit ();
    }
}

void libheimdall_set_user_agent (libheimdall_instance_t *p_i,
                            const char *name, const char *http)
{
    libheimdall_int_t *p_libheimdall = p_i->p_libheimdall_int;
    char *str;

    var_SetString (p_libheimdall, "user-agent", name);
    if ((http != NULL)
     && (asprintf (&str, "%s LibHeimdall/"PACKAGE_VERSION, http) != -1))
    {
        var_SetString (p_libheimdall, "http-user-agent", str);
        free (str);
    }
}

void libheimdall_set_app_id(libheimdall_instance_t *p_i, const char *id,
                       const char *version, const char *icon)
{
    libheimdall_int_t *p_libheimdall = p_i->p_libheimdall_int;

    var_SetString(p_libheimdall, "app-id", id ? id : "");
    var_SetString(p_libheimdall, "app-version", version ? version : "");
    var_SetString(p_libheimdall, "app-icon-name", icon ? icon : "");
}

const char * libheimdall_get_version(void)
{
    return VERSION_MESSAGE;
}

const char * libheimdall_get_compiler(void)
{
    return Heimdall_Compiler();
}

const char * libheimdall_get_changeset(void)
{
    extern const char psz_heimdall_changeset[];
    return psz_heimdall_changeset;
}

void libheimdall_free( void *ptr )
{
    free( ptr );
}

static libheimdall_module_description_t *module_description_list_get(
                libheimdall_instance_t *p_instance, const char *capability )
{
    libheimdall_module_description_t *p_list = NULL,
                          *p_actual = NULL,
                          *p_previous = NULL;
    size_t count;
    module_t **module_list = module_list_get( &count );

    for (size_t i = 0; i < count; i++)
    {
        module_t *p_module = module_list[i];

        if ( !module_provides( p_module, capability ) )
            continue;

        p_actual = ( libheimdall_module_description_t * ) malloc( sizeof( libheimdall_module_description_t ) );
        if ( p_actual == NULL )
        {
            libheimdall_printerr( "Not enough memory" );
            libheimdall_module_description_list_release( p_list );
            module_list_free( module_list );
            return NULL;
        }

        if ( p_list == NULL )
            p_list = p_actual;

        const char* name = module_get_object( p_module );
        const char* shortname = module_GetShortName( p_module );
        const char* longname = module_GetLongName( p_module );
        const char* help = module_get_help( p_module );
        const char* help_html = module_get_help_html( p_module );
        p_actual->psz_name = name ? strdup( name ) : NULL;
        p_actual->psz_shortname = shortname ? strdup( shortname ) : NULL;
        p_actual->psz_longname = longname ? strdup( longname ) : NULL;
        p_actual->psz_help = help ? strdup( help ) : NULL;
        p_actual->psz_help_html = help_html ? strdup( help_html ) : NULL;

        p_actual->p_next = NULL;
        if ( p_previous )
            p_previous->p_next = p_actual;
        p_previous = p_actual;
    }

    module_list_free( module_list );
    HEIMDALL_UNUSED( p_instance );
    return p_list;
}

void libheimdall_module_description_list_release( libheimdall_module_description_t *p_list )
{
    libheimdall_module_description_t *p_actual, *p_before;
    p_actual = p_list;

    while ( p_actual )
    {
        free( p_actual->psz_name );
        free( p_actual->psz_shortname );
        free( p_actual->psz_longname );
        free( p_actual->psz_help );
        free( p_actual->psz_help_html );
        p_before = p_actual;
        p_actual = p_before->p_next;
        free( p_before );
    }
}

libheimdall_module_description_t *libheimdall_audio_filter_list_get( libheimdall_instance_t *p_instance )
{
    return module_description_list_get( p_instance, "audio filter" );
}

libheimdall_module_description_t *libheimdall_video_filter_list_get( libheimdall_instance_t *p_instance )
{
    return module_description_list_get( p_instance, "video filter" );
}

libheimdall_time_t libheimdall_clock(void)
{
    return US_FROM_HEIMDALL_TICK(heimdall_tick_now());
}

const char heimdall_module_name[] = "libheimdall";
