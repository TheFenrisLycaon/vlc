/*****************************************************************************
 * filters.h: VAAPI filters helper for Heimdall
 *****************************************************************************
 * Copyright (C) 2017 VLC authors, VideoLAN and VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Thomas Guillem <thomas@gllm.fr>
 *          Petri Hintukainen <phintuka@gmail.com>
 *          Victorien Le Couviour--Tuffet <victorien.lecouviour.tuffet@gmail.com>
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

#ifndef HEIMDALL_VAAPI_FILTERS_H
# define HEIMDALL_VAAPI_FILTERS_H

#include "heimdall_vaapi.h"

/* chroma filters callbacks */
int  heimdall_vaapi_OpenChroma(filter_t *);
void heimdall_vaapi_CloseChroma(filter_t *);

#endif /* HEIMDALL_VAAPI_FILTERS_H */
