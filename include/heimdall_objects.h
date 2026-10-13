/*****************************************************************************
 * heimdall_objects.h: heimdall_object_t definition and manipulation methods
 *****************************************************************************
 * Copyright (C) 2002-2008 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
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

#ifndef HEIMDALL_OBJECTS_H
#define HEIMDALL_OBJECTS_H 1

#ifdef __cplusplus
#include <new>
extern "C" {
#endif

/**
 * \defgroup heimdall_object Heimdall objects
 * \ingroup heimdall
 * @{
 * \file
 * Common Heimdall object definitions
 */

struct heimdall_logger;
struct heimdall_tracer;
struct heimdall_object_internals;
struct heimdall_object_marker;

/**
 * Heimdall object common members
 *
 * Common public properties for all Heimdall objects.
 * Object also have private properties maintained by the core, see
 * \ref heimdall_object_internals_t
 */
struct heimdall_object_t
{
    struct heimdall_logger *logger;
    union {
        struct heimdall_object_internals *priv;
        struct heimdall_object_marker *obj;
    };

    bool no_interact;

    /** Module probe flag
     *
     * A boolean during module probing when the probe is "forced".
     * See \ref module_need().
     */
    bool force;
};

#ifdef __cplusplus
}
#endif

/**
 * Type-safe heimdall_object_t cast
 *
 * This macro attempts to cast a pointer to a compound type to a
 * \ref heimdall_object_t pointer in a type-safe manner.
 * It checks if the compound type actually starts with an embedded
 * \ref heimdall_object_t structure.
 */
#if !defined(__cplusplus)
# define HEIMDALL_OBJECT(x) \
    _Generic((x)->obj, \
        struct heimdall_object_marker *: (x), \
        struct heimdall_object_t: (&((x)->obj)) \
    )
#else
static inline heimdall_object_t *HEIMDALL_OBJECT(heimdall_object_t *o)
    { return o; }

template<typename T>
static inline heimdall_object_t *HEIMDALL_OBJECT(T *d)
    { return &d->obj; }
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The root object */
struct libheimdall_int_t
{
    struct heimdall_object_t obj;
};

/**
 * Allocates and initializes a heimdall object.
 *
 * The object will need to be released with ::heimdall_object_release()
 * before \p parent is released.
 *
 * @param parent A parent object to create the new object from
 * @param i_size object byte size
 *
 * @return the new object, or NULL on error.
 */
HEIMDALL_API void *heimdall_object_create(heimdall_object_t *parent, size_t i_size) HEIMDALL_MALLOC HEIMDALL_USED;

/**
 * Drops the strong reference to an object.
 *
 * This removes the initial strong reference to a given object. This must be
 * called exactly once per allocated object after it is no longer needed,
 * matching heimdall_object_create() or heimdall_custom_create().
 */
HEIMDALL_API void heimdall_object_delete(heimdall_object_t *obj);
#define heimdall_object_delete(obj) heimdall_object_delete(HEIMDALL_OBJECT(obj))

HEIMDALL_API size_t heimdall_list_children(heimdall_object_t *, heimdall_object_t **, size_t) HEIMDALL_USED;

/**
 * Returns the object type name.
 *
 * This returns a nul-terminated string identifying the object type.
 * The string is valid for at least as long as the object reference.
 *
 * \param obj object whose type name to get
 */
HEIMDALL_API const char *heimdall_object_typename(const heimdall_object_t *obj) HEIMDALL_USED;

/**
 * Gets the parent of an object.
 *
 * \return the parent object (NULL if none)
 *
 * \note The returned parent object pointer is valid as long as the child is.
 */
HEIMDALL_API heimdall_object_t *heimdall_object_parent(heimdall_object_t *obj) HEIMDALL_USED;
#define heimdall_object_parent(o) heimdall_object_parent(HEIMDALL_OBJECT(o))

static inline struct heimdall_logger *heimdall_object_logger(heimdall_object_t *obj)
{
    return obj->logger;
}
#define heimdall_object_logger(o) heimdall_object_logger(HEIMDALL_OBJECT(o))

 /**
 * Get tracer of a heimdall instance from an object.
 *
 * \return the tracer of a heimdall instance from an object (NULL if none).
 */
HEIMDALL_API struct heimdall_tracer *heimdall_object_get_tracer(heimdall_object_t *obj);

/**
 * Tries to get the name of module bound to an object.
 *
 * \warning This function is intrinsically race-prone, as a module may be
 * bound or unbound asynchronously by another thread.
 * Do not trust the result for any purpose other than debugging/tracing.
 *
 * \return Normally, this returns a heap-allocated nul-terminated string
 * which is the name of the module. If no module are bound to the object, it
 * returns NULL. It also returns NULL on error.
 */
#define heimdall_object_get_name(obj) var_GetString(obj, "module-name")

#define heimdall_object_create(a,b) heimdall_object_create( HEIMDALL_OBJECT(a), b )

#define heimdall_object_find_name(a,b) \
    heimdall_object_find_name( HEIMDALL_OBJECT(a),b)

HEIMDALL_USED
static inline libheimdall_int_t *heimdall_object_instance(heimdall_object_t *obj)
{
    heimdall_object_t *parent;

    do
        parent = obj;
    while ((obj = heimdall_object_parent(obj)) != NULL);

    return (libheimdall_int_t *)parent;
}
#define heimdall_object_instance(o) heimdall_object_instance(HEIMDALL_OBJECT(o))

/* Here for backward compatibility. TODO: Move to <heimdall_vout.h>! */
HEIMDALL_API vout_thread_t *vout_Hold(vout_thread_t *vout);
HEIMDALL_API void vout_Release(vout_thread_t *vout);

/* Here for backward compatibility. TODO: Move to <heimdall_aout.h>! */
HEIMDALL_API audio_output_t *aout_Hold(audio_output_t *aout);
HEIMDALL_API void aout_Release(audio_output_t *aout);

/* TODO: remove heimdall_object_hold/_release() for GUIs, remove this */
HEIMDALL_DEPRECATED static inline void *heimdall_object_hold(heimdall_object_t *o)
{
    const char *tn = heimdall_object_typename(o);

    if (!strcmp(tn, "audio output"))
        aout_Hold((audio_output_t *)o);
    if (!strcmp(tn, "video output"))
        vout_Hold((vout_thread_t *)o);
    return o;
}

static inline void heimdall_object_release(heimdall_object_t *o)
{
    const char *tn = heimdall_object_typename(o);

    if (!strcmp(tn, "audio output"))
        aout_Release((audio_output_t *)o);
    if (!strcmp(tn, "video output"))
        vout_Release((vout_thread_t *)o);
}
#define heimdall_object_release(o) heimdall_object_release(HEIMDALL_OBJECT(o))

/**
 * @defgroup objres Object resources
 *
 * The object resource functions tie resource allocation to an instance of
 * a module through a Heimdall object.
 * Such resource will be automatically freed, in first in last out order,
 * when the module instance associated with the Heimdall object is terminated.
 *
 * Specifically, if the module instance activation/probe function fails, the
 * resource will be freed immediately after the failure. If the activation
 * succeeds, the resource will be freed when the module instance is terminated.
 *
 * This is a convenience mechanism to save explicit clean-up function calls
 * in modules.
 *
 * @{
 */

/**
 * Allocates memory for a module.
 *
 * This function allocates memory from the heap for a module instance.
 * The memory is uninitialized.
 *
 * @param obj Heimdall object to tie the memory allocation to
 * @param size byte size of the memory allocation
 *
 * @return a pointer to the allocated memory, or NULL on error (errno is set).
 */
HEIMDALL_API HEIMDALL_MALLOC void *heimdall_obj_malloc(heimdall_object_t *obj, size_t size);
#define heimdall_obj_malloc(o, s) heimdall_obj_malloc(HEIMDALL_OBJECT(o), s)

/**
 * Allocates a zero-initialized table for a module.
 *
 * This function allocates a table from the heap for a module instance.
 * The memory is initialized to all zeroes.
 *
 * @param obj Heimdall object to tie the memory allocation to
 * @param nmemb number of table entries
 * @param size byte size of a table entry
 *
 * @return a pointer to the allocated memory, or NULL on error (errno is set).
 */
HEIMDALL_API HEIMDALL_MALLOC void *heimdall_obj_calloc(heimdall_object_t *obj, size_t nmemb,
                                        size_t size);
#define heimdall_obj_calloc(o, n, s) heimdall_obj_calloc(HEIMDALL_OBJECT(o), n, s)

/**
 * Duplicates a string for a module.
 *
 * This function allocates a copy of a nul-terminated string for a module
 * instance.
 *
 * @param obj Heimdall object to tie the memory allocation to
 * @param str string to copy
 *
 * @return a pointer to the copy, or NULL on error (errno is set).
 */
HEIMDALL_API HEIMDALL_MALLOC char *heimdall_obj_strdup(heimdall_object_t *obj, const char *str);
#define heimdall_obj_strdup(o, s) heimdall_obj_strdup(HEIMDALL_OBJECT(o), s)

/**
 * Manually frees module memory.
 *
 * This function manually frees a resource allocated with heimdall_obj_malloc(),
 * heimdall_obj_calloc() or heimdall_obj_strdup() before the module instance is
 * terminated. This is seldom necessary.
 *
 * @param obj Heimdall object that the allocation was tied to
 * @param ptr pointer to the allocated resource
 */
HEIMDALL_API void heimdall_obj_free(heimdall_object_t *obj, void *ptr);
#define heimdall_obj_free(o, p) heimdall_obj_free(HEIMDALL_OBJECT(o), p)

#ifdef __cplusplus
} /* extern "C" */

# include <type_traits>

#undef heimdall_object_create

template <typename O> HEIMDALL_MALLOC HEIMDALL_USED
static inline void* heimdall_object_create(O *obj, size_t size)
{
    return heimdall_object_create(HEIMDALL_OBJECT(obj), size);
}

template<typename T, typename O> HEIMDALL_MALLOC HEIMDALL_USED
static inline T* heimdall_object_create(O *obj)
{
    static_assert(std::is_pointer<T>::value == false, "heimdall_object_create can only create objects");
    void *object = heimdall_object_create(HEIMDALL_OBJECT(obj), sizeof(T));
    if (object == nullptr)
        return nullptr;

    return new(object) T;
}

#undef heimdall_object_delete
template<typename O>
static inline void heimdall_object_delete(O *obj)
{
    if (!std::is_trivially_destructible<O>::value)
        obj->~O();
    heimdall_object_delete(HEIMDALL_OBJECT(obj));
}

#endif

/** @} */
/** @} */

#endif
