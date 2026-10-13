/*****************************************************************************
 * libheimdall_picture.h:  libheimdall external API
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#ifndef HEIMDALL_LIBHEIMDALL_PICTURE_H
#define HEIMDALL_LIBHEIMDALL_PICTURE_H 1

# ifdef __cplusplus
extern "C" {
# endif

typedef struct libheimdall_picture_t libheimdall_picture_t;
typedef struct libheimdall_picture_list_t libheimdall_picture_list_t;

typedef enum libheimdall_picture_type_t
{
    libheimdall_picture_Argb,
    libheimdall_picture_Png,
    libheimdall_picture_Jpg,
    libheimdall_picture_WebP,
    libheimdall_picture_Rgba,
} libheimdall_picture_type_t;

/**
 * Increment the reference count of this picture.
 *
 * \see libheimdall_picture_release()
 * \param pic A picture object
 * \return the same object
 */
LIBHEIMDALL_API libheimdall_picture_t *
libheimdall_picture_retain( libheimdall_picture_t* pic );

/**
 * Decrement the reference count of this picture.
 * When the reference count reaches 0, the picture will be released.
 * The picture must not be accessed after calling this function.
 *
 * \see libheimdall_picture_retain
 * \param pic A picture object
 */
LIBHEIMDALL_API void
libheimdall_picture_release( libheimdall_picture_t* pic );

/**
 * Saves this picture to a file. The image format is the same as the one
 * returned by \link libheimdall_picture_type \endlink
 *
 * \param pic A picture object
 * \param path The path to the generated file
 * \return 0 in case of success, -1 otherwise
 */
LIBHEIMDALL_API int
libheimdall_picture_save( const libheimdall_picture_t* pic, const char* path );

/**
 * Returns the image internal buffer, including potential padding.
 * The libheimdall_picture_t owns the returned buffer, which must not be modified nor
 * freed.
 *
 * \param pic A picture object
 * \param size A pointer to a size_t that will hold the size of the buffer [required]
 * \return A pointer to the internal buffer.
 */
LIBHEIMDALL_API const unsigned char*
libheimdall_picture_get_buffer( const libheimdall_picture_t* pic, size_t *size );

/**
 * Returns the picture type
 *
 * \param pic A picture object
 * \see libheimdall_picture_type_t
 */
LIBHEIMDALL_API libheimdall_picture_type_t
libheimdall_picture_type( const libheimdall_picture_t* pic );

/**
 * Returns the image stride, ie. the number of bytes per line.
 * This can only be called on images of type libheimdall_picture_Argb or
 * libheimdall_picture_Rgba
 *
 * \param pic A picture object
 */
LIBHEIMDALL_API unsigned int
libheimdall_picture_get_stride( const libheimdall_picture_t* pic );

/**
 * Returns the width of the image in pixels
 *
 * \param pic A picture object
 */
LIBHEIMDALL_API unsigned int
libheimdall_picture_get_width( const libheimdall_picture_t* pic );

/**
 * Returns the height of the image in pixels
 *
 * \param pic A picture object
 */
LIBHEIMDALL_API unsigned int
libheimdall_picture_get_height( const libheimdall_picture_t* pic );

/**
 * Returns the time at which this picture was generated, in microseconds (us)
 * \param pic A picture object
 */
LIBHEIMDALL_API libheimdall_time_t
libheimdall_picture_get_time( const libheimdall_picture_t* pic );

/**
 * Returns the number of pictures in the list
 */
LIBHEIMDALL_API size_t libheimdall_picture_list_count( const libheimdall_picture_list_t* list );

/**
 * Returns the picture at the provided index.
 *
 * The returned picture is a weak reference of libheimdall_picture_t* owned by the
 * list. It must not be released and remains valid only as long as the list is
 * alive.
 *
 * If the index is out of bound, the result is undefined.
 */
LIBHEIMDALL_API libheimdall_picture_t* libheimdall_picture_list_at( const libheimdall_picture_list_t* list,
                                                     size_t index );

/**
 * Destroys a picture list and releases the pictures it contains
 * \param list The list to destroy
 *
 * Calling this function with a NULL list is safe and will return immediately
 */
LIBHEIMDALL_API void libheimdall_picture_list_destroy( libheimdall_picture_list_t* list );

# ifdef __cplusplus
}
# endif

#endif // HEIMDALL_LIBHEIMDALL_PICTURE_H
