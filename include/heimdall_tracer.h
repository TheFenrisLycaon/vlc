/*****************************************************************************
 * heimdall_tracer.h: tracing interface
 * This library provides basic functions for threads to interact with user
 * interface, such as trace output.
 *****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
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

#ifndef HEIMDALL_TRACES_H
#define HEIMDALL_TRACES_H

#include <stdarg.h>

#include <heimdall_common.h>
#include <heimdall_threads.h>

/**
 * \defgroup tracer Tracer module and API
 * \ingroup os
 *
 * Functions for modules to emit traces.
 *
 * @{
 * \file
 * Tracing functions
 *
 * \defgroup tracer_module Tracer Module implementation
 * \ingroup tracer
 *
 * @{
 */

/**
 * Trace message values
 */
enum heimdall_tracer_value
{
    HEIMDALL_TRACER_INT,
    HEIMDALL_TRACER_DOUBLE,
    HEIMDALL_TRACER_STRING,
    HEIMDALL_TRACER_UINT,
};

typedef union
{
    int64_t integer;
    double double_;
    const char *string;
    uint64_t uinteger;
} heimdall_tracer_value_t;

/**
 * Trace message
 */
struct heimdall_tracer_entry
{
    const char *key;            /**< Key to identify the value */
    heimdall_tracer_value_t value;   /**< Trace value */
    enum heimdall_tracer_value type; /**< Type of the value */
};

struct heimdall_tracer;

/**
 * Trace record containing the key-values from the trace.
 */
struct heimdall_tracer_trace
{
    /**
     * Defines the list of key-value in the trace. The entries must link
     * towards an array terminated by a HEIMDALL_TRACE_END entry, which remains
     * valid as long as the trace itself should remain valid.
     **/
    const struct heimdall_tracer_entry *entries;
};


/**
 * Tracer operations returned by the module probe function
 */
struct heimdall_tracer_operations
{
    /**
     * Called when tracing data
     *
     * \param sys data pointer set by heimdall_tracer_open_cb()
     * \param ts timestamp of the trace (based on heimdall_tick_now())
     * \param entries can only be \ref heimdall_tracer_entry and the va-args list
     * should be ended by a \ref heimdall_tracer_entry with a NULL key.
     */
    void (*trace)(void *sys, heimdall_tick_t ts, const struct heimdall_tracer_trace *trace);

    /**
     * Called to clean module specific resources
     *
     * \param sys data pointer set by heimdall_tracer_open_cb()
     */
    void (*destroy)(void *sys);
};

/**
 * Module probe/open function signature
 *
 * \param obj a valid object
 * \param[out] sysp to module specific data
 * \return the operations implemented by the module or NULL in case of error
 * */
typedef struct heimdall_tracer_operations *(*heimdall_tracer_open_cb)(heimdall_object_t *obj,
                                                            void **restrict sysp);

/**
 * @}
 *
 * \defgroup tracer_api Tracer API
 * \ingroup tracer
 *
 * @{
 */

/**
 * Create a tracer object
 *
 * \note This function is for advanced debugging/testing.
 * Use heimdall_object_get_tracer() to get the existing tracer.
 *
 * \param parent parent object used to create the tracer
 * \param name module to load or NULL for the default one
 * \return a valid tracer or NULL in case of error
 */
HEIMDALL_API struct heimdall_tracer *heimdall_tracer_Create(heimdall_object_t *parent,
                                             const char *name);

/**
 * Destroy a tracer object
 */
HEIMDALL_API void heimdall_tracer_Destroy(struct heimdall_tracer *tracer);

/**
 * Emit traces
 *
 * \param tracer tracer emitting the traces
 * \param ts timestamp of the current trace
 * \param trace the trace to register with its list of key / value parameters.
 *              Key must be a not NULL string. Value has to be defined with
 *              one of the type defined in the \ref heimdall_tracer_entry union.
 */

HEIMDALL_API void heimdall_tracer_TraceWithTs(struct heimdall_tracer *tracer, heimdall_tick_t ts,
                                    const struct heimdall_tracer_trace *trace);
#define heimdall_tracer_TraceWithTs(tracer, ts, ...) \
    (heimdall_tracer_TraceWithTs)(tracer, ts, &(const struct heimdall_tracer_trace) { \
        .entries = (const struct heimdall_tracer_entry[]) { \
            __VA_ARGS__ \
        } \
    })

#define heimdall_tracer_Trace(tracer, ...) \
    heimdall_tracer_TraceWithTs(tracer, heimdall_tick_now(), __VA_ARGS__)

static inline struct heimdall_tracer_entry heimdall_tracer_entry_FromInt(const char *key, int64_t value)
{
    heimdall_tracer_value_t tracer_value;
    tracer_value.integer = value;
    struct heimdall_tracer_entry trace = { key, tracer_value, HEIMDALL_TRACER_INT };
    return trace;
}

static inline struct heimdall_tracer_entry heimdall_tracer_entry_FromUnsigned(const char *key, uint64_t value)
{
    heimdall_tracer_value_t tracer_value;
    tracer_value.uinteger = value;
    struct heimdall_tracer_entry trace = { key, tracer_value, HEIMDALL_TRACER_UINT };
    return trace;
}

static inline struct heimdall_tracer_entry heimdall_tracer_entry_FromDouble(const char *key, double value)
{
    heimdall_tracer_value_t tracer_value = {
        .double_ = value,
    };
    struct heimdall_tracer_entry trace = { key, tracer_value, HEIMDALL_TRACER_DOUBLE };
    return trace;
}

static inline struct heimdall_tracer_entry heimdall_tracer_entry_FromString(const char *key, const char *value)
{
    heimdall_tracer_value_t tracer_value;
    tracer_value.string = value;
    struct heimdall_tracer_entry trace = { key, tracer_value, HEIMDALL_TRACER_STRING };
    return trace;
}

#ifndef __cplusplus
#define HEIMDALL_TRACE_END \
        heimdall_tracer_entry_FromString(NULL, NULL)

#define HEIMDALL_TRACE(key, value) \
        _Generic((value), \
        uint64_t: heimdall_tracer_entry_FromUnsigned, \
        int64_t: heimdall_tracer_entry_FromInt, \
        double: heimdall_tracer_entry_FromDouble, \
        char *: heimdall_tracer_entry_FromString, \
        const char *: heimdall_tracer_entry_FromString) (key, value)
#else
#define HEIMDALL_TRACE_END \
        heimdall_tracer_entry_FromString(nullptr, nullptr)

static inline struct heimdall_tracer_entry HEIMDALL_TRACE(const char *key, int64_t value)
{
    return heimdall_tracer_entry_FromInt(key, value);
}

static inline struct heimdall_tracer_entry HEIMDALL_TRACE(const char *key, uint64_t value)
{
    return heimdall_tracer_entry_FromUnsigned(key, value);
}

static inline struct heimdall_tracer_entry HEIMDALL_TRACE(const char *key, char *value)
{
    return heimdall_tracer_entry_FromString(key, value);
}

static inline struct heimdall_tracer_entry HEIMDALL_TRACE(const char *key, const char *value)
{
    return heimdall_tracer_entry_FromString(key, value);
}
#endif

#define HEIMDALL_TRACE_TICK_NS(key, tick) HEIMDALL_TRACE((key), NS_FROM_HEIMDALL_TICK((tick)))

#ifdef __cplusplus
#undef heimdall_tracer_TraceWithTs
#undef heimdall_tracer_Trace

template<typename... Entries>
static inline void heimdall_tracer_TraceWithTs(struct heimdall_tracer *tracer,
                                          heimdall_tick_t ts,
                                          struct heimdall_tracer_entry first,
                                          Entries... rest)
{
    const struct heimdall_tracer_entry arr[] = { first, rest... };
    const struct heimdall_tracer_trace trace = { arr };
    heimdall_tracer_TraceWithTs(tracer, ts, &trace);
}

template<typename... Entries>
static inline void heimdall_tracer_Trace(struct heimdall_tracer *tracer,
                                    struct heimdall_tracer_entry first,
                                    Entries... rest)
{
    heimdall_tracer_TraceWithTs(tracer, heimdall_tick_now(), first, rest...);
}
#endif

/**
 * @}
 *
 * \defgroup tracer_helper Tracer helper functions
 * \ingroup tracer
 *
 * @{
 */

static inline void heimdall_tracer_TraceStreamPTS(struct heimdall_tracer *tracer, const char *type,
                                const char *id, const char* stream,
                                heimdall_tick_t pts)
{
    heimdall_tracer_Trace(tracer, HEIMDALL_TRACE("type", type),
                             HEIMDALL_TRACE("id", id),
                             HEIMDALL_TRACE("stream", stream),
                             HEIMDALL_TRACE_TICK_NS("pts", pts),
                             HEIMDALL_TRACE_END);
}

static inline void heimdall_tracer_TraceStreamDTS(struct heimdall_tracer *tracer, const char *type,
                                const char *id, const char* stream,
                                heimdall_tick_t pts, heimdall_tick_t dts)
{
    heimdall_tracer_Trace(tracer, HEIMDALL_TRACE("type", type),
                             HEIMDALL_TRACE("id", id),
                             HEIMDALL_TRACE("stream", stream),
                             HEIMDALL_TRACE_TICK_NS("pts", pts),
                             HEIMDALL_TRACE_TICK_NS("dts", dts),
                             HEIMDALL_TRACE_END);
}

static inline void heimdall_tracer_TraceEvent(struct heimdall_tracer *tracer, const char *type,
                                         const char *id, const char *event)
{
    heimdall_tracer_Trace(tracer, HEIMDALL_TRACE("type", type),
                             HEIMDALL_TRACE("id", id),
                             HEIMDALL_TRACE("event", event),
                             HEIMDALL_TRACE_END);
}

static inline void heimdall_tracer_TracePCR( struct heimdall_tracer *tracer, const char *type,
                                    const char *id, heimdall_tick_t pcr)
{
    heimdall_tracer_Trace(tracer, HEIMDALL_TRACE("type", type),
                             HEIMDALL_TRACE("id", id),
                             HEIMDALL_TRACE_TICK_NS("pcr", pcr),
                             HEIMDALL_TRACE_END);
}

/**
 * @}
 */
/**
 * @}
 */
#endif
