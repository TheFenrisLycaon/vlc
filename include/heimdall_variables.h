/*****************************************************************************
 * heimdall_variables.h: variables handling
 *****************************************************************************
 * Copyright (C) 2002-2004 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@netcourrier.com>
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

#ifndef HEIMDALL_VARIABLES_H
#define HEIMDALL_VARIABLES_H 1

/**
 * \defgroup variables Variables
 * \ingroup heimdall_object
 *
 * Heimdall object variables and callbacks
 *
 * @{
 * \file
 * Heimdall object variables and callbacks interface
 */

#define HEIMDALL_VAR_TYPE      0x00ff
#define HEIMDALL_VAR_CLASS     0x00f0
#define HEIMDALL_VAR_FLAGS     0xff00

/**
 * \defgroup var_type Variable types
 * These are the different types a heimdall variable can have.
 * @{
 */
#define HEIMDALL_VAR_VOID      0x0010
#define HEIMDALL_VAR_BOOL      0x0020
#define HEIMDALL_VAR_INTEGER   0x0030
#define HEIMDALL_VAR_STRING    0x0040
#define HEIMDALL_VAR_FLOAT     0x0050
#define HEIMDALL_VAR_ADDRESS   0x0070
#define HEIMDALL_VAR_COORDS    0x00A0
/**@}*/

/** \defgroup var_flags Additive flags
 * These flags are added to the type field of the variable. Most as a result of
 * a var_Change() call, but some may be added at creation time
 * @{
 */
#define HEIMDALL_VAR_HASCHOICE 0x0100

#define HEIMDALL_VAR_ISCOMMAND 0x2000

/** Creation flag */
/* If the variable is not found on the current module
   search all parents and finally module config until found */
#define HEIMDALL_VAR_DOINHERIT 0x8000
/**@}*/

/**
 * \defgroup var_action Variable actions
 * These are the different actions that can be used with var_Change().
 * The parameters given are the meaning of the two last parameters of
 * var_Change() when this action is being used.
 * @{
 */

#define HEIMDALL_VAR_SETSTEP             0x0012

/**
 * Set the value of this variable without triggering any callbacks
 * \param p_val The new value
 * \param p_val2 Unused
 */
#define HEIMDALL_VAR_SETVALUE            0x0013

#define HEIMDALL_VAR_SETTEXT             0x0014
#define HEIMDALL_VAR_GETTEXT             0x0015

#define HEIMDALL_VAR_GETMIN              0x0016
#define HEIMDALL_VAR_GETMAX              0x0017
#define HEIMDALL_VAR_GETSTEP             0x0018

#define HEIMDALL_VAR_ADDCHOICE           0x0020
#define HEIMDALL_VAR_DELCHOICE           0x0021
#define HEIMDALL_VAR_CLEARCHOICES        0x0022
#define HEIMDALL_VAR_GETCHOICES          0x0024

#define HEIMDALL_VAR_CHOICESCOUNT        0x0026
#define HEIMDALL_VAR_SETMINMAX           0x0027

/**@}*/

/**
 * Variable actions.
 *
 * These are the different actions that can be used with var_GetAndSet().
 */
enum heimdall_var_atomic_op {
    HEIMDALL_VAR_BOOL_TOGGLE, /**< Invert a boolean value (param ignored) */
    HEIMDALL_VAR_INTEGER_ADD, /**< Add parameter to an integer value */
    HEIMDALL_VAR_INTEGER_OR,  /**< Binary OR over an integer bits field */
    HEIMDALL_VAR_INTEGER_NAND,/**< Binary NAND over an integer bits field */
};

/**
 * Heimdall value structure
 */
typedef union
{
    int64_t         i_int;
    bool            b_bool;
    float           f_float;
    char *          psz_string;
    void *          p_address;
    struct { int32_t x; int32_t y; } coords;

} heimdall_value_t;

/*****************************************************************************
 * Variable callbacks: called when the value is modified
 *****************************************************************************/
typedef int ( * heimdall_callback_t ) ( heimdall_object_t *,      /* variable's object */
                                   char const *,            /* variable name */
                                   heimdall_value_t,                 /* old value */
                                   heimdall_value_t,                 /* new value */
                                   void * );                /* callback data */

/*****************************************************************************
 * List callbacks: called when elements are added/removed from the list
 *****************************************************************************/
typedef int ( * heimdall_list_callback_t ) ( heimdall_object_t *,      /* variable's object */
                                        char const *,            /* variable name */
                                        int,                  /* HEIMDALL_VAR_* action */
                                        heimdall_value_t *,      /* new/deleted value  */
                                        void *);                 /* callback data */

/**
 * Creates a Heimdall object variable.
 *
 * This function creates a named variable within a Heimdall object.
 * If a variable already exists with the same name within the same object, its
 * reference count is incremented instead.
 *
 * \param obj Object to hold the variable
 * \param name Variable name
 * \param type Variable type. Must be one of \ref var_type combined with
 *               zero or more \ref var_flags
 */
HEIMDALL_API int var_Create(heimdall_object_t *obj, const char *name, int type);

/**
 * Destroys a Heimdall object variable.
 *
 * This function decrements the reference count of a named variable within a
 * Heimdall object. If the reference count reaches zero, the variable is destroyed.
 *
 * \param obj Object holding the variable
 * \param name Variable name
 */
HEIMDALL_API void var_Destroy(heimdall_object_t *obj, const char *name);

/**
 * Performs a special action on a variable.
 *
 * \param obj Object holding the variable
 * \param name Variable name
 * \param action Action to perform. Must be one of \ref var_action
 */
HEIMDALL_API int var_Change(heimdall_object_t *obj, const char *name, int action, ...);

/**
 * Get the type of a variable.
 *
 * \see var_type
 *
 * \return The variable type if it exists
 *         or 0 if the variable could not be found.
 */
HEIMDALL_API int var_Type(heimdall_object_t *obj, const char *name) HEIMDALL_USED;

/**
 * Sets a variable value.
 *
 * \param obj Object holding the variable
 * \param name Variable name
 * \param val Variable value to set
 */
HEIMDALL_API int var_Set(heimdall_object_t *obj, const char *name, heimdall_value_t val);

/**
 * Gets a variable value.
 *
 * \param obj Object holding the variable
 * \param name Variable name
 * \param valp Pointer to a \ref heimdall_value_t object to hold the value [OUT]
 */
HEIMDALL_API int var_Get(heimdall_object_t *obj, const char *name, heimdall_value_t *valp);

HEIMDALL_API int var_SetChecked( heimdall_object_t *, const char *, int, heimdall_value_t );
HEIMDALL_API int var_GetChecked( heimdall_object_t *, const char *, int, heimdall_value_t * );

/**
 * Perform an atomic read-modify-write of a variable.
 *
 * \param obj object holding the variable
 * \param name variable name
 * \param op read-modify-write operation to perform
 *           (see \ref heimdall_var_atomic_op)
 * \param value value of the variable after the modification
 * \retval HEIMDALL_SUCCESS Operation successful
 * \retval HEIMDALL_ENOENT Variable not found
 *
 * \bug The modified value is returned rather than the original value.
 * As such, the original value cannot be known in the case of non-reversible
 * operation such as \ref HEIMDALL_VAR_INTEGER_OR and \ref HEIMDALL_VAR_INTEGER_NAND.
 */
HEIMDALL_API int var_GetAndSet(heimdall_object_t *obj, const char *name, int op,
                          heimdall_value_t *value);

/**
 * Finds the value of a variable.
 *
 * If the specified object does not hold a variable with the specified name,
 * try the parent object, and iterate until the top of the objects tree. If no
 * match is found, the value is read from the configuration.
 */
HEIMDALL_API int var_Inherit( heimdall_object_t *, const char *, int, heimdall_value_t * );


/*****************************************************************************
 * Variable callbacks
 *****************************************************************************
 * int MyCallback( heimdall_object_t *p_this,
 *                 char const *psz_variable,
 *                 heimdall_value_t oldvalue,
 *                 heimdall_value_t newvalue,
 *                 void *p_data);
 *****************************************************************************/

/**
 * Registers a callback for a variable.
 *
 * We store a function pointer that will be called upon variable
 * modification.
 *
 * \param obj Object holding the variable
 * \param name Variable name
 * \param callback Callback function pointer
 * \param opaque Opaque data pointer for use by the callback.
 *
 * \warning The callback function is run in the thread that calls var_Set() on
 *          the variable. Use proper locking. This thread may not have much
 *          time to spare, so keep callback functions short.
 *
 * \bug It is not possible to atomically retrieve the current value and
 * register a callback. As a consequence, extreme care must be taken to ensure
 * that the variable value cannot change before the callback is registered.
 * Failure to do so will result in intractable race conditions.
 */
HEIMDALL_API void var_AddCallback(heimdall_object_t *obj, const char *name,
                             heimdall_callback_t callback, void *opaque);

/**
 * Deregisters a callback from a variable.
 *
 * The callback and opaque pointer must be supplied again, as the same callback
 * function might have been registered more than once.
 */
HEIMDALL_API void var_DelCallback(heimdall_object_t *obj, const char *name,
                             heimdall_callback_t callback, void *opaque);

/**
 * Triggers callbacks on a variable.
 *
 * This triggers any callbacks registered on the named variable without
 * actually modifying the variable value. This is primarily useful for
 * variables with \ref HEIMDALL_VAR_VOID type (which do not have a value).
 *
 * \param obj Object holding the variable
 * \param name Variable name
 */
HEIMDALL_API void var_TriggerCallback(heimdall_object_t *obj, const char *name);

/**
 * Register a callback for a list variable
 *
 * The callback is triggered when an element is added/removed from the
 * list or when the list is cleared.
 *
 * See var_AddCallback().
 */
HEIMDALL_API void var_AddListCallback( heimdall_object_t *, const char *, heimdall_list_callback_t, void * );

/**
 * Remove a callback from a list variable
 *
 * See var_DelCallback().
 */
HEIMDALL_API void var_DelListCallback( heimdall_object_t *, const char *, heimdall_list_callback_t, void * );

/*****************************************************************************
 * helpers functions
 *****************************************************************************/

/**
 * Set the value of an integer variable
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 * \param i The new integer value of this variable
 */
static inline int var_SetInteger( heimdall_object_t *p_obj, const char *psz_name,
                                  int64_t i )
{
    heimdall_value_t val;
    val.i_int = i;
    return var_SetChecked( p_obj, psz_name, HEIMDALL_VAR_INTEGER, val );
}

/**
 * Set the value of an boolean variable
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 * \param b The new boolean value of this variable
 */
static inline int var_SetBool( heimdall_object_t *p_obj, const char *psz_name, bool b )
{
    heimdall_value_t val;
    val.b_bool = b;
    return var_SetChecked( p_obj, psz_name, HEIMDALL_VAR_BOOL, val );
}

static inline int var_SetCoords( heimdall_object_t *obj, const char *name,
                                 int32_t x, int32_t y )
{
    heimdall_value_t val;
    val.coords.x = x;
    val.coords.y = y;
    return var_SetChecked (obj, name, HEIMDALL_VAR_COORDS, val);
}

/**
 * Set the value of a float variable
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 * \param f The new float value of this variable
 */
static inline int var_SetFloat( heimdall_object_t *p_obj, const char *psz_name, float f )
{
    heimdall_value_t val;
    val.f_float = f;
    return var_SetChecked( p_obj, psz_name, HEIMDALL_VAR_FLOAT, val );
}

/**
 * Set the value of a string variable
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 * \param psz_string The new string value of this variable
 */
static inline int var_SetString( heimdall_object_t *p_obj, const char *psz_name, const char *psz_string )
{
    heimdall_value_t val;
    val.psz_string = (char *)psz_string;
    return var_SetChecked( p_obj, psz_name, HEIMDALL_VAR_STRING, val );
}

/**
 * Set the value of a pointer variable
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 * \param ptr The new pointer value of this variable
 */
static inline
int var_SetAddress( heimdall_object_t *p_obj, const char *psz_name, void *ptr )
{
    heimdall_value_t val;
    val.p_address = ptr;
    return var_SetChecked( p_obj, psz_name, HEIMDALL_VAR_ADDRESS, val );
}

/**
 * Get an integer value
*
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline int64_t var_GetInteger( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val;
    if( !var_GetChecked( p_obj, psz_name, HEIMDALL_VAR_INTEGER, &val ) )
        return val.i_int;
    else
        return 0;
}

/**
 * Get a boolean value
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline bool var_GetBool( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val; val.b_bool = false;

    if( !var_GetChecked( p_obj, psz_name, HEIMDALL_VAR_BOOL, &val ) )
        return val.b_bool;
    else
        return false;
}

static inline void var_GetCoords( heimdall_object_t *obj, const char *name,
                                  int32_t *px, int32_t *py )
{
    heimdall_value_t val;

    if (likely(!var_GetChecked (obj, name, HEIMDALL_VAR_COORDS, &val)))
    {
        *px = val.coords.x;
        *py = val.coords.y;
    }
    else
        *px = *py = 0;
}

/**
 * Get a float value
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline float var_GetFloat( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val; val.f_float = 0.0;
    if( !var_GetChecked( p_obj, psz_name, HEIMDALL_VAR_FLOAT, &val ) )
        return val.f_float;
    else
        return 0.0;
}

/**
 * Get a string value
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED HEIMDALL_MALLOC
static inline char *var_GetString( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val; val.psz_string = NULL;
    if( var_GetChecked( p_obj, psz_name, HEIMDALL_VAR_STRING, &val ) )
        return NULL;
    else
        return val.psz_string;
}

HEIMDALL_USED HEIMDALL_MALLOC
static inline char *var_GetNonEmptyString( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val;
    if( var_GetChecked( p_obj, psz_name, HEIMDALL_VAR_STRING, &val ) )
        return NULL;
    if( val.psz_string && *val.psz_string )
        return val.psz_string;
    free( val.psz_string );
    return NULL;
}

HEIMDALL_USED
static inline void *var_GetAddress( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val;
    if( var_GetChecked( p_obj, psz_name, HEIMDALL_VAR_ADDRESS, &val ) )
        return NULL;
    else
        return val.p_address;
}

/**
 * Increment an integer variable
 * \param p_obj the object that holds the variable
 * \param psz_name the name of the variable
 */
static inline int64_t var_IncInteger( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val;
    val.i_int = 1;
    if( var_GetAndSet( p_obj, psz_name, HEIMDALL_VAR_INTEGER_ADD, &val ) )
        return 0;
    return val.i_int;
}

/**
 * Decrement an integer variable
 * \param p_obj the object that holds the variable
 * \param psz_name the name of the variable
 */
static inline int64_t var_DecInteger( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val;
    val.i_int = -1;
    if( var_GetAndSet( p_obj, psz_name, HEIMDALL_VAR_INTEGER_ADD, &val ) )
        return 0;
    return val.i_int;
}

static inline uint64_t var_OrInteger( heimdall_object_t *obj, const char *name,
                                      unsigned v )
{
    heimdall_value_t val;
    val.i_int = v;
    if( var_GetAndSet( obj, name, HEIMDALL_VAR_INTEGER_OR, &val ) )
        return 0;
    return val.i_int;
}

static inline uint64_t var_NAndInteger( heimdall_object_t *obj, const char *name,
                                        unsigned v )
{
    heimdall_value_t val;
    val.i_int = v;
    if( var_GetAndSet( obj, name, HEIMDALL_VAR_INTEGER_NAND, &val ) )
        return 0;
    return val.i_int;
}

/**
 * Create a integer variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline int64_t var_CreateGetInteger( heimdall_object_t *p_obj, const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );
    return var_GetInteger( p_obj, psz_name );
}

/**
 * Create a boolean variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline bool var_CreateGetBool( heimdall_object_t *p_obj, const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT );
    return var_GetBool( p_obj, psz_name );
}

/**
 * Create a float variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline float var_CreateGetFloat( heimdall_object_t *p_obj, const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_FLOAT | HEIMDALL_VAR_DOINHERIT );
    return var_GetFloat( p_obj, psz_name );
}

/**
 * Create a string variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED HEIMDALL_MALLOC
static inline char *var_CreateGetString( heimdall_object_t *p_obj,
                                           const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT );
    return var_GetString( p_obj, psz_name );
}

HEIMDALL_USED HEIMDALL_MALLOC
static inline char *var_CreateGetNonEmptyString( heimdall_object_t *p_obj,
                                                   const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT );
    return var_GetNonEmptyString( p_obj, psz_name );
}

/**
 * Create an address variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline void *var_CreateGetAddress( heimdall_object_t *p_obj,
                                           const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_ADDRESS | HEIMDALL_VAR_DOINHERIT );
    return var_GetAddress( p_obj, psz_name );
}

/**
 * Create a integer command variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline int64_t var_CreateGetIntegerCommand( heimdall_object_t *p_obj, const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT
                                   | HEIMDALL_VAR_ISCOMMAND );
    return var_GetInteger( p_obj, psz_name );
}

/**
 * Create a boolean command variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline bool var_CreateGetBoolCommand( heimdall_object_t *p_obj, const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_BOOL | HEIMDALL_VAR_DOINHERIT
                                   | HEIMDALL_VAR_ISCOMMAND );
    return var_GetBool( p_obj, psz_name );
}

/**
 * Create a float command variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED
static inline float var_CreateGetFloatCommand( heimdall_object_t *p_obj, const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_FLOAT | HEIMDALL_VAR_DOINHERIT
                                   | HEIMDALL_VAR_ISCOMMAND );
    return var_GetFloat( p_obj, psz_name );
}

/**
 * Create a string command variable with inherit and get its value.
 *
 * \param p_obj The object that holds the variable
 * \param psz_name The name of the variable
 */
HEIMDALL_USED HEIMDALL_MALLOC
static inline char *var_CreateGetStringCommand( heimdall_object_t *p_obj,
                                           const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT
                                   | HEIMDALL_VAR_ISCOMMAND );
    return var_GetString( p_obj, psz_name );
}

HEIMDALL_USED HEIMDALL_MALLOC
static inline char *var_CreateGetNonEmptyStringCommand( heimdall_object_t *p_obj,
                                                   const char *psz_name )
{
    var_Create( p_obj, psz_name, HEIMDALL_VAR_STRING | HEIMDALL_VAR_DOINHERIT
                                   | HEIMDALL_VAR_ISCOMMAND );
    return var_GetNonEmptyString( p_obj, psz_name );
}

HEIMDALL_USED
static inline size_t var_CountChoices( heimdall_object_t *p_obj, const char *psz_name )
{
    size_t count;
    if( var_Change( p_obj, psz_name, HEIMDALL_VAR_CHOICESCOUNT, &count ) )
        return 0;
    return count;
}

static inline bool var_ToggleBool( heimdall_object_t *p_obj, const char *psz_name )
{
    heimdall_value_t val;
    if( var_GetAndSet( p_obj, psz_name, HEIMDALL_VAR_BOOL_TOGGLE, &val ) )
        return false;
    return val.b_bool;
}

HEIMDALL_USED
static inline bool var_InheritBool( heimdall_object_t *obj, const char *name )
{
    heimdall_value_t val;

    if( var_Inherit( obj, name, HEIMDALL_VAR_BOOL, &val ) )
        val.b_bool = false;
    return val.b_bool;
}

HEIMDALL_USED
static inline int64_t var_InheritInteger( heimdall_object_t *obj, const char *name )
{
    heimdall_value_t val;

    if( var_Inherit( obj, name, HEIMDALL_VAR_INTEGER, &val ) )
        val.i_int = 0;
    return val.i_int;
}

HEIMDALL_USED
static inline float var_InheritFloat( heimdall_object_t *obj, const char *name )
{
    heimdall_value_t val;

    if( var_Inherit( obj, name, HEIMDALL_VAR_FLOAT, &val ) )
        val.f_float = 0.;
    return val.f_float;
}

HEIMDALL_USED HEIMDALL_MALLOC
static inline char *var_InheritString( heimdall_object_t *obj, const char *name )
{
    heimdall_value_t val;

    if( var_Inherit( obj, name, HEIMDALL_VAR_STRING, &val ) )
        val.psz_string = NULL;
    else if( val.psz_string && !*val.psz_string )
    {
        free( val.psz_string );
        val.psz_string = NULL;
    }
    return val.psz_string;
}

HEIMDALL_USED
static inline void *var_InheritAddress( heimdall_object_t *obj, const char *name )
{
    heimdall_value_t val;

    if( var_Inherit( obj, name, HEIMDALL_VAR_ADDRESS, &val ) )
        val.p_address = NULL;
    return val.p_address;
}


/**
 * Inherit a string as a fractional value.
 *
 * This function inherits a string, and interprets it as an unsigned rational
 * number, i.e. a fraction. It also accepts a normally formatted floating point
 * number.
 *
 * \warning The caller shall perform any and all necessary boundary checks.
 *
 * \note The rational number is always reduced,
 * i.e. the returned numerator and denominator are always co-prime numbers.
 *
 * \note Fraction with zero as denominator are considered valid,
 * including the undefined form zero-by-zero.
 *
 * \return Zero on success, an error if parsing fails.
  */
HEIMDALL_API int var_InheritURational(heimdall_object_t *obj, unsigned *num,
                                 unsigned *den, const char *name);

/**
 * Parses a string with multiple options.
 *
 * Parses a set of colon-separated or semicolon-separated
 * <code>name=value</code> pairs.
 * Some access (or access_demux) plugins uses this scheme
 * in media resource location.
 * @note Only trusted/safe variables are allowed. This is intended.
 *
 * @warning Only use this for plugins implementing Heimdall-specific resource
 * location schemes. This would not make any sense for standardized ones.
 *
 * @param obj Heimdall object on which to set variables (and emit error messages)
 * @param mrl string to parse
 * @param prefix prefix to prepend to option names in the string
 *
 * @return HEIMDALL_ENOMEM on error, HEIMDALL_SUCCESS on success.
 */
HEIMDALL_API int var_LocationParse(heimdall_object_t *obj, const char *mrl, const char *prefix);

#ifndef DOC
#define var_Create(a,b,c) var_Create(HEIMDALL_OBJECT(a), b, c)
#define var_Destroy(a,b) var_Destroy(HEIMDALL_OBJECT(a), b)
#define var_Change(a,b,...) var_Change(HEIMDALL_OBJECT(a), b, __VA_ARGS__)
#define var_Type(a,b) var_Type(HEIMDALL_OBJECT(a), b)
#define var_Set(a,b,c) var_Set(HEIMDALL_OBJECT(a), b, c)
#define var_Get(a,b,c) var_Get(HEIMDALL_OBJECT(a), b, c)
#define var_SetChecked(o,n,t,v) var_SetChecked(HEIMDALL_OBJECT(o), n, t, v)
#define var_GetChecked(o,n,t,v) var_GetChecked(HEIMDALL_OBJECT(o), n, t, v)

#define var_AddCallback(a,b,c,d) var_AddCallback(HEIMDALL_OBJECT(a), b, c, d)
#define var_DelCallback(a,b,c,d) var_DelCallback(HEIMDALL_OBJECT(a), b, c, d)
#define var_TriggerCallback(a,b) var_TriggerCallback(HEIMDALL_OBJECT(a), b)
#define var_AddListCallback(a,b,c,d) \
        var_AddListCallback(HEIMDALL_OBJECT(a), b, c, d)
#define var_DelListCallback(a,b,c,d) \
        var_DelListCallback(HEIMDALL_OBJECT(a), b, c, d)

#define var_SetInteger(a,b,c) var_SetInteger(HEIMDALL_OBJECT(a), b, c)
#define var_SetBool(a,b,c) var_SetBool(HEIMDALL_OBJECT(a), b, c)
#define var_SetCoords(o,n,x,y) var_SetCoords(HEIMDALL_OBJECT(o), n, x, y)
#define var_SetFloat(a,b,c) var_SetFloat(HEIMDALL_OBJECT(a), b, c)
#define var_SetString(a,b,c) var_SetString(HEIMDALL_OBJECT(a), b, c)
#define var_SetAddress(o, n, p) var_SetAddress(HEIMDALL_OBJECT(o), n, p)

#define var_GetCoords(o,n,x,y) var_GetCoords(HEIMDALL_OBJECT(o), n, x, y)

#define var_IncInteger(a,b) var_IncInteger(HEIMDALL_OBJECT(a), b)
#define var_DecInteger(a,b) var_DecInteger(HEIMDALL_OBJECT(a), b)
#define var_OrInteger(a,b,c) var_OrInteger(HEIMDALL_OBJECT(a), b, c)
#define var_NAndInteger(a,b,c) var_NAndInteger(HEIMDALL_OBJECT(a), b, c)

#define var_CreateGetInteger(a,b) var_CreateGetInteger(HEIMDALL_OBJECT(a), b)
#define var_CreateGetBool(a,b) var_CreateGetBool(HEIMDALL_OBJECT(a), b)
#define var_CreateGetFloat(a,b) var_CreateGetFloat(HEIMDALL_OBJECT(a), b)
#define var_CreateGetString(a,b) var_CreateGetString(HEIMDALL_OBJECT(a), b)
#define var_CreateGetNonEmptyString(a,b) \
        var_CreateGetNonEmptyString(HEIMDALL_OBJECT(a), b)
#define var_CreateGetAddress(a,b) var_CreateGetAddress( HEIMDALL_OBJECT(a), b)

#define var_CreateGetIntegerCommand(a,b)   var_CreateGetIntegerCommand( HEIMDALL_OBJECT(a),b)
#define var_CreateGetBoolCommand(a,b)   var_CreateGetBoolCommand( HEIMDALL_OBJECT(a),b)
#define var_CreateGetFloatCommand(a,b)   var_CreateGetFloatCommand( HEIMDALL_OBJECT(a),b)
#define var_CreateGetStringCommand(a,b)   var_CreateGetStringCommand( HEIMDALL_OBJECT(a),b)
#define var_CreateGetNonEmptyStringCommand(a,b)   var_CreateGetNonEmptyStringCommand( HEIMDALL_OBJECT(a),b)

#define var_CountChoices(a,b) var_CountChoices(HEIMDALL_OBJECT(a),b)
#define var_ToggleBool(a,b) var_ToggleBool(HEIMDALL_OBJECT(a),b )

#define var_InheritBool(o, n) var_InheritBool(HEIMDALL_OBJECT(o), n)
#define var_InheritInteger(o, n) var_InheritInteger(HEIMDALL_OBJECT(o), n)
#define var_InheritFloat(o, n) var_InheritFloat(HEIMDALL_OBJECT(o), n)
#define var_InheritString(o, n) var_InheritString(HEIMDALL_OBJECT(o), n)
#define var_InheritAddress(o, n) var_InheritAddress(HEIMDALL_OBJECT(o), n)
#define var_InheritURational(a,b,c,d) var_InheritURational(HEIMDALL_OBJECT(a), b, c, d)

#define var_GetInteger(a,b) var_GetInteger(HEIMDALL_OBJECT(a),b)
#define var_GetBool(a,b) var_GetBool(HEIMDALL_OBJECT(a),b)
#define var_GetFloat(a,b) var_GetFloat(HEIMDALL_OBJECT(a),b)
#define var_GetString(a,b) var_GetString(HEIMDALL_OBJECT(a),b)
#define var_GetNonEmptyString(a,b) var_GetNonEmptyString( HEIMDALL_OBJECT(a),b)
#define var_GetAddress(a,b) var_GetAddress(HEIMDALL_OBJECT(a),b)

#define var_LocationParse(o, m, p) var_LocationParse(HEIMDALL_OBJECT(o), m, p)
#endif

/**
 * @}
 */
#endif /*  _HEIMDALL_VARIABLES_H */
