/*****************************************************************************
 * rand.c : non-predictible random bytes generator
 *****************************************************************************
 * Copyright © 2007 Rémi Denis-Courmont
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

#include <heimdall_common.h>
#include <heimdall_rand.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <heimdall_fs.h>

#include <heimdall_hash.h>
#include <heimdall_tick.h>
#include <heimdall_threads.h>

/*
 * Pseudo-random number generator using a HMAC-MD5 in counter mode.
 * Probably not very secure (expert patches welcome) but definitely
 * better than rand() which is defined to be reproducible...
 */
#define BLOCK_SIZE 64

static uint8_t okey[BLOCK_SIZE], ikey[BLOCK_SIZE];

static void heimdall_rand_init (void)
{
    uint8_t key[BLOCK_SIZE];

    /* Get non-predictible value as key for HMAC */
    int fd = heimdall_open ("/dev/urandom", O_RDONLY);
    if (fd == -1)
        return; /* Uho! */

    for (size_t i = 0; i < sizeof (key);)
    {
         ssize_t val = read (fd, key + i, sizeof (key) - i);
         if (val > 0)
             i += val;
    }

    /* Precompute outer and inner keys for HMAC */
    for (size_t i = 0; i < sizeof (key); i++)
    {
        okey[i] = key[i] ^ 0x5c;
        ikey[i] = key[i] ^ 0x36;
    }

    heimdall_close (fd);
}


void heimdall_rand_bytes (void *buf, size_t len)
{
    static heimdall_mutex_t lock = HEIMDALL_STATIC_MUTEX;
    static uint64_t counter = 0;
    struct timespec ts;

    timespec_get(&ts, TIME_UTC);

    while (len > 0)
    {
        uint64_t val;
        heimdall_hash_md5_t mdi, mdo;
        uint8_t mdi_buf[HEIMDALL_HASH_MD5_DIGEST_SIZE];
        uint8_t mdo_buf[HEIMDALL_HASH_MD5_DIGEST_SIZE];

        heimdall_hash_md5_Init (&mdi);
        heimdall_hash_md5_Init (&mdo);

        heimdall_mutex_lock (&lock);
        if (counter == 0)
            heimdall_rand_init ();
        val = counter++;

        heimdall_hash_md5_Update (&mdi, ikey, sizeof (ikey));
        heimdall_hash_md5_Update (&mdo, okey, sizeof (okey));
        heimdall_mutex_unlock (&lock);

        heimdall_hash_md5_Update (&mdi, &ts, sizeof (ts));
        heimdall_hash_md5_Update (&mdi, &val, sizeof (val));
        heimdall_hash_md5_Finish (&mdi, mdi_buf, sizeof(mdi_buf));
        heimdall_hash_md5_Update (&mdo, mdi_buf, sizeof(mdi_buf));
        heimdall_hash_md5_Finish (&mdo, mdo_buf, sizeof(mdo_buf));

        memcpy (buf, mdo_buf, (len < sizeof(mdo_buf)) ? len : sizeof(mdo_buf));

        if (len < sizeof(mdo_buf))
            break;

        len -= sizeof(mdo_buf);
        buf = ((uint8_t *)buf) + sizeof(mdo_buf);
    }
}
