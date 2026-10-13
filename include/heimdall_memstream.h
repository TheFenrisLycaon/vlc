/*****************************************************************************
 * heimdall_memstream.h:
 *****************************************************************************
 * Copyright (C) 2016 Rémi Denis-Courmont
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

#ifndef HEIMDALL_MEMSTREAM_H
# define HEIMDALL_MEMSTREAM_H 1

# include <stdarg.h>
# include <stdio.h>

/**
 * \defgroup memstream In-memory byte streams
 * \ingroup cext
 *
 * In-memory byte stream are a portable wrapper for in-memory formatted output
 * byte streams. Compare with POSIX @c open_memstream().
 *
 * @{
 */

/**
 * In-memory stream object.
 */
struct heimdall_memstream
{
    FILE *stream;
    char *ptr; /**< Buffer start address */
    size_t length; /**< Buffer length in bytes */
};

/**
 * Initializes a byte stream object.
 *
 * @note Even when this function fails, the stream object is initialized and
 * can be used safely. It is sufficient to check for errors from
 * heimdall_memstream_flush() and heimdall_memstream_close().
 *
 * Compare with POSIX @c open_memstream().
 *
 * @param ms byte stream object
 *
 * @retval 0 on success
 * @retval EOF on error
 */
HEIMDALL_API
int heimdall_memstream_open(struct heimdall_memstream *ms);

/**
 * Flushes a byte stream object.
 *
 * This function ensures that any previous write to the byte stream is flushed
 * and the in-memory buffer is synchronized. It can be used observe the content
 * of the buffer before the final heimdall_memstream_close().
 *
 * Compare with @c fflush().
 *
 * @note heimdall_memstream_close() implicitly flushes the object.
 * Calling heimdall_memstream_flush() before closing is thus superfluous.
 *
 * @warning @c ms->ptr must <b>not</b> be freed. It can only be freed after
 * a successful call to heimdall_memstream_close().
 *
 * @retval 0 success, i.e., @c ms->ptr and @c ms->length are valid
 * @retval EOF failure (@c ms->ptr and @c ms->length are unspecified)
 */
HEIMDALL_API
int heimdall_memstream_flush(struct heimdall_memstream *ms) HEIMDALL_USED;

/**
 * Closes a byte stream object.
 *
 * This function flushes the stream object, releases any underlying
 * resource, except for the heap-allocated formatted buffer @c ms->ptr,
 * and deinitializes the object.
 *
 * On success, the caller is responsible for freeing the buffer with @c free().
 *
 * Compare with @c fclose().
 *
 * \retval 0 success
 * \retval EOF failure (@c ms->ptr and @c ms->length are unspecified)
 */
HEIMDALL_API
int heimdall_memstream_close(struct heimdall_memstream *ms) HEIMDALL_USED;

/**
 * Appends a binary blob to a byte stream.
 *
 * Compare with @c fwrite().
 *
 * @param ms the Heimdall memstream to write to
 * @param ptr start address of the blob
 * @param len byte length of the blob
 */
HEIMDALL_API
size_t heimdall_memstream_write(struct heimdall_memstream *ms,
                           const void *ptr, size_t len);

/**
 * Appends a single byte to a byte stream.
 *
 * Compare with @c putc() or @c fputc().
 *
 * @param ms the Heimdall memstream to write to
 * @param c Unsigned byte value converted to int.
 */
HEIMDALL_API
int heimdall_memstream_putc(struct heimdall_memstream *ms, int c);

/**
 * Appends a nul-terminated string to a byte stream.
 *
 * Compare with @c fputs().
 */
HEIMDALL_API
int heimdall_memstream_puts(struct heimdall_memstream *ms, const char *str);

/**
 * Appends a formatted string to a byte stream.
 *
 * Compare with @c vfprintf().
 */
HEIMDALL_API
int heimdall_memstream_vprintf(struct heimdall_memstream *ms, const char *fmt,
                          va_list args);

/**
 * Appends a formatted string to a byte stream.
 *
 * Compare with @c fprintf().
 */
HEIMDALL_API
int heimdall_memstream_printf(struct heimdall_memstream *s, const char *fmt,
                         ...) HEIMDALL_FORMAT(2,3);

# ifdef __GNUC__
static inline int heimdall_memstream_puts_len(struct heimdall_memstream *ms,
                                         const char *str, size_t len)
{
    return (heimdall_memstream_write(ms, str, len) == len) ? (int)len : EOF;
}
#  define heimdall_memstream_puts(ms,s) \
    (__builtin_constant_p(__builtin_strlen(s)) ? \
        heimdall_memstream_puts_len(ms,s,__builtin_strlen(s)) : \
        heimdall_memstream_puts(ms,s))
# endif

/** @} */

#endif /* HEIMDALL_MEMSTREAM_H */
