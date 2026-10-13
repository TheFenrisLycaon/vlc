/*****************************************************************************
 * heimdall_mouse.h: mouse related structures and functions
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
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

#ifndef _HEIMDALL_MOUSE_H
#define _HEIMDALL_MOUSE_H 1

/**
 * Mouse buttons
 */
enum heimdall_mouse_button
{
    MOUSE_BUTTON_LEFT=0,
    MOUSE_BUTTON_CENTER,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_WHEEL_UP,
    MOUSE_BUTTON_WHEEL_DOWN,
    MOUSE_BUTTON_WHEEL_LEFT,
    MOUSE_BUTTON_WHEEL_RIGHT,
    MOUSE_BUTTON_MAX
};

/**
 * Mouse state
 */
typedef struct heimdall_mouse_t
{
    /* Coordinate */
    int i_x;
    int i_y;
    /* Mask of pressed button */
    int i_pressed;
    /* Is double clicked */
    bool b_double_click;
} heimdall_mouse_t;

/**
 * Mouse event callback
 * @param mouse new mouse event to process, can be NULL for an invalidate state
 * (a new vout is created or restarted).
 * @param user_data
 */
typedef void (*heimdall_mouse_event)(const heimdall_mouse_t *mouse, void *user_data);

static inline void heimdall_mouse_Init( heimdall_mouse_t *p_mouse )
{
    p_mouse->i_x = 0;
    p_mouse->i_y = 0;
    p_mouse->i_pressed = 0;
    p_mouse->b_double_click = false;
}

/* */
static inline void heimdall_mouse_SetPressed( heimdall_mouse_t *p_mouse,
                                         int i_button )
{
    p_mouse->i_pressed |= 1 << i_button;
}
static inline void heimdall_mouse_SetReleased( heimdall_mouse_t *p_mouse,
                                          int i_button )
{
    p_mouse->i_pressed &= ~(1 << i_button);
}
static inline void heimdall_mouse_SetPosition( heimdall_mouse_t *p_mouse,
                                          int i_x, int i_y )
{
    p_mouse->i_x = i_x;
    p_mouse->i_y = i_y;
}

/* */
static inline bool heimdall_mouse_IsPressed( const heimdall_mouse_t *p_mouse,
                                        int i_button )
{
    return ( p_mouse->i_pressed & (1 << i_button) ) != 0;
}
static inline bool heimdall_mouse_IsLeftPressed( const heimdall_mouse_t *p_mouse )
{
    return heimdall_mouse_IsPressed( p_mouse, MOUSE_BUTTON_LEFT );
}
static inline bool heimdall_mouse_IsCenterPressed( const heimdall_mouse_t *p_mouse )
{
    return heimdall_mouse_IsPressed( p_mouse, MOUSE_BUTTON_CENTER );
}
static inline bool heimdall_mouse_IsRightPressed( const heimdall_mouse_t *p_mouse )
{
    return heimdall_mouse_IsPressed( p_mouse, MOUSE_BUTTON_RIGHT );
}
static inline bool heimdall_mouse_IsWheelUpPressed( const heimdall_mouse_t *p_mouse )
{
    return heimdall_mouse_IsPressed( p_mouse, MOUSE_BUTTON_WHEEL_UP );
}
static inline bool heimdall_mouse_IsWheelDownPressed( const heimdall_mouse_t *p_mouse )
{
    return heimdall_mouse_IsPressed( p_mouse, MOUSE_BUTTON_WHEEL_DOWN );
}
static inline void heimdall_mouse_GetMotion( int *pi_x, int *pi_y,
                                        const heimdall_mouse_t *p_old,
                                        const heimdall_mouse_t *p_new )
{
    *pi_x = p_new->i_x - p_old->i_x;
    *pi_y = p_new->i_y - p_old->i_y;
}

/* */
static inline bool heimdall_mouse_HasChanged( const heimdall_mouse_t *p_old,
                                         const heimdall_mouse_t *p_new )
{
    return p_old->i_x != p_new->i_x || p_old->i_y != p_new->i_y ||
           p_old->i_pressed != p_new->i_pressed;
}
static inline bool heimdall_mouse_HasMoved( const heimdall_mouse_t *p_old,
                                       const heimdall_mouse_t *p_new )
{
    return p_old->i_x != p_new->i_x || p_old->i_y != p_new->i_y;
}
static inline bool heimdall_mouse_HasDragged( const heimdall_mouse_t *p_old,
                                       const heimdall_mouse_t *p_new )
{
    return heimdall_mouse_IsLeftPressed( p_new ) &&
           heimdall_mouse_HasMoved( p_old, p_new );
}
static inline bool heimdall_mouse_HasButton( const heimdall_mouse_t *p_old,
                                        const heimdall_mouse_t *p_new )
{
    return p_old->i_pressed != p_new->i_pressed;
}
static inline bool heimdall_mouse_HasPressed( const heimdall_mouse_t *p_old,
                                         const heimdall_mouse_t *p_new,
                                         int i_button )
{
    const int i_mask = 1 << i_button;
    return (p_old->i_pressed & i_mask) == 0 && (p_new->i_pressed & i_mask);
}
static inline bool heimdall_mouse_HasReleased( const heimdall_mouse_t *p_old,
                                          const heimdall_mouse_t *p_new,
                                          int i_button )
{
    const int i_mask = 1 << i_button;
    return (p_old->i_pressed & i_mask) && (p_new->i_pressed & i_mask) == 0;
}
#endif /* _HEIMDALL_MOUSE_H */

