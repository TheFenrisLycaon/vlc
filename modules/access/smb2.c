/*****************************************************************************
 * smb2.c: SMB2 access plug-in
 *****************************************************************************
 * Copyright © 2018  VLC authors, VideoLAN and VideoLabs
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

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_POLL_H
# include <poll.h>
#endif

#include <heimdall_common.h>
#include <heimdall_access.h>
#include <heimdall_dialog.h>
#include <heimdall_input_item.h>
#include <heimdall_plugin.h>
#include <heimdall_url.h>
#include <heimdall_keystore.h>
#include <heimdall_interrupt.h>
#include <heimdall_network.h>
#include <heimdall_memstream.h>

#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include <smb2/libsmb2-raw.h>

#ifdef HAVE_DSM
# include <bdsm/bdsm.h>

#if BDSM_VERSION_CURRENT >= 5

static void
netbios_ns_interrupt_callback(void *data)
{
    netbios_ns_abort(data);
}

static inline void
netbios_ns_interrupt_register(netbios_ns *ns)
{
    heimdall_interrupt_register(netbios_ns_interrupt_callback, ns);
}

static inline int
netbios_ns_interrupt_unregister(void)
{
    return heimdall_interrupt_unregister();
}

#else

static inline void
netbios_ns_interrupt_register(netbios_ns *ns)
{
    (void) ns;
}

static inline int
netbios_ns_interrupt_unregister(void)
{
    return 0;
}

#endif

#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include "smb_common.h"
#include "cache.h"

static int Open(heimdall_object_t *);
static void Close(heimdall_object_t *);

heimdall_module_begin()
    set_shortname("smb2")
    set_description(N_("SMB2 / SMB3 input"))
    set_help(N_("Samba (Windows network shares) input via libsmb2"))
    set_capability("access", 21)
    set_subcategory(SUBCAT_INPUT_ACCESS)
    add_string("smb-user", NULL, SMB_USER_TEXT, SMB_USER_LONGTEXT)
    add_password("smb-pwd", NULL, SMB_PASS_TEXT, SMB_PASS_LONGTEXT)
    add_string("smb-domain", NULL, SMB_DOMAIN_TEXT, SMB_DOMAIN_LONGTEXT)
    add_shortcut("smb", "smb2")
    set_callbacks(Open, Close)
heimdall_module_end()

HEIMDALL_ACCESS_CACHE_REGISTER(smb2_cache);

struct access_sys
{
    struct smb2_context *   smb2;
    struct smb2fh *         smb2fh;
    struct smb2dir *        smb2dir;
#ifdef LIBSMB2_SHARE_ENUM_V2
    struct srvsvc_NetrShareEnum_rep *share_enum;
#else
    struct srvsvc_netshareenumall_rep *share_enum;
#endif
    uint64_t                smb2_size;
    heimdall_url_t               encoded_url;
    bool                    eof;
    bool                    smb2_connected;

    struct heimdall_access_cache_entry *cache_entry;
};

struct heimdall_smb2_op
{
    struct heimdall_logger *log;

    struct smb2_context *smb2;
    struct smb2_context **smb2p;

    int error_status;

    bool res_done;
    union {
        struct
        {
            size_t len;
        } read;
        void *data;
    } res;
};

#define HEIMDALL_SMB2_OP(access, smb2p_) { \
    .log = access ? heimdall_object_logger(access) : NULL, \
    .smb2p = smb2p_, \
    .smb2 = (assert(*smb2p_ != NULL), *smb2p_), \
    .error_status = 0, \
    .res_done = false, \
};

static inline void
heimdall_smb2_op_reset(struct heimdall_smb2_op *op, struct smb2_context **smb2p)
{
    op->res_done = false;
    op->smb2p = smb2p;
    op->smb2 = *smb2p;
    op->error_status = 0;
}

static int
heimdall_smb2_check_status(struct heimdall_smb2_op *op, const char *psz_func, int status)
{
    if (status < 0)
    {
        const char *psz_error = smb2_get_error(op->smb2);
        if (op->log)
            heimdall_warning(op->log, "%s failed: %d, '%s'", psz_func, status, psz_error);
        op->error_status = status;
        return -1;
    }
    else
    {
        op->res_done = true;
        return 0;
    }
}

static void
heimdall_smb2_set_error(struct heimdall_smb2_op *op, const char *psz_func, int err)
{
    if (op->log && err != -EINTR)
        heimdall_error(op->log, "%s failed: %d, %s", psz_func, err, smb2_get_error(op->smb2));

    /* Don't override if set via heimdall_smb2_check_status */
    if (op->error_status == 0)
        op->error_status = err;

    smb2_destroy_context(op->smb2);
    op->smb2 = NULL;
    *op->smb2p = NULL;
}

#define HEIMDALL_SMB2_CHECK_STATUS(op, status) \
    heimdall_smb2_check_status(op, __func__, status)

#define HEIMDALL_SMB2_SET_ERROR(op, func, err) \
    heimdall_smb2_set_error(op, func, err)

#define HEIMDALL_SMB2_STATUS_DENIED(x) (x == -ECONNREFUSED || x == -EACCES)

static int
heimdall_smb2_mainloop(struct heimdall_smb2_op *op)
{
    while (op->error_status == 0 && !op->res_done)
    {
        int ret, smb2_timeout;
        size_t fd_count;
        const t_socket *fds = smb2_get_fds(op->smb2, &fd_count, &smb2_timeout);
        int events = smb2_which_events(op->smb2);

        struct pollfd p_fds[fd_count];
        for (size_t i = 0; i < fd_count; ++i)
        {
            p_fds[i].events = events;
            p_fds[i].fd = fds[i];
        }

        if (fds == NULL || (ret = heimdall_poll_i11e(p_fds, fd_count, smb2_timeout)) < 0)
        {
            if (op->log && errno == EINTR)
                heimdall_warning(op->log, "heimdall_poll_i11e interrupted");
            HEIMDALL_SMB2_SET_ERROR(op, "poll", -errno);
        }
        else if (ret == 0)
        {
            if (smb2_service_fd(op->smb2, -1, 0) < 0)
                HEIMDALL_SMB2_SET_ERROR(op, "smb2_service", -EINVAL);
        }
        else
        {
            for (size_t i = 0; i < fd_count; ++i)
            {
                if (p_fds[i].revents
                 && smb2_service_fd(op->smb2, p_fds[i].fd, p_fds[i].revents) < 0)
                    HEIMDALL_SMB2_SET_ERROR(op, "smb2_service", -EINVAL);
            }
        }
    }

    if (op->error_status != 0 && op->smb2 != NULL)
    {
        /* An error was signalled from a smb2 cb. Destroy the smb2 context now
         * since this call might still trigger callbacks using the current op
         * (that is allocated on the stack). */
        smb2_destroy_context(op->smb2);
        op->smb2 = NULL;
        *op->smb2p = NULL;
    }

    return op->error_status;
}

#define HEIMDALL_SMB2_GENERIC_CB() \
    struct heimdall_smb2_op *op = private_data; \
    assert(op->smb2 == smb2); (void) smb2; \
    if (HEIMDALL_SMB2_CHECK_STATUS(op, status)) \
        return

static void
smb2_generic_cb(struct smb2_context *smb2, int status, void *data,
                void *private_data)
{
    HEIMDALL_UNUSED(data);
    HEIMDALL_SMB2_GENERIC_CB();
}

static void
smb2_read_cb(struct smb2_context *smb2, int status, void *data,
             void *private_data)
{
    HEIMDALL_UNUSED(data);
    HEIMDALL_SMB2_GENERIC_CB();

    op->res.read.len = status;
}

static ssize_t
FileRead(stream_t *access, void *buf, size_t len)
{
    struct access_sys *sys = access->p_sys;

    if (sys->eof || sys->smb2 == NULL)
        return 0;

    /* Limit the read size since smb2_read_async() will complete only after
     * reading the whole requested data and not when whatever data is available
     * (high read size means a faster I/O but a higher latency). */
    if (len > 262144)
        len = 262144;

    struct heimdall_smb2_op op = HEIMDALL_SMB2_OP(access, &sys->smb2);
    op.res.read.len = 0;

    int err = smb2_read_async(sys->smb2, sys->smb2fh, buf, len,
                              smb2_read_cb, &op);
    if (err < 0)
    {
        HEIMDALL_SMB2_SET_ERROR(&op, "smb2_read_async", err);
        return 0;
    }

    if (heimdall_smb2_mainloop(&op) < 0)
        return 0;

    if (op.res.read.len == 0)
        sys->eof = true;

    return op.res.read.len;
}

static int
FileSeek(stream_t *access, uint64_t i_pos)
{
    struct access_sys *sys = access->p_sys;

    if (sys->smb2 == NULL)
        return HEIMDALL_EGENERIC;

    if (i_pos > INT64_MAX)
    {
        msg_Err(access, "can't seek past INT64_MAX (requested: %"PRIu64")\n",
                i_pos);
        return HEIMDALL_EGENERIC;
    }

    struct heimdall_smb2_op op = HEIMDALL_SMB2_OP(access, &sys->smb2);

    int64_t err = smb2_lseek(op.smb2, sys->smb2fh, i_pos, SEEK_SET, NULL);
    if (err < 0)
    {
        HEIMDALL_SMB2_SET_ERROR(&op, "smb2_lseek", err);
        return err;
    }
    sys->eof = false;

    return HEIMDALL_SUCCESS;
}

static int
FileControl(stream_t *access, int i_query, va_list args)
{
    struct access_sys *sys = access->p_sys;

    switch (i_query)
    {
        case STREAM_CAN_SEEK:
            *va_arg(args, bool *) = true;
            break;

        case STREAM_CAN_FASTSEEK:
            *va_arg(args, bool *) = false;
            break;

        case STREAM_CAN_PAUSE:
        case STREAM_CAN_CONTROL_PACE:
        case STREAM_CAN_DOWNLOAD:
            *va_arg(args, bool *) = true;
            break;

        case STREAM_GET_SIZE:
        {
            *va_arg(args, uint64_t *) = sys->smb2_size;
            break;
        }

        case STREAM_GET_PTS_DELAY:
            *va_arg(args, heimdall_tick_t * ) = HEIMDALL_TICK_FROM_MS(
                var_InheritInteger(access, "network-caching"));
            break;

        case STREAM_SET_PAUSE_STATE:
            break;

        default:
            return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

HEIMDALL_MALLOC static char *
heimdall_smb2_get_url(heimdall_url_t *url, const char *file)
{
    /* smb2://<psz_host><i_port><psz_path><file>?<psz_option> */
    struct heimdall_memstream buf;
    heimdall_memstream_open(&buf);
    if (strchr(url->psz_host, ':') != NULL)
        heimdall_memstream_printf(&buf, "smb://[%s]", url->psz_host);
    else
        heimdall_memstream_printf(&buf, "smb://%s", url->psz_host);

    if (url->i_port != 0)
        heimdall_memstream_printf(&buf, ":%d", url->i_port);

    if (url->psz_path != NULL)
    {
        heimdall_memstream_puts(&buf, url->psz_path);
        if (url->psz_path[0] != '\0' && url->psz_path[strlen(url->psz_path) - 1] != '/')
            heimdall_memstream_putc(&buf, '/');
    }
    else
        heimdall_memstream_putc(&buf, '/');

    heimdall_memstream_puts(&buf, file);

    if (url->psz_option)
        heimdall_memstream_printf(&buf, "?%s", url->psz_option);

    if (heimdall_memstream_close(&buf))
        return NULL;
    return buf.ptr;
}

static int AddItem(stream_t *access, struct heimdall_readdir_helper *rdh,
                   const char* name, int i_type, struct smb2_stat_64 *stats)
{
    struct access_sys *sys = access->p_sys;
    char *name_encoded = heimdall_uri_encode(name);
    if (name_encoded == NULL)
        return HEIMDALL_ENOMEM;

    char *url = heimdall_smb2_get_url(&sys->encoded_url, name_encoded);
    free(name_encoded);
    if (url == NULL)
        return HEIMDALL_ENOMEM;

    input_item_t *p_item;
    int ret = heimdall_readdir_helper_additem(rdh, url, NULL, name, i_type,
                                         ITEM_NET, &p_item);

    if (ret == HEIMDALL_SUCCESS && p_item && stats)
    {
        input_item_AddStat( p_item, "mtime", stats->smb2_mtime);
        input_item_AddStat( p_item, "size", stats->smb2_size);
    }
    free(url);
    return ret;
}

static int
DirRead(stream_t *access, input_item_node_t *p_node)
{
    struct access_sys *sys = access->p_sys;
    struct smb2dirent *smb2dirent;
    int ret = HEIMDALL_SUCCESS;
    assert(sys->smb2dir);

    struct heimdall_readdir_helper rdh;
    heimdall_readdir_helper_init(&rdh, access, p_node);

    while (ret == HEIMDALL_SUCCESS
        && (smb2dirent = smb2_readdir(sys->smb2, sys->smb2dir)) != NULL)
    {
        int i_type;
        switch (smb2dirent->st.smb2_type)
        {
        case SMB2_TYPE_FILE:
            i_type = ITEM_TYPE_FILE;
            break;
        case SMB2_TYPE_DIRECTORY:
            i_type = ITEM_TYPE_DIRECTORY;
            break;
        default:
            i_type = ITEM_TYPE_UNKNOWN;
            break;
        }
        ret = AddItem(access, &rdh, smb2dirent->name, i_type, &smb2dirent->st);
    }

    heimdall_readdir_helper_finish(&rdh, ret == HEIMDALL_SUCCESS);

    return ret;
}

static int
ShareEnum(stream_t *access, input_item_node_t *p_node)
{
    struct access_sys *sys = access->p_sys;
    assert(sys->share_enum != NULL);

    int ret = HEIMDALL_SUCCESS;
    struct heimdall_readdir_helper rdh;
    heimdall_readdir_helper_init(&rdh, access, p_node);

#ifdef LIBSMB2_SHARE_ENUM_V2
    struct srvsvc_SHARE_INFO_1_CONTAINER *ctr = &sys->share_enum->ses.ShareInfo.Level1;
    size_t ctr_count = ctr->EntriesRead;
#else
    struct srvsvc_netsharectr *ctr = sys->share_enum->ctr;
    size_t ctr_count = ctr->ctr1.count;
#endif

    for (uint32_t iinfo = 0;
         iinfo < ctr_count && ret == HEIMDALL_SUCCESS; ++iinfo)
    {
#ifdef LIBSMB2_SHARE_ENUM_V2
       struct srvsvc_SHARE_INFO_1 *info = &ctr->Buffer->share_info_1[iinfo];
       const char *name = info->netname.utf8;
       uint32_t type = info->type;
#else
       struct srvsvc_netshareinfo1 *info = &ctr->ctr1.array[iinfo];
       const char *name = info->name;
       uint32_t type = info->type;
#endif

       if (type & SHARE_TYPE_HIDDEN)
           continue;
       switch (type & 0x3)
       {
           case SHARE_TYPE_DISKTREE:
               ret = AddItem(access, &rdh, name, ITEM_TYPE_DIRECTORY, NULL);
               break;
       }
    }

    heimdall_readdir_helper_finish(&rdh, ret == HEIMDALL_SUCCESS);
    return 0;
}

static int
heimdall_smb2_close_fh(stream_t *access, struct smb2_context **smb2p,
                  struct smb2fh *smb2fh)
{
    struct heimdall_smb2_op op = HEIMDALL_SMB2_OP(access, smb2p);

    int err = smb2_close_async(op.smb2, smb2fh, smb2_generic_cb, &op);
    if (err < 0)
    {
        HEIMDALL_SMB2_SET_ERROR(&op, "smb2_close_async", err);
        return -1;
    }

    return heimdall_smb2_mainloop(&op);
}

static int
heimdall_smb2_disconnect_share(stream_t *access, struct smb2_context **smb2p)
{
    struct heimdall_smb2_op op = HEIMDALL_SMB2_OP(access, smb2p);

    int err = smb2_disconnect_share_async(op.smb2, smb2_generic_cb, &op);
    if (err < 0)
    {
        HEIMDALL_SMB2_SET_ERROR(&op, "smb2_connect_share_async", err);
        return -1;
    }

    return heimdall_smb2_mainloop(&op);
}

static void
smb2_open_cb(struct smb2_context *smb2, int status, void *data,
             void *private_data)
{
    HEIMDALL_SMB2_GENERIC_CB();

    op->res.data = data;
}

static void
heimdall_smb2_print_addr(stream_t *access)
{
    struct access_sys *sys = access->p_sys;

    heimdall_sockaddr addr;
    if (getsockname(smb2_get_fd(sys->smb2), &addr.sa,
                    &(socklen_t){ sizeof(addr.ss) }) != 0)
        return;

    const void *sin_addr;
    switch (addr.ss.ss_family)
    {
        case AF_INET6:
            sin_addr = &addr.sin6.sin6_addr;
            break;
        case AF_INET:
            sin_addr = &addr.sin.sin_addr;
            break;
        default:
            return;
    }
    char ip[INET6_ADDRSTRLEN];
    if (inet_ntop(addr.ss.ss_family, sin_addr, ip, sizeof(ip)) == NULL)
        return;

    if (strcmp(ip, sys->encoded_url.psz_host) == 0)
        return;

    msg_Dbg(access, "%s: connected from %s\n", sys->encoded_url.psz_host, ip);
}

static int
heimdall_smb2_open_share(stream_t *access, struct smb2_context **smb2p,
                    struct smb2_url *smb2_url, bool do_enum)
{
    struct access_sys *sys = access->p_sys;
    struct smb2_stat_64 smb2_stat;

    struct heimdall_smb2_op op = HEIMDALL_SMB2_OP(access, smb2p);

    int ret;
    if (do_enum)
    {
#ifdef LIBSMB2_SHARE_ENUM_V2
        ret = smb2_share_enum_async(op.smb2, SHARE_INFO_1, smb2_open_cb, &op);
#else
        ret = smb2_share_enum_async(op.smb2, smb2_open_cb, &op);
#endif
    }
    else
    {
        ret = smb2_stat_async(op.smb2, smb2_url->path, &smb2_stat,
                              smb2_generic_cb, &op);
        if (ret < 0)
        {
            HEIMDALL_SMB2_SET_ERROR(&op, "smb2_stat_async", ret);
            goto error;
        }

        if (heimdall_smb2_mainloop(&op) != 0)
            goto error;

        if (smb2_stat.smb2_type == SMB2_TYPE_FILE)
        {
            heimdall_smb2_op_reset(&op, smb2p);

            sys->smb2_size = smb2_stat.smb2_size;
            ret = smb2_open_async(op.smb2, smb2_url->path, O_RDONLY,
                                  smb2_open_cb, &op);
        }
        else if (smb2_stat.smb2_type == SMB2_TYPE_DIRECTORY)
        {
            heimdall_smb2_op_reset(&op, smb2p);

            ret = smb2_opendir_async(op.smb2, smb2_url->path, smb2_open_cb, &op);
        }
        else
        {
            msg_Err(access, "smb2_stat_cb: file type not handled");
            ret = -ENOENT;
        }
    }

    if (ret < 0)
    {
        HEIMDALL_SMB2_SET_ERROR(&op, "smb2_open*_async", ret);
        goto error;
    }

    if (heimdall_smb2_mainloop(&op) != 0)
        goto error;

    if (do_enum)
        sys->share_enum = op.res.data;
    else if (smb2_stat.smb2_type == SMB2_TYPE_FILE)
        sys->smb2fh = op.res.data;
    else if (smb2_stat.smb2_type == SMB2_TYPE_DIRECTORY)
        sys->smb2dir = op.res.data;
    else
        heimdall_assert_unreachable();

    return 0;
error:
    return op.error_status;
}

static void
heimdall_smb2_FreeContext(void *context)
{
    struct smb2_context *smb2 = context;

    heimdall_smb2_disconnect_share(NULL, &smb2);
    if (smb2 != NULL)
        smb2_destroy_context(smb2);
}

static int
heimdall_smb2_connect_share(stream_t *access, const char *server,
                       const char *share, const char *username)
{
    struct access_sys *sys = access->p_sys;

    struct heimdall_smb2_op op = HEIMDALL_SMB2_OP(access, &sys->smb2);
    int err = smb2_connect_share_async(sys->smb2, server, share,
                                       username, smb2_generic_cb, &op);
    if (err < 0)
    {
        HEIMDALL_SMB2_SET_ERROR(&op, "smb2_connect_share_async", err);
        return op.error_status;
    }

    return heimdall_smb2_mainloop(&op);
}

static int
heimdall_smb2_connect_open_share(stream_t *access, const char *url,
                            const heimdall_credential *credential,
                            bool guest_with_valid_passwd)
{
    struct access_sys *sys = access->p_sys;

    struct smb2_url *smb2_url = NULL;

    int err;
    sys->smb2 = smb2_init_context();
    if (sys->smb2 == NULL)
    {
        msg_Err(access, "smb2_init_context failed");
        return -ENOMEM;
    }
    smb2_url = smb2_parse_url(sys->smb2, url);

    if (!smb2_url || !smb2_url->share || !smb2_url->server)
    {
        msg_Err(access, "smb2_parse_url failed");
        err = -EINVAL;
        goto error;
    }

    const bool do_enum = smb2_url->share[0] == '\0';
    const char *username = credential->psz_username;
    const char *password = credential->psz_password;
    const char *domain = credential->psz_realm;
    const char *share = do_enum ? "IPC$" : smb2_url->share;
    if (!username)
    {
        username = "Guest";
        /* A NULL password enable ntlmssp anonymous login */
        password = guest_with_valid_passwd ? "" : NULL;
    }

    struct heimdall_access_cache_entry *cache_entry =
        heimdall_access_cache_GetSmbEntry(&smb2_cache, smb2_url->server, share,
                                     credential->psz_username);
    if (cache_entry != NULL)
    {
        struct smb2_context *smb2 = cache_entry->context;
        err = heimdall_smb2_open_share(access, &smb2, smb2_url, do_enum);
        if (err == 0)
        {
            assert(smb2 != NULL);

            smb2_destroy_context(sys->smb2);
            sys->smb2 = cache_entry->context;
            sys->smb2_connected = true;
            sys->cache_entry = cache_entry;

            smb2_destroy_url(smb2_url);
            msg_Dbg(access, "re-using old smb2 session");
            return 0;
        }
        else
            heimdall_access_cache_entry_Delete(cache_entry);
    }

    smb2_set_security_mode(sys->smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);
    smb2_set_password(sys->smb2, password);
    smb2_set_domain(sys->smb2, domain ? domain : "");

    err = heimdall_smb2_connect_share(access, smb2_url->server, share, username);
    if (err < 0)
        goto error;

    sys->smb2_connected = true;

    heimdall_smb2_print_addr(access);

    err = heimdall_smb2_open_share(access, &sys->smb2, smb2_url, do_enum);
    if (err < 0)
        goto error;

    sys->cache_entry = heimdall_access_cache_entry_NewSmb(sys->smb2, smb2_url->server, share,
                                                     credential->psz_username,
                                                     heimdall_smb2_FreeContext);
    if (sys->cache_entry == NULL)
    {
        err = -ENOMEM;
        goto error;
    }

    smb2_destroy_url(smb2_url);
    return 0;

error:
    if (smb2_url != NULL)
        smb2_destroy_url(smb2_url);

    if (sys->smb2 != NULL)
    {
        if (sys->smb2_connected)
        {
            heimdall_smb2_disconnect_share(access, &sys->smb2);
            sys->smb2_connected = false;
        }
        if (sys->smb2 != NULL)
        {
            smb2_destroy_context(sys->smb2);
            sys->smb2 = NULL;
        }
    }
    return err;
}

static int
heimdall_smb2_resolve(stream_t *access, const char *host, unsigned port,
                 char **out_host)
{
    (void) access;
    if (!host)
        return -ENOENT;

#ifdef HAVE_DSM
    /* Test if the host is an IP */
    struct in_addr addr;
    if (inet_pton(AF_INET, host, &addr) == 1)
        return -ENOENT;

    /* Test if the host can be resolved */
    struct addrinfo *info = NULL;
    if (heimdall_getaddrinfo_i11e(host, port, NULL, &info) == 0)
    {
        freeaddrinfo(info);
        /* Let smb2 resolve it */
        return -ENOENT;
    }

    /* Test if the host is a netbios name */
    netbios_ns *ns = netbios_ns_new();
    if (!ns)
        return -ENOMEM;

    int ret = -ENOENT;
    netbios_ns_interrupt_register(ns);
    uint32_t ip4_addr;
    if (netbios_ns_resolve(ns, host, NETBIOS_FILESERVER, &ip4_addr) == 0)
    {
        char ip[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &ip4_addr, ip, sizeof(ip)))
        {
            *out_host = strdup(ip);
            ret = 0;
        }
    }
    if (netbios_ns_interrupt_unregister() == EINTR)
    {
        if (unlikely(ret == 0))
            free(*out_host);
        netbios_ns_destroy(ns);
        return -EINTR;
    }
    netbios_ns_destroy(ns);
    return ret;
#else
    (void) port;
    return -ENOENT;
#endif
}

static int
Open(heimdall_object_t *p_obj)
{
    stream_t *access = (stream_t *)p_obj;
    struct access_sys *sys = heimdall_obj_calloc(p_obj, 1, sizeof (*sys));
    char *var_domain = NULL;
    int ret;

    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;
    access->p_sys = sys;

    /* Parse the encoded URL */
    if (heimdall_UrlParseFixup(&sys->encoded_url, access->psz_url) != 0)
        return HEIMDALL_ENOMEM;

    if (sys->encoded_url.psz_path == NULL)
        sys->encoded_url.psz_path = (char *) "/";

    char *resolved_host = NULL;
    ret = heimdall_smb2_resolve(access, sys->encoded_url.psz_host,
                           sys->encoded_url.i_port, &resolved_host);

    /* smb2_* functions need a decoded url. Re compose the url from the
     * modified sys->encoded_url (with the resolved host). */
    char *url;
    if (ret == -EINTR)
        goto error;
    else if (ret == 0)
    {
        heimdall_url_t resolved_url = sys->encoded_url;
        resolved_url.psz_host = resolved_host;
        url = heimdall_uri_compose(&resolved_url);
    }
    else
    {
        url = heimdall_uri_compose(&sys->encoded_url);
    }
    if (!heimdall_uri_decode(url))
    {
        free(url);
        free(resolved_host);
        ret = -ENOMEM;
        goto error;
    }

    heimdall_credential credential;
    heimdall_credential_init(&credential, &sys->encoded_url);
    var_domain = var_InheritString(access, "smb-domain");
    credential.psz_realm = var_domain;

    /* First, try Guest login or using "smb-" options (without
     * keystore/user interaction) */
    if (heimdall_credential_get(&credential, access, "smb-user", "smb-pwd", NULL,
                           NULL) == -EINTR)
    {
        heimdall_credential_clean(&credential);
        free(resolved_host);
        ret = -EINTR;
        goto error;
    }

    ret = heimdall_smb2_connect_open_share(access, url, &credential, false);
    if (ret == -EINVAL && credential.psz_username == NULL)
    {
        /* Since last Windows 11 update (KB5026436), Windows SMB servers need a
         * valid Auth (user + password) even for a guest/anonymous login. The
         * server will return 'STATUS_INVALID_PARAMETER' (so, libsmb2 will
         * return '-EINVAL') if the password is invalid. Therefore, try to
         * connect again with a valid password in that case.
         *
         * We don't try to connect with a valid password on the first try since
         * it seems to break anonymous login with other samba servers (but
         * samba.c doesn't have this problem so this might be libsmb2 issue).
         * */
        ret = heimdall_smb2_connect_open_share(access, url, &credential, true);
    }

    while (HEIMDALL_SMB2_STATUS_DENIED(ret)
        && heimdall_credential_get(&credential, access, "smb-user", "smb-pwd",
                              SMB_LOGIN_DIALOG_TITLE, SMB_LOGIN_DIALOG_TEXT,
                              sys->encoded_url.psz_host) == 0)
        ret = heimdall_smb2_connect_open_share(access, url, &credential, false);
    free(resolved_host);
    free(url);
    if (ret == 0)
        heimdall_credential_store(&credential, access);
    heimdall_credential_clean(&credential);

    if (ret != 0)
    {
        const char *error = smb2_get_error(sys->smb2);
        if (error && *error)
            heimdall_dialog_display_error(access,
                                     _("SMB2 operation failed"), "%s", error);
        if (credential.i_get_order == GET_FROM_DIALOG)
        {
            /* Tell other smb modules (likely dsm) that we already requested
             * credential to the users and that it it useless to try again.
             * This avoid to show 2 login dialogs for the same access. */
            var_Create(access, "smb-dialog-failed", HEIMDALL_VAR_VOID);
        }
        goto error;
    }

    if (sys->smb2fh != NULL)
    {
        access->pf_read = FileRead;
        access->pf_seek = FileSeek;
        access->pf_control = FileControl;
    }
    else if (sys->smb2dir != NULL)
    {
        access->pf_readdir = DirRead;
        access->pf_seek = NULL;
        access->pf_control = access_vaDirectoryControlHelper;
    }
    else if (sys->share_enum != NULL)
    {
        access->pf_readdir = ShareEnum;
        access->pf_seek = NULL;
        access->pf_control = access_vaDirectoryControlHelper;
    }
    else
        heimdall_assert_unreachable();

    free(var_domain);
    return HEIMDALL_SUCCESS;

error:
    heimdall_UrlClean(&sys->encoded_url);
    free(var_domain);

    /* Returning HEIMDALL_ETIMEOUT will stop the module probe and prevent to load
     * the next smb module. The smb2 module can return this specific error in
     * case of network error (EIO) or when the user asked to cancel it
     * (heimdall_killed()). Indeed, in these cases, it is useless to try next smb
     * modules. */
    return ret == -EINTR || ret == -EIO || heimdall_killed() ? HEIMDALL_ETIMEOUT : HEIMDALL_EGENERIC;
}

static void
Close(heimdall_object_t *p_obj)
{
    stream_t *access = (stream_t *)p_obj;
    struct access_sys *sys = access->p_sys;

    if (sys->smb2fh != NULL)
    {
        if (sys->smb2)
            heimdall_smb2_close_fh(access, &sys->smb2, sys->smb2fh);
    }
    else if (sys->smb2dir != NULL)
        smb2_closedir(sys->smb2, sys->smb2dir);
    else if (sys->share_enum != NULL)
        smb2_free_data(sys->smb2, sys->share_enum);
    else
        heimdall_assert_unreachable();

    assert(sys->smb2_connected);

    if (sys->smb2 != NULL)
        heimdall_access_cache_AddEntry(&smb2_cache, sys->cache_entry);
    else
        heimdall_access_cache_entry_Delete(sys->cache_entry);

    heimdall_UrlClean(&sys->encoded_url);
}
