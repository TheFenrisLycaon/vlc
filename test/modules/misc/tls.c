/*****************************************************************************
 * tls.c: Transport Layer Security module test
 *****************************************************************************
 * Copyright © 2016 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <poll.h>

#include "../../libheimdall/test.h"

#include <heimdall_common.h>
#include <heimdall_poll.h>
#include <heimdall_modules.h>
#include <heimdall_tls.h>
#include "../../../lib/libheimdall_internal.h"

#include <heimdall/heimdall.h>

static heimdall_tls_server_t *server_creds;
static heimdall_tls_client_t *client_creds;

static void *tls_echo(void *data)
{
    heimdall_tls_t *tls = data;
    ssize_t val;
    char buf[256];

    while ((val = heimdall_tls_SessionHandshake(server_creds, tls)) > 0)
    {
        struct pollfd ufd;

        switch (val)
        {
            case 1:  ufd.events = POLLIN;  break;
            case 2:  ufd.events = POLLOUT; break;
            default: heimdall_assert_unreachable();
        }

        ufd.fd = heimdall_tls_GetPollFD(tls, &ufd.events);
        poll(&ufd, 1, -1);
    }

    if (val < 0)
        goto error;

    while ((val = heimdall_tls_Read(tls, buf, sizeof (buf), false)) > 0)
        if (heimdall_tls_Write(tls, buf, val) < val)
            goto error;

    if (val < 0 || heimdall_tls_Shutdown(tls, false))
        goto error;

    heimdall_tls_Close(tls);
    return tls;
error:
    heimdall_tls_Close(tls);
    return NULL;
}

static heimdall_tls_t *securepair(heimdall_thread_t *th,
                             const char *const salpnv[],
                             const char *const calpnv[],
                             char **restrict alp)
{
    heimdall_tls_t *socks[2];
    heimdall_tls_t *server, *client;
    int val;

    val = heimdall_tls_SocketPair(PF_LOCAL, 0, socks);
    assert(val == 0);

    server = heimdall_tls_ServerSessionCreate(server_creds, socks[0], salpnv);
    assert(server != NULL);

    val = heimdall_clone(th, tls_echo, server);
    assert(val == 0);

    client = heimdall_tls_ClientSessionCreate(client_creds, socks[1],
                                         "localhost", "heimdall-tls-test",
                                         calpnv, alp);
    if (client == NULL)
    {
        heimdall_tls_SessionDelete(socks[1]);
        heimdall_join(*th, NULL);
        return NULL;
    }
    return client;
}

#define CERTDIR TOP_SRCDIR "/test/samples/certs"
#define CERTFILE CERTDIR "/certkey.pem"

static const char *const test_cert_argv[] = {
    "--no-gnutls-system-trust", "--gnutls-dir-trust=" CERTDIR, NULL };
static const char *const alpn[] = { "foo", "bar", NULL };
static const char *const alpn_bad[] = { "baz", NULL };

int main(void)
{
    libheimdall_instance_t *heimdall;
    heimdall_object_t *obj;
    heimdall_thread_t th;
    void *p;
    heimdall_tls_t *tls;
    char *alp;
    int val;

    test_setup();

    /*** Tests with normal certs database - server cert not acceptable. ***/
    heimdall = libheimdall_new(0, NULL);
    assert(heimdall != NULL);
    obj = HEIMDALL_OBJECT(heimdall->p_libheimdall_int);

    server_creds = heimdall_tls_ServerCreate(obj, SRCDIR"/nonexistent", NULL);
    assert(server_creds == NULL);
    server_creds = heimdall_tls_ServerCreate(obj, SRCDIR"/samples/empty.voc", NULL);
    assert(server_creds == NULL);
    server_creds = heimdall_tls_ServerCreate(obj, CERTFILE, SRCDIR"/nonexistent");
    assert(server_creds == NULL);
    server_creds = heimdall_tls_ServerCreate(obj, CERTFILE, NULL);
    if (server_creds == NULL)
    {
        libheimdall_release(heimdall);
        return 77;
    }
    heimdall_tls_ServerDelete(server_creds);

    server_creds = heimdall_tls_ServerCreate(obj, CERTFILE, CERTFILE);
    assert(server_creds != NULL);
    client_creds = heimdall_tls_ClientCreate(obj);
    assert(client_creds != NULL);

    /* Test unknown certificate */
    tls = securepair(&th, alpn, alpn, &alp);
    assert(tls == NULL);
    tls = securepair(&th, alpn, alpn, NULL);
    assert(tls == NULL);

    heimdall_tls_ClientDelete(client_creds);
    heimdall_tls_ServerDelete(server_creds);
    libheimdall_release(heimdall);

    /*** Tests with test certs database - server cert accepted. ***/
    heimdall = libheimdall_new(ARRAY_SIZE(test_cert_argv) - 1, test_cert_argv);
    if (heimdall == NULL)
    {
        libheimdall_release(heimdall);
        return 77;
    }
    obj = HEIMDALL_OBJECT(heimdall->p_libheimdall_int);

    server_creds = heimdall_tls_ServerCreate(obj, CERTFILE, NULL);
    assert(server_creds != NULL);
    client_creds = heimdall_tls_ClientCreate(obj);
    assert(client_creds != NULL);

    /* Test known certificate */
    tls = securepair(&th, alpn, alpn, &alp);
    assert(tls != NULL);
    assert(alp != NULL);
    assert(strcmp(alp, alpn[0]) == 0);
    free(alp);

    /* Do some I/O */
    char buf[12];
    struct iovec iov;

    iov.iov_base = buf;
    iov.iov_len = sizeof (buf);
    val = tls->ops->readv(tls, &iov, 1);
    assert(val == -1 && errno == EAGAIN);

    val = heimdall_tls_Write(tls, "Hello ", 6);
    assert(val == 6);
    val = heimdall_tls_Write(tls, "world!", 6);
    assert(val == 6);

    val = heimdall_tls_Read(tls, buf, sizeof (buf), true);
    assert(val == 12);
    assert(!memcmp(buf, "Hello world!", 12));

    val = heimdall_tls_Shutdown(tls, false);
    assert(val == 0);
    heimdall_join(th, &p);
    assert(p != NULL);
    val = heimdall_tls_Read(tls, buf, sizeof (buf), false);
    assert(val == 0);
    heimdall_tls_Close(tls);

    /* Test known certificate, ignore ALPN result */
    tls = securepair(&th, alpn, alpn, NULL);
    assert(tls != NULL);

    /* Do a lot of I/O, test congestion handling */
    static unsigned char data[16184];
    size_t bytes = 0;
    unsigned seed = 0;

    iov.iov_base = data;
    iov.iov_len = sizeof (data);

    do
    {
        for (size_t i = 0; i < sizeof (data); i++)
            data[i] = rand_r(&seed);
        bytes += sizeof (data);
    }
    while ((val = tls->ops->writev(tls, &iov, 1)) == sizeof (data));

    bytes -= sizeof (data);
    if (val > 0)
        bytes += val;

    fprintf(stderr, "Sent %zu bytes.\n", bytes);
    seed = 0;

    while (bytes > 0)
    {
        unsigned char c = rand_r(&seed);

        val = heimdall_tls_Read(tls, buf, 1, false);
        assert(val == 1);
        assert(c == (unsigned char)buf[0]);
        bytes--;
    }

    heimdall_tls_Close(tls);
    heimdall_join(th, NULL);

    /* Test known certificate, no ALPN */
    tls = securepair(&th, alpn, NULL, &alp);
    assert(tls != NULL);
    assert(alp == NULL);
    heimdall_tls_Close(tls);
    heimdall_join(th, NULL);

    tls = securepair(&th, NULL, alpn, NULL);
    assert(tls != NULL);
    assert(alp == NULL);
    heimdall_tls_Close(tls);
    heimdall_join(th, NULL);

    /* Test ALPN combinations */
    tls = securepair(&th, alpn, alpn + 1, &alp);
    assert(tls != NULL);
    assert(alp != NULL);
    assert(strcmp(alp, alpn[1]) == 0);
    free(alp);
    heimdall_tls_Close(tls);
    heimdall_join(th, NULL);

    tls = securepair(&th, alpn + 1, alpn, &alp);
    assert(tls != NULL);
    assert(alp != NULL);
    assert(strcmp(alp, alpn[1]) == 0);
    free(alp);
    heimdall_tls_Close(tls);
    heimdall_join(th, NULL);

    /* Test ALPN mismatch */
    tls = securepair(&th, alpn, alpn_bad, &alp);
    assert(tls != NULL);
    assert(alp == NULL); /* currently, ALPN is marked optional in hello */
    heimdall_tls_Close(tls);
    heimdall_join(th, NULL);

    heimdall_tls_ClientDelete(client_creds);
    heimdall_tls_ServerDelete(server_creds);
    libheimdall_release(heimdall);

    return 0;
}
