/*****************************************************************************
 * variables.c: test for variables
 *****************************************************************************
 * Copyright (C) 2009 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Rémi Duraffort <ivoire@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include <limits.h>

#include "../../libheimdall/test.h"
#include "../lib/libheimdall_internal.h"

static const char *psz_var_name[] = {
    "a", "abcdef", "abcdefg", "abc123", "abc-123", "é€!!"
};
#define VAR_COUNT (ARRAY_SIZE(psz_var_name))
static heimdall_value_t var_value[VAR_COUNT];

static void test_integer( libheimdall_int_t *p_libheimdall )
{
    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Create( p_libheimdall, psz_var_name[i], HEIMDALL_VAR_INTEGER );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        var_value[i].i_int = rand();
        var_SetInteger( p_libheimdall, psz_var_name[i], var_value[i].i_int );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        assert( var_GetInteger( p_libheimdall, psz_var_name[i] ) == var_value[i].i_int );
        var_IncInteger( p_libheimdall, psz_var_name[i] );
        assert( var_GetInteger( p_libheimdall, psz_var_name[i] ) == var_value[i].i_int + 1 );
        var_DecInteger( p_libheimdall, psz_var_name[i] );
        assert( var_GetInteger( p_libheimdall, psz_var_name[i] ) == var_value[i].i_int );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Destroy( p_libheimdall, psz_var_name[i] );
}

static void test_booleans( libheimdall_int_t *p_libheimdall )
{
    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Create( p_libheimdall, psz_var_name[i], HEIMDALL_VAR_BOOL );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        var_value[i].b_bool = (rand() > RAND_MAX/2);
        var_SetBool( p_libheimdall, psz_var_name[i], var_value[i].b_bool );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        assert( var_GetBool( p_libheimdall, psz_var_name[i] ) == var_value[i].b_bool );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_ToggleBool( p_libheimdall, psz_var_name[i] );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        assert( var_GetBool( p_libheimdall, psz_var_name[i] ) != var_value[i].b_bool );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Destroy( p_libheimdall, psz_var_name[i] );
}

static void test_floats( libheimdall_int_t *p_libheimdall )
{
    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Create( p_libheimdall, psz_var_name[i], HEIMDALL_VAR_FLOAT );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        var_value[i].f_float = rand();
        var_SetFloat( p_libheimdall, psz_var_name[i], var_value[i].f_float );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        assert( var_GetFloat( p_libheimdall, psz_var_name[i] ) == var_value[i].f_float );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Destroy( p_libheimdall, psz_var_name[i] );
}

static void test_fracts( libheimdall_int_t *p_libheimdall )
{
    const char *name = psz_var_name[0];
    unsigned num, den;

    var_Create( p_libheimdall, name, HEIMDALL_VAR_STRING );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) != 0 );

    var_SetString( p_libheimdall, name, "123garbage" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) != 0 );

    var_SetString( p_libheimdall, name, "4/5garbage" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) != 0 );

    var_SetString( p_libheimdall, name, "6.7garbage" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) != 0 );

    var_SetString( p_libheimdall, name, "." );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 0 && den == 1 );

    var_SetString( p_libheimdall, name, "010" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 10 && den == 1 );

    var_SetString( p_libheimdall, name, "30" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 30 && den == 1 );

    var_SetString( p_libheimdall, name, "30.0" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 30 && den == 1 );

    var_SetString( p_libheimdall, name, "030.030" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 3003 && den == 100 );

    var_SetString( p_libheimdall, name, "60/2" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 30 && den == 1 );

    var_SetString( p_libheimdall, name, "29.97" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 2997 && den == 100 );

    var_SetString( p_libheimdall, name, "30000/1001" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 30000 && den == 1001 );

    var_SetString( p_libheimdall, name, ".125" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 1 && den == 8 );

    var_SetString( p_libheimdall, name, "12:9" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 4 && den == 3 );

    var_SetString( p_libheimdall, name, "000000/00000000" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 0 && den == 0 );

    var_SetString( p_libheimdall, name, "12345/0" );
    assert( var_InheritURational( p_libheimdall, &num, &den, name ) == 0 );
    assert( num == 1 && den == 0 );

    var_Destroy( p_libheimdall, name );
}

static void test_strings( libheimdall_int_t *p_libheimdall )
{
    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Create( p_libheimdall, psz_var_name[i], HEIMDALL_VAR_STRING );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_SetString( p_libheimdall, psz_var_name[i], psz_var_name[i] );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        char *psz_tmp = var_GetString( p_libheimdall, psz_var_name[i] );

        assert( psz_tmp != NULL );
        assert( !strcmp( psz_tmp, psz_var_name[i] ) );
        free( psz_tmp );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Destroy( p_libheimdall, psz_var_name[i] );

    /* Some more test for strings */
    var_Create( p_libheimdall, "bla", HEIMDALL_VAR_STRING );
    assert( var_GetNonEmptyString( p_libheimdall, "bla" ) == NULL );
    var_SetString( p_libheimdall, "bla", "" );
    assert( var_GetNonEmptyString( p_libheimdall, "bla" ) == NULL );
    var_SetString( p_libheimdall, "bla", "test" );

    char *psz_tmp = var_GetNonEmptyString( p_libheimdall, "bla" );
    assert( psz_tmp != NULL );
    assert( !strcmp( psz_tmp, "test" ) );
    free( psz_tmp );
    var_Destroy( p_libheimdall, "bla" );
}

static void test_address( libheimdall_int_t *p_libheimdall )
{
    char dummy[VAR_COUNT];

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Create( p_libheimdall, psz_var_name[i], HEIMDALL_VAR_ADDRESS );

    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        var_value[i].p_address = &dummy[i];
        var_SetAddress( p_libheimdall, psz_var_name[i], var_value[i].p_address );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        heimdall_value_t val;
        var_Get( p_libheimdall, psz_var_name[i], &val );
        assert( val.p_address == var_value[i].p_address );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Destroy( p_libheimdall, psz_var_name[i] );
}

static int callback( heimdall_object_t* p_this, char const *psz_var,
                     heimdall_value_t oldval, heimdall_value_t newval, void *p_data)
{
    unsigned i;

    (void)p_this;    (void)oldval;

    // Check the parameters
    assert( p_data == psz_var_name );

    // Find the variable
    for( i = 0; i < VAR_COUNT; i++ )
    {
        if( !strcmp( psz_var_name[i], psz_var ) )
            break;
    }
    // Check the variable is known
    assert( i < VAR_COUNT );

    var_value[i].i_int = newval.i_int;
    return HEIMDALL_SUCCESS;
}

static void test_callbacks( libheimdall_int_t *p_libheimdall )
{
    /* add the callbacks */
    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        var_Create( p_libheimdall, psz_var_name[i], HEIMDALL_VAR_INTEGER );
        var_AddCallback( p_libheimdall, psz_var_name[i], callback, psz_var_name );
    }

    /* Set the variables and trigger the callbacks */
    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        int i_temp = rand();
        var_SetInteger( p_libheimdall, psz_var_name[i], i_temp );
        assert( i_temp == var_value[i].i_int );
        var_SetInteger( p_libheimdall, psz_var_name[i], 0 );
        assert( var_value[i].i_int == 0 );
        var_value[i].i_int = 1;
    }

    /* Only trigger the callback: the value will be 0 again */
    for( unsigned i = 0; i < VAR_COUNT; i++ )
    {
        var_TriggerCallback( p_libheimdall, psz_var_name[i] );
        assert( var_value[i].i_int == 0 );
    }

    for( unsigned i = 0; i < VAR_COUNT; i++ )
        var_Destroy( p_libheimdall, psz_var_name[i] );
}

static void test_limits( libheimdall_int_t *p_libheimdall )
{
    heimdall_value_t val;
    val.i_int = 0;
    var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER );

    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMIN, &val );
    assert( val.i_int == INT64_MIN );
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMAX, &val );
    assert( val.i_int == INT64_MAX );

    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_SETMINMAX,
                (heimdall_value_t){ .i_int = -1234 },
                (heimdall_value_t){ .i_int = 12345 } );

    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMIN, &val );
    assert( val.i_int == -1234 );
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMAX, &val );
    assert( val.i_int == 12345 );

    var_SetInteger( p_libheimdall, "bla", -123456 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == -1234 );
    var_SetInteger( p_libheimdall, "bla", 1234 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 1234 );
    var_SetInteger( p_libheimdall, "bla", 12346 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 12345 );

    val.i_int = 42;
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_SETSTEP, val );
    var_SetInteger( p_libheimdall, "bla", 20 );
    val.i_int = 0;
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETSTEP, &val );
    assert( val.i_int == 42 );

    var_SetInteger( p_libheimdall, "bla", 20 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 0 );

    var_SetInteger( p_libheimdall, "bla", 21 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 42 );

    var_Destroy( p_libheimdall, "bla" );
}

static void test_choices( libheimdall_int_t *p_libheimdall )
{
    heimdall_value_t val;
    heimdall_value_t *vals;
    char **texts;
    size_t count;

    var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND );
    val.i_int = 1;
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_ADDCHOICE, val, "one" );

    val.i_int = 2;
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_ADDCHOICE, val, "two" );

    assert( var_CountChoices( p_libheimdall, "bla" ) == 2 );

    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_DELCHOICE, val );
    assert( var_CountChoices( p_libheimdall, "bla" ) == 1 );

    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETCHOICES, &count, &vals, &texts );
    assert( count == 1 && vals[0].i_int == 1 && !strcmp( texts[0], "one" ) );
    free(texts[0]);
    free(texts);
    free(vals);

    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_CLEARCHOICES );
    assert( var_CountChoices( p_libheimdall, "bla" ) == 0 );

    var_Destroy( p_libheimdall, "bla" );
}

static void test_change( libheimdall_int_t *p_libheimdall )
{
    heimdall_value_t val, min, max, step;

    min.i_int = -1242;
    max.i_int = +42;
    step.i_int = 13;

    var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER );
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_SETMINMAX, min, max );
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_SETSTEP, step );

    var_SetInteger( p_libheimdall, "bla", 13 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 13 );
    var_SetInteger( p_libheimdall, "bla", 27 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 26 );
    var_SetInteger( p_libheimdall, "bla", 35 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 39 );
    var_SetInteger( p_libheimdall, "bla", -2 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == 0 );
    var_SetInteger( p_libheimdall, "bla", -9 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == -13 );
    var_SetInteger( p_libheimdall, "bla", -27 );
    assert( var_GetInteger( p_libheimdall, "bla" ) == -26 );

    /* Test everything is right */
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMIN, &val );
    assert( val.i_int == min.i_int );
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMAX, &val );
    assert( val.i_int == max.i_int );
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETSTEP, &val );
    assert( val.i_int == step.i_int );

    var_Destroy( p_libheimdall, "bla" );
}

static void test_creation_and_type( libheimdall_int_t *p_libheimdall )
{
    heimdall_value_t val;
    val.i_int = 4212;

    var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER );
    assert( var_Type( p_libheimdall, "bla" ) == (HEIMDALL_VAR_INTEGER) );

    assert( var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT ) == HEIMDALL_SUCCESS );
    assert( var_Type( p_libheimdall, "bla" ) == (HEIMDALL_VAR_INTEGER) );

    assert( var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND ) == HEIMDALL_SUCCESS );
    assert( var_Type( p_libheimdall, "bla" ) == (HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND) );

    assert( var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMIN, &val ) != 0
         || val.i_int == INT64_MIN );
    assert( var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMAX, &val ) != 0
         || val.i_int == INT64_MAX );
    val.i_int = 4212;
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_SETMINMAX, val, val );
    assert( var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMIN, &val ) == 0
         && val.i_int == 4212 );
    assert( var_Change( p_libheimdall, "bla", HEIMDALL_VAR_GETMAX, &val ) == 0
         && val.i_int == 4212 );

    assert( var_Change( p_libheimdall, "bla" , HEIMDALL_VAR_GETSTEP, &val ) != 0 );
    val.i_int = 4212;
    var_Change( p_libheimdall, "bla", HEIMDALL_VAR_SETSTEP, val );
    assert( var_Change( p_libheimdall, "bla" , HEIMDALL_VAR_GETSTEP, &val ) == 0
         && val.i_int == 4212 );

    var_Destroy( p_libheimdall, "bla" );
    var_Destroy( p_libheimdall, "bla" );
    var_Destroy( p_libheimdall, "bla" );
    assert( var_Get( p_libheimdall, "bla", &val ) == HEIMDALL_ENOENT );

    var_Create( p_libheimdall, "program", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_DOINHERIT );
    assert( var_Type( p_libheimdall, "program" ) == (HEIMDALL_VAR_INTEGER) );

    assert( var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND ) == HEIMDALL_SUCCESS );
    assert( var_Type( p_libheimdall, "bla" ) == (HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND) );

    var_Destroy( p_libheimdall, "bla" );
    var_Destroy( p_libheimdall, "bla" );
    var_Destroy( p_libheimdall, "bla" );
    assert( var_Get( p_libheimdall, "bla", &val ) == HEIMDALL_ENOENT );

    var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER );
    assert( var_Create( p_libheimdall, "bla", HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND ) == HEIMDALL_SUCCESS );
    assert( var_Type( p_libheimdall, "bla" ) == (HEIMDALL_VAR_INTEGER | HEIMDALL_VAR_ISCOMMAND) );

    var_Destroy( p_libheimdall, "bla" );
    var_Destroy( p_libheimdall, "bla" );
    var_Destroy( p_libheimdall, "bla" );
    assert( var_Get( p_libheimdall, "bla", &val ) == HEIMDALL_ENOENT );
}

static void test_variables( libheimdall_instance_t *p_heimdall )
{
    libheimdall_int_t *p_libheimdall = p_heimdall->p_libheimdall_int;
    srand( time( NULL ) );

    test_log( "Testing for integers\n" );
    test_integer( p_libheimdall );

    test_log( "Testing for booleans\n" );
    test_booleans( p_libheimdall );

    test_log( "Testing for floats\n" );
    test_floats( p_libheimdall );

    test_log( "Testing for rationals\n" );
    test_fracts( p_libheimdall );

    test_log( "Testing for strings\n" );
    test_strings( p_libheimdall );

    test_log( "Testing for addresses\n" );
    test_address( p_libheimdall );

    test_log( "Testing the callbacks\n" );
    test_callbacks( p_libheimdall );

    test_log( "Testing the limits\n" );
    test_limits( p_libheimdall );

    test_log( "Testing choices\n" );
    test_choices( p_libheimdall );

    test_log( "Testing var_Change()\n" );
    test_change( p_libheimdall );

    test_log( "Testing type at creation\n" );
    test_creation_and_type( p_libheimdall );
}


int main( void )
{
    libheimdall_instance_t *p_heimdall;

    test_init();

    test_log( "Testing the core variables\n" );
    p_heimdall = libheimdall_new( test_defaults_nargs, test_defaults_args );
    assert( p_heimdall != NULL );

    test_variables( p_heimdall );

    libheimdall_release( p_heimdall );

    return 0;
}
