/*****************************************************************************
 * libheimdall.h: Internal libheimdall generic/misc declaration
 *****************************************************************************
 * Copyright (C) 1999, 2000, 2001, 2002 VLC authors and VideoLAN
 * Copyright © 2006-2007 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
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

#ifndef LIBHEIMDALL_LIBHEIMDALL_H
# define LIBHEIMDALL_LIBHEIMDALL_H 1

#include <heimdall_input_item.h>

# ifdef __cplusplus
extern "C" {
# endif

extern const char psz_heimdall_changeset[];

typedef struct variable_t variable_t;

/*
 * OS-specific initialization
 */
void system_Init      ( void );
void system_Configure ( libheimdall_int_t *, int, const char *const [] );
#if defined(_WIN32) || defined(__OS2__)
void system_End(void);
#endif
void heimdall_CPU_dump(heimdall_object_t *);

/*
 * Threads subsystem
 */

void heimdall_threads_setup (libheimdall_int_t *);

void heimdall_trace (const char *fn, const char *file, unsigned line);
#define heimdall_backtrace() heimdall_trace(__func__, __FILE__, __LINE__)

/*
 * Logging
 */
typedef struct heimdall_logger heimdall_logger_t;
typedef struct heimdall_tracer heimdall_tracer_t;

int heimdall_LogPreinit(libheimdall_int_t *) HEIMDALL_USED;
void heimdall_LogInit(libheimdall_int_t *);

/*
 * LibHeimdall exit event handling
 */
typedef struct heimdall_exit
{
    heimdall_mutex_t lock;
    void (*handler) (void *);
    void *opaque;
} heimdall_exit_t;

void heimdall_ExitInit( heimdall_exit_t * );

/*
 * LibHeimdall objects stuff
 */

/**
 * Initializes a Heimdall object.
 *
 * @param obj storage space for object to initialize [OUT]
 * @param parent parent object (or NULL to initialize the root) [IN]
 * @param type_name object type name
 *
 * @note The type name pointer must remain valid even after the object is
 * deinitialized, as it might be passed by address to log message queue.
 * Using constant string literals is appropriate.
 *
 * @retval 0 on success
 * @retval -1 on (out of memory) error
 */
int heimdall_object_init(heimdall_object_t *obj, heimdall_object_t *parent,
                    const char *type_name);

/**
 * Deinitializes a Heimdall object.
 *
 * This frees resources allocated by heimdall_object_init().
 */
void heimdall_object_deinit(heimdall_object_t *obj);

/**
 * Creates a Heimdall object.
 *
 * Note that because the object name pointer must remain valid, potentially
 * even after the destruction of the object (through the message queues), this
 * function CANNOT be exported to plugins as is. In this case, the old
 * heimdall_object_create() must be used instead.
 *
 * @param p_this an existing Heimdall object
 * @param i_size byte size of the object structure
 * @param psz_type object type name
 * @return the created object, or NULL.
 */
extern void *
heimdall_custom_create (heimdall_object_t *p_this, size_t i_size, const char *psz_type);
#define heimdall_custom_create(o, s, n) \
        heimdall_custom_create(HEIMDALL_OBJECT(o), s, n)

/**
 * Allocates an object resource.
 *
 * @param size storage size in bytes of the resource data
 * @param release callback to release the resource
 *
 * @return a pointer to the (uninitialized) storage space, or NULL on error
 */
void *heimdall_objres_new(size_t size, void (*release)(void *));

/**
 * Pushes an object resource on the object resources stack.
 *
 * @param obj object to allocate the resource for
 * @param data resource base address (as returned by heimdall_objres_new())
 */
void heimdall_objres_push(heimdall_object_t *obj, void *data);

/**
 * Releases all resources of an object.
 *
 * All resources added with heimdall_objres_add() are released in reverse order.
 * The resource list is reset to empty.
 *
 * @param obj object whose resources to release
 */
void heimdall_objres_clear(heimdall_object_t *obj);

/**
 * Releases one object resource explicitly.
 *
 * If a resource associated with an object needs to be released explicitly
 * earlier than normal, call this function. This is relatively slow and should
 * be avoided.
 *
 * @param obj object whose resource to release
 * @param data private data for the comparison function
 * @param match comparison function to match the targeted resource
 */
void heimdall_objres_remove(heimdall_object_t *obj, void *data,
                       bool (*match)(void *, void *));

/**
 * Private LibHeimdall instance data.
 */
typedef struct heimdall_dialog_provider heimdall_dialog_provider;
typedef struct heimdall_keystore heimdall_keystore;
typedef struct heimdall_actions_t heimdall_actions_t;
typedef struct heimdall_playlist heimdall_playlist_t;
typedef struct heimdall_media_source_provider_t heimdall_media_source_provider_t;
typedef struct intf_thread_t intf_thread_t;

typedef struct libheimdall_priv_t
{
    libheimdall_int_t       public_data;

    /* Singleton objects */
    heimdall_mutex_t lock; ///< protect playlist and interfaces
    vlm_t             *p_vlm;  ///< the VLM singleton (or NULL)
    heimdall_dialog_provider *p_dialog_provider; ///< dialog provider
    heimdall_keystore      *p_memory_keystore; ///< memory keystore
    intf_thread_t *interfaces;  ///< Linked-list of interfaces
    heimdall_playlist_t *main_playlist;
    heimdall_media_source_provider_t *media_source_provider;
    heimdall_actions_t *actions; ///< Hotkeys handler
    struct heimdall_medialibrary_t *p_media_library; ///< Media library instance
    struct heimdall_tracer *tracer; ///< Tracer callbacks

    /* Exit callback */
    heimdall_exit_t       exit;
} libheimdall_priv_t;

static inline libheimdall_priv_t *libheimdall_priv (libheimdall_int_t *libheimdall)
{
    return container_of(libheimdall, libheimdall_priv_t, public_data);
}

int intf_InsertItem(libheimdall_int_t *, const char *mrl, unsigned optc,
                    const char * const *optv, unsigned flags);
void intf_DestroyAll( libheimdall_int_t * );

heimdall_playlist_t *
libheimdall_GetMainPlaylist(libheimdall_int_t *libheimdall);

/*
 * Variables stuff
 */
void var_OptionParse (heimdall_object_t *, const char *, bool trusted);

# ifdef __cplusplus
} // extern "C"
# endif

#endif
