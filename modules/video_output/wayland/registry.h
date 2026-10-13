/**
 * @file registry.h
 * @brief Wayland client register common helpers for Heimdall media player
 */
/*****************************************************************************
 * Copyright © 2019 Rémi Denis-Courmont
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

#include <stdint.h>

struct wl_display;
struct wl_event_queue;
struct wl_interface;
struct wl_shm;
struct heimdall_wl_registry;

struct heimdall_wl_registry *heimdall_wl_registry_get(struct wl_display *display,
                                            struct wl_event_queue *queue);
void heimdall_wl_registry_destroy(struct heimdall_wl_registry *vr);

uint_fast32_t heimdall_wl_interface_get_version(struct heimdall_wl_registry *,
                                           const char *interface);
struct wl_proxy *heimdall_wl_interface_bind(struct heimdall_wl_registry *,
                                       const char *interface,
                                       const struct wl_interface *,
                                       uint32_t *version);

struct wl_compositor *heimdall_wl_compositor_get(struct heimdall_wl_registry *);
struct wl_shm *heimdall_wl_shm_get(struct heimdall_wl_registry *);

