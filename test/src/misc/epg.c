/*****************************************************************************
 * epg.c test EPG
 *****************************************************************************
 * Copyright (C) 2016 - VideoLAN Authors
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/
#include "../../libheimdall/test.h"
#ifdef NDEBUG
 #undef NDEBUG
#endif
#include <heimdall_common.h>
#include <heimdall_epg.h>
#include <assert.h>

static void assert_current( const heimdall_epg_t *p_epg, const char *psz_name )
{
    if( (void*)psz_name == (void*)p_epg->p_current )
    {
        assert( psz_name == NULL );
    }
    else
    {
        assert( p_epg->p_current );
        assert( p_epg->p_current->psz_name );
        assert( psz_name[0] == p_epg->p_current->psz_name[0] );
    }
}

static void print_order( const heimdall_epg_t *p_epg )
{
    printf("order: ");
    for( size_t i = 0; i < p_epg->i_event; i++ )
        printf("%s ", p_epg->pp_event[i]->psz_name );
    if( p_epg->p_current )
        printf(" current %s", p_epg->p_current->psz_name );
    printf("\n");
}

static void assert_events( const heimdall_epg_t *p_epg, const char *psz_names, size_t i_names )
{
    assert( p_epg->i_event == i_names );
    for( size_t i = 0; i < p_epg->i_event; i++ )
    {
        assert( p_epg->pp_event[i]->psz_name &&
                p_epg->pp_event[i]->psz_name[0] == psz_names[i] );
    }
}

#define EPG_ADD(epg, start, duration, a) \
    do {\
        heimdall_epg_event_t *p_evt = heimdall_epg_event_New( start, start, duration );\
        if( p_evt )\
        {\
            p_evt->psz_name = strdup( a );\
            if( !p_evt->psz_name || !heimdall_epg_AddEvent( epg, p_evt ) )\
                heimdall_epg_event_Delete( p_evt );\
        }\
    } while( 0 )

int main( void )
{
    test_init();

    int i=1;

    /* Simple insert/current test */
    printf("--test %d\n", i++);
    heimdall_epg_t *p_epg = heimdall_epg_New( 0, 0 );
    assert(p_epg);
    EPG_ADD( p_epg,  42, 20, "A" );
    EPG_ADD( p_epg,  62, 20, "B" );
    EPG_ADD( p_epg,  82, 20, "C" );
    EPG_ADD( p_epg, 102, 20, "D" );
    print_order( p_epg );
    assert_events( p_epg, "ABCD", 4 );
    assert_current( p_epg, NULL );

    heimdall_epg_SetCurrent( p_epg, 82 );
    assert_current( p_epg, "C" );

    heimdall_epg_Delete( p_epg );


    /* Test reordering / head/tail inserts */
    printf("--test %d\n", i++);
    p_epg = heimdall_epg_New( 0, 0 );
    assert(p_epg);
    EPG_ADD( p_epg,  82, 20, "C" );
    EPG_ADD( p_epg,  62, 20, "B" );
    EPG_ADD( p_epg, 102, 20, "D" );
    EPG_ADD( p_epg,  42, 20, "A" );
    print_order( p_epg );
    assert_events( p_epg, "ABCD", 4 );
    heimdall_epg_Delete( p_epg );

    /* Test reordering/bisect lookup on insert */
    printf("--test %d\n", i++);
    p_epg = heimdall_epg_New( 0, 0 );
    assert(p_epg);
    EPG_ADD( p_epg, 142, 20, "F" );
    EPG_ADD( p_epg, 122, 20, "E" );
    EPG_ADD( p_epg, 102, 20, "D" );
    EPG_ADD( p_epg,  82, 20, "C" );
    EPG_ADD( p_epg,  42, 20, "A" );
    EPG_ADD( p_epg,  62, 20, "B" );
    print_order( p_epg );
    assert_events( p_epg, "ABCDEF", 6 );
    heimdall_epg_Delete( p_epg );

    /* Test deduplication and current pointer rebasing on insert */
    printf("--test %d\n", i++);
    p_epg = heimdall_epg_New( 0, 0 );
    assert(p_epg);
    EPG_ADD( p_epg,  62, 20, "E" );
    EPG_ADD( p_epg,  62, 20, "F" );
    EPG_ADD( p_epg,  42, 20, "A" );
    heimdall_epg_SetCurrent( p_epg, 62 );
    EPG_ADD( p_epg,  82, 20, "C" );
    EPG_ADD( p_epg,  62, 20, "B" );
    EPG_ADD( p_epg, 102, 20, "D" );
    print_order( p_epg );
    assert_events( p_epg, "ABCD", 4 );
    assert_current( p_epg, "B" );
    heimdall_epg_Delete( p_epg );

    return 0;
}
