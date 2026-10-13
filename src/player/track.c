/*****************************************************************************
 * player_track.c: Player track and program implementation
 *****************************************************************************
 * Copyright © 2018-2019 VLC authors and VideoLAN
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <limits.h>

#include <heimdall_common.h>
#include "player.h"

static char *
heimdall_player_program_DupTitle(int id, const char *title)
{
    char *dup;
    if (title)
        dup = strdup(title);
    else if (asprintf(&dup, "%d", id) == -1)
        dup = NULL;
    return dup;
}

struct heimdall_player_program *
heimdall_player_program_New(int id, const char *name)
{
    struct heimdall_player_program *prgm = malloc(sizeof(*prgm));
    if (!prgm)
        return NULL;
    prgm->name = heimdall_player_program_DupTitle(id, name);
    if (!prgm->name)
    {
        free(prgm);
        return NULL;
    }
    prgm->group_id = id;
    prgm->selected = prgm->scrambled = false;

    return prgm;
}

int
heimdall_player_program_Update(struct heimdall_player_program *prgm, int id,
                          const char *name)
{
    free((char *)prgm->name);
    prgm->name = heimdall_player_program_DupTitle(id, name);
    return prgm->name != NULL ? HEIMDALL_SUCCESS : HEIMDALL_ENOMEM;
}

struct heimdall_player_program *
heimdall_player_program_Dup(const struct heimdall_player_program *src)
{
    struct heimdall_player_program *dup =
        heimdall_player_program_New(src->group_id, src->name);

    if (!dup)
        return NULL;
    dup->selected = src->selected;
    dup->scrambled = src->scrambled;
    return dup;
}

void
heimdall_player_program_Delete(struct heimdall_player_program *prgm)
{
    free((char *)prgm->name);
    free(prgm);
}

struct heimdall_player_program *
heimdall_player_program_vector_FindById(heimdall_player_program_vector *vec, int id,
                                   size_t *idx)
{
    for (size_t i = 0; i < vec->size; ++i)
    {
        struct heimdall_player_program *prgm = vec->data[i];
        if (prgm->group_id == id)
        {
            if (idx)
                *idx = i;
            return prgm;
        }
    }
    return NULL;
}

struct heimdall_player_track_priv *
heimdall_player_track_priv_New(heimdall_es_id_t *id, const char *name, const es_format_t *fmt)
{
    struct heimdall_player_track_priv *trackpriv = malloc(sizeof(*trackpriv));
    if (!trackpriv)
        return NULL;
    struct heimdall_player_track *track = &trackpriv->t;

    trackpriv->delay = HEIMDALL_TICK_MAX;
    trackpriv->vout = NULL;
    trackpriv->vout_order = HEIMDALL_VOUT_ORDER_NONE;
    trackpriv->selected_by_user = false;

    track->name = strdup(name);
    if (!track->name)
    {
        free(track);
        return NULL;
    }

    int ret = es_format_Copy(&track->fmt, fmt);
    if (ret != HEIMDALL_SUCCESS)
    {
        free((char *)track->name);
        free(track);
        return NULL;
    }
    track->es_id = heimdall_es_id_Hold(id);
    track->selected = false;

    return trackpriv;
}

void
heimdall_player_track_priv_Delete(struct heimdall_player_track_priv *trackpriv)
{
    struct heimdall_player_track *track = &trackpriv->t;
    es_format_Clean(&track->fmt);
    free((char *)track->name);
    heimdall_es_id_Release(track->es_id);
    free(trackpriv);
}

void
heimdall_player_track_Delete(struct heimdall_player_track *track)
{
    struct heimdall_player_track_priv *trackpriv =
        container_of(track, struct heimdall_player_track_priv, t);
    heimdall_player_track_priv_Delete(trackpriv);
}

struct heimdall_player_track *
heimdall_player_track_Dup(const struct heimdall_player_track *src)
{
    struct heimdall_player_track_priv *duppriv =
        heimdall_player_track_priv_New(src->es_id, src->name, &src->fmt);

    if (!duppriv)
        return NULL;
    duppriv->t.selected = src->selected;
    return &duppriv->t;
}

int
heimdall_player_track_priv_Update(struct heimdall_player_track_priv *trackpriv,
                             const char *name, const es_format_t *fmt)
{
    struct heimdall_player_track *track = &trackpriv->t;

    if (strcmp(name, track->name) != 0)
    {
        char *dup = strdup(name);
        if (!dup)
            return HEIMDALL_ENOMEM;
        free((char *)track->name);
        track->name = dup;
    }

    es_format_t fmtdup;
    int ret = es_format_Copy(&fmtdup, fmt);
    if (ret != HEIMDALL_SUCCESS)
        return ret;

    es_format_Clean(&track->fmt);
    track->fmt = fmtdup;
    return HEIMDALL_SUCCESS;
}

struct heimdall_player_track_priv *
heimdall_player_track_vector_FindById(heimdall_player_track_vector *vec, heimdall_es_id_t *id,
                                 size_t *idx)
{
    for (size_t i = 0; i < vec->size; ++i)
    {
        struct heimdall_player_track_priv *trackpriv = vec->data[i];
        if (trackpriv->t.es_id == id)
        {
            if (idx)
                *idx = i;
            return trackpriv;
        }
    }
    return NULL;
}


int
heimdall_player_GetFirstSelectedTrackId(const heimdall_player_track_vector* tracks)
{
    struct heimdall_player_track_priv* t;
    heimdall_vector_foreach(t, tracks)
    {
        if (t->t.selected)
            return t->t.fmt.i_id;
    }
    return -1;
}
