/*****************************************************************************
 * lru.c : Last recently used cache implementation
 *****************************************************************************
 * Copyright (C) 2020 - VideoLabs, VLC authors and VideoLAN
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_arrays.h>
#include <heimdall_list.h>
#include "lru.h"

struct heimdall_lru_entry
{
    char *psz_key;
    void *value;
    struct heimdall_list node;
};

struct heimdall_lru
{
    void (*releaseValue)(void *, void *);
    void *priv;
    unsigned max;
    heimdall_dictionary_t dict;
    struct heimdall_list list;
    struct heimdall_lru_entry *last;
};

static void heimdall_lru_releaseentry( void *value, void *priv )
{
    struct heimdall_lru_entry *entry = value;
    struct heimdall_lru *lru = priv;
    free(entry->psz_key);
    if( lru->releaseValue )
        lru->releaseValue( lru->priv, entry->value );
    free(entry);
}

heimdall_lru * heimdall_lru_New( unsigned max,
                       void(*releaseValue)(void *, void *), void *priv )
{
    heimdall_lru *lru = malloc(sizeof(*lru));
    if( lru )
    {
        lru->priv = priv;
        lru->max = max;
        heimdall_dictionary_init( &lru->dict, max );
        heimdall_list_init( &lru->list );
        lru->releaseValue = releaseValue;
        lru->last = NULL;
    }
    return lru;
}

void heimdall_lru_Release( heimdall_lru *lru )
{
    heimdall_dictionary_clear( &lru->dict, heimdall_lru_releaseentry, lru );
    free( lru );
}

bool heimdall_lru_HasKey( heimdall_lru *lru, const char *psz_key )
{
    return heimdall_dictionary_has_key( &lru->dict, psz_key );
}

void * heimdall_lru_Get( heimdall_lru *lru, const char *psz_key )
{
    struct heimdall_lru_entry *entry = heimdall_dictionary_value_for_key( &lru->dict, psz_key );
    if( entry )
    {
        if( !heimdall_list_is_first( &entry->node, &lru->list ) )
        {
            if( heimdall_list_is_last( &entry->node, &lru->list ) )
            {
                lru->last = heimdall_list_entry(entry->node.prev, struct heimdall_lru_entry, node);
            }
            heimdall_list_remove( &entry->node );
            heimdall_list_add_after( &entry->node, &lru->list );
        }
        return entry->value;
    }
    return NULL;
}

void heimdall_lru_Insert( heimdall_lru *lru, const char *psz_key, void *value )
{
    struct heimdall_lru_entry *entry = calloc(1, sizeof(*entry));
    if(!entry)
    {
        lru->releaseValue(lru->priv, value);
        return;
    }
    entry->psz_key = strdup( psz_key );
    if(!entry->psz_key)
    {
        lru->releaseValue(lru->priv, value);
        free(entry);
        return;
    }
    entry->value = value;
    heimdall_list_init( &entry->node );

    if( heimdall_list_is_empty( &lru->list ) )
        lru->last = entry;
    heimdall_dictionary_insert( &lru->dict, psz_key, entry );
    heimdall_list_add_after( &entry->node, &lru->list );

    if( (unsigned)heimdall_dictionary_keys_count(&lru->dict) >= lru->max )
    {
        struct heimdall_lru_entry *toremove = lru->last;
        lru->last = heimdall_list_entry(toremove->node.prev, struct heimdall_lru_entry, node);
        heimdall_list_remove(&toremove->node);
        heimdall_dictionary_remove_value_for_key(&lru->dict, toremove->psz_key, NULL, NULL);
        heimdall_lru_releaseentry(toremove, lru);
    }
}

void heimdall_lru_Apply( heimdall_lru *lru,
                    void(*func)(void *, const char *, void *),
                    void *priv )
{
    struct heimdall_lru_entry *entry;
    heimdall_list_foreach( entry, &lru->list, node )
        func( priv, entry->psz_key, entry->value );
}
