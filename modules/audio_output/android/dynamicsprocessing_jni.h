/*****************************************************************************
 * android/dynamicsprocessing_jni.h: Android DynamicsProcessing
 *****************************************************************************
 * Copyright © 2012-2023 VLC authors and VideoLAN, VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Thomas Guillem <thomas@gllm.fr>
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

int
DynamicsProcessing_InitJNI(heimdall_object_t *obj);

jobject
DynamicsProcessing_New(heimdall_object_t *obj, int32_t session_id);
#define DynamicsProcessing_New(o, s) \
    (DynamicsProcessing_New)(HEIMDALL_OBJECT(o), s)

int
DynamicsProcessing_SetVolume(heimdall_object_t *obj, jobject dp, float volume);
#define DynamicsProcessing_SetVolume(o, dp, v) \
    (DynamicsProcessing_SetVolume)(HEIMDALL_OBJECT(o), dp, v)

void
DynamicsProcessing_Disable(heimdall_object_t *obj, jobject dp);
#define DynamicsProcessing_Disable(o, dp) \
    (DynamicsProcessing_Disable)(HEIMDALL_OBJECT(o), dp)

void
DynamicsProcessing_Delete(heimdall_object_t *obj, jobject dp);
#define DynamicsProcessing_Delete(o, dp) \
    (DynamicsProcessing_Delete)(HEIMDALL_OBJECT(o), dp)


