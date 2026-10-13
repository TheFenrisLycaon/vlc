/*****************************************************************************
 * keystore.c:
 *****************************************************************************
 * Copyright (C) 2015-2016  VLC authors, VideoLAN and VideoLabs
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
# include <config.h>
#endif

#include <heimdall_common.h>
#include <heimdall_dialog.h>
#include <heimdall_keystore.h>
#include <heimdall_modules.h>
#include <heimdall_url.h>
#include <heimdall_interrupt.h>
#include "../libheimdall.h"

#include <assert.h>
#include <limits.h>

static heimdall_keystore *
keystore_create(heimdall_object_t *p_parent, const char *psz_name)
{
    heimdall_keystore *p_keystore = heimdall_custom_create(p_parent, sizeof (*p_keystore),
                                                 "keystore");
    if (unlikely(p_keystore == NULL))
        return NULL;

    p_keystore->p_module = module_need(p_keystore, "keystore", psz_name, true);
    if (p_keystore->p_module == NULL)
    {
        heimdall_object_delete(p_keystore);
        return NULL;
    }
    assert(p_keystore->pf_store);
    assert(p_keystore->pf_find);
    assert(p_keystore->pf_remove);

    return p_keystore;
}

#undef heimdall_keystore_create
heimdall_keystore *
heimdall_keystore_create(heimdall_object_t *p_parent)
{
    assert(p_parent);

    char *modlist = var_InheritString(p_parent, "keystore");
    heimdall_keystore *p_keystore = keystore_create(p_parent, modlist);

    free(modlist);
    return p_keystore;
}

void
heimdall_keystore_release(heimdall_keystore *p_keystore)
{
    assert(p_keystore);
    module_unneed(p_keystore, p_keystore->p_module);

    heimdall_object_delete(p_keystore);
}

int
heimdall_keystore_store(heimdall_keystore *p_keystore,
                   const char * const ppsz_values[KEY_MAX],
                   const uint8_t *p_secret, ssize_t i_secret_len,
                   const char *psz_label)
{
    assert(p_keystore && ppsz_values && p_secret && i_secret_len);

    if (!ppsz_values[KEY_PROTOCOL] || !ppsz_values[KEY_SERVER])
    {
        msg_Err(p_keystore, "invalid store request: "
                "protocol and server should be valid");
        return HEIMDALL_EGENERIC;
    }
    if (ppsz_values[KEY_PORT])
    {
        long int i_port = strtol(ppsz_values[KEY_PORT], NULL, 10);
        if (i_port == LONG_MIN || i_port == LONG_MAX)
        {
            msg_Err(p_keystore, "invalid store request: "
                    "port is not valid number");
            return HEIMDALL_EGENERIC;
        }
    }
    if (i_secret_len < 0)
        i_secret_len = strlen((const char *)p_secret) + 1;
    return p_keystore->pf_store(p_keystore, ppsz_values, p_secret, i_secret_len,
                                psz_label);
}

unsigned int
heimdall_keystore_find(heimdall_keystore *p_keystore,
                  const char * const ppsz_values[KEY_MAX],
                  heimdall_keystore_entry **pp_entries)
{
    assert(p_keystore && ppsz_values && pp_entries);
    return p_keystore->pf_find(p_keystore, ppsz_values, pp_entries);
}

unsigned int
heimdall_keystore_remove(heimdall_keystore *p_keystore,
                    const char *const ppsz_values[KEY_MAX])
{
    assert(p_keystore && ppsz_values);
    return p_keystore->pf_remove(p_keystore, ppsz_values);
}

void
heimdall_keystore_release_entries(heimdall_keystore_entry *p_entries, unsigned int i_count)
{
    for (unsigned int i = 0; i < i_count; ++i)
        heimdall_keystore_release_entry(&p_entries[i]);
    free(p_entries);
}

int
libheimdall_InternalKeystoreInit(libheimdall_int_t *p_libheimdall)
{
    assert(p_libheimdall != NULL);
    libheimdall_priv_t *p_priv = libheimdall_priv(p_libheimdall);

    p_priv->p_memory_keystore = keystore_create(HEIMDALL_OBJECT(p_libheimdall), "memory");
    return p_priv->p_memory_keystore != NULL ? HEIMDALL_SUCCESS : HEIMDALL_EGENERIC;
}

void
libheimdall_InternalKeystoreClean(libheimdall_int_t *p_libheimdall)
{
    assert(p_libheimdall != NULL);
    libheimdall_priv_t *p_priv = libheimdall_priv(p_libheimdall);

    if (p_priv->p_memory_keystore != NULL)
    {
        heimdall_keystore_release(p_priv->p_memory_keystore);
        p_priv->p_memory_keystore = NULL;
    }
}

static heimdall_keystore *
get_memory_keystore(heimdall_object_t *p_obj)
{
    return libheimdall_priv(heimdall_object_instance(p_obj))->p_memory_keystore;
}

static heimdall_keystore_entry *
find_closest_path(heimdall_keystore_entry *p_entries, unsigned i_count,
                  const char *psz_path)
{
    heimdall_keystore_entry *p_match_entry = NULL;
    size_t i_last_pathlen = 0;
    char *psz_decoded_path = heimdall_uri_decode_duplicate(psz_path);
    if (psz_decoded_path == NULL)
        return NULL;

    /* Try to find the entry that has the closest path to psz_url */
    for (unsigned int i = 0; i < i_count; ++i)
    {
        heimdall_keystore_entry *p_entry = &p_entries[i];
        const char *psz_entry_path = p_entry->ppsz_values[KEY_PATH];
        if (psz_entry_path == NULL)
        {
            if (p_match_entry == NULL)
                p_match_entry = p_entry;
            continue;
        }
        size_t i_entry_pathlen = strlen(psz_entry_path);

        if (strncasecmp(psz_decoded_path, psz_entry_path, i_entry_pathlen) == 0
         && i_entry_pathlen > i_last_pathlen)
        {
            i_last_pathlen = i_entry_pathlen;
            p_match_entry = p_entry;
        }
    }
    free(psz_decoded_path);
    return p_match_entry;
}

static bool
is_credential_valid(heimdall_credential *p_credential)
{
    if (p_credential->psz_username && p_credential->psz_password)
        return true;
    p_credential->psz_password = NULL;
    return false;
}

static bool
is_url_valid(const heimdall_url_t *p_url)
{
    return p_url && p_url->psz_protocol && p_url->psz_protocol[0]
        && p_url->psz_host && p_url->psz_host[0];
}

/* Default port for each protocol */
static struct
{
    const char *    psz_protocol;
    uint16_t        i_port;
} protocol_default_ports [] = {
    { "rtsp", 80 },
    { "http", 80 },
    { "https", 443 },
    { "ftp", 21 },
    { "sftp", 22 },
    { "smb", 445 },
};

/* Don't store a port if it's the default one */
static bool
protocol_set_port(const heimdall_url_t *p_url, char *psz_port)
{
    int i_port = -1;

    if (p_url->i_port != 0 && p_url->i_port <= UINT16_MAX)
        i_port = p_url->i_port;
    else
    {
        for (unsigned int i = 0; i < ARRAY_SIZE(protocol_default_ports); ++i)
        {
            if (strcasecmp(p_url->psz_protocol,
                           protocol_default_ports[i].psz_protocol) == 0)
            {
                i_port = protocol_default_ports[i].i_port;
                break;
            }
        }
    }
    if (i_port != -1)
    {
        sprintf(psz_port, "%" PRIu16, (uint16_t) i_port);
        return true;
    }
    return false;
}

static bool
protocol_is_smb(const heimdall_url_t *p_url)
{
    return strcasecmp(p_url->psz_protocol, "smb") == 0;
}

static bool
protocol_store_path(const heimdall_url_t *p_url)
{
    return p_url->psz_path
      && (strncasecmp(p_url->psz_protocol, "http", 4) == 0
      || strcasecmp(p_url->psz_protocol, "rtsp") == 0
      || protocol_is_smb(p_url));
}

/* Split domain;user in userinfo */
static void
smb_split_domain(heimdall_credential *p_credential)
{
    const char *psz_delim = strchr(p_credential->psz_username, ';');
    if (psz_delim == NULL)
        return;

    size_t i_len = psz_delim - p_credential->psz_username;
    if (i_len > 0)
    {
        free(p_credential->psz_split_domain);
        p_credential->psz_split_domain =
            strndup(p_credential->psz_username, i_len);
        p_credential->psz_realm = p_credential->psz_split_domain;
    }
    p_credential->psz_username = psz_delim + 1;
}

static void
credential_find_keystore(heimdall_credential *p_credential, heimdall_keystore *p_keystore)
{
    const heimdall_url_t *p_url = p_credential->p_url;

    const char *ppsz_values[KEY_MAX] = { 0 };
    ppsz_values[KEY_PROTOCOL] = p_url->psz_protocol;
    ppsz_values[KEY_USER] = p_credential->psz_username;
    ppsz_values[KEY_SERVER] = p_url->psz_host;
    /* don't try to match with the path */
    ppsz_values[KEY_REALM] = p_credential->psz_realm;
    ppsz_values[KEY_AUTHTYPE] = p_credential->psz_authtype;
    char psz_port[21];
    if (protocol_set_port(p_url, psz_port))
        ppsz_values[KEY_PORT] = psz_port;

    heimdall_keystore_entry *p_entries;
    unsigned int i_entries_count;
    i_entries_count = heimdall_keystore_find(p_keystore, ppsz_values, &p_entries);

    /* Remove last entries after heimdall_keystore_find call since
     * p_credential->psz_username (default username) can be a pointer to an
     * entry */
    if (p_credential->i_entries_count > 0)
    {
        heimdall_keystore_release_entries(p_credential->p_entries,
                                     p_credential->i_entries_count);
        p_credential->psz_username = NULL;
    }
    p_credential->p_entries = p_entries;
    p_credential->i_entries_count = i_entries_count;

    if (p_credential->i_entries_count > 0)
    {
        heimdall_keystore_entry *p_entry;

        if (protocol_store_path(p_url))
            p_entry = find_closest_path(p_credential->p_entries,
                                        p_credential->i_entries_count,
                                        p_url->psz_path);
        else
            p_entry = &p_credential->p_entries[0];

        if (!p_entry || p_entry->p_secret[p_entry->i_secret_len - 1] != '\0')
        {
            heimdall_keystore_release_entries(p_credential->p_entries,
                                         p_credential->i_entries_count);
            p_credential->i_entries_count = 0;
        }
        else
        {
            p_credential->psz_password = (const char *)p_entry->p_secret;
            p_credential->psz_username = p_entry->ppsz_values[KEY_USER];
            p_credential->psz_realm = p_entry->ppsz_values[KEY_REALM];
            p_credential->psz_authtype = p_entry->ppsz_values[KEY_AUTHTYPE];
            p_credential->b_from_keystore = true;
        }
    }
}

void
heimdall_credential_init(heimdall_credential *p_credential, const heimdall_url_t *p_url)
{
    assert(p_credential);

    memset(p_credential, 0, sizeof(*p_credential));
    p_credential->i_get_order = GET_FROM_URL;
    p_credential->p_url = p_url;
}

void
heimdall_credential_clean(heimdall_credential *p_credential)
{
    if (p_credential->i_entries_count > 0)
        heimdall_keystore_release_entries(p_credential->p_entries,
                                     p_credential->i_entries_count);
    if (p_credential->p_keystore)
        heimdall_keystore_release(p_credential->p_keystore);

    free(p_credential->psz_split_domain);
    free(p_credential->psz_var_username);
    free(p_credential->psz_var_password);
    free(p_credential->psz_dialog_username);
    free(p_credential->psz_dialog_password);
}

#undef heimdall_credential_get
int
heimdall_credential_get(heimdall_credential *p_credential, heimdall_object_t *p_parent,
                   const char *psz_option_username,
                   const char *psz_option_password,
                   const char *psz_dialog_title,
                   const char *psz_dialog_fmt, ...)
{
    assert(p_credential && p_parent);
    const heimdall_url_t *p_url = p_credential->p_url;

    if (!is_url_valid(p_url))
    {
        msg_Err(p_parent, "heimdall_credential_get: invalid url");
        return -EINVAL;
    }

    p_credential->b_from_keystore = false;
    /* Don't set username to NULL, we may want to use the last one set */
    p_credential->psz_password = NULL;

    while (!is_credential_valid(p_credential))
    {
        /* First, fetch credential from URL (if any).
         * Secondly, fetch credential from Heimdall Options (if any).
         * Thirdly, fetch credential from keystore (if any) using user and realm
         * previously set by the caller, the URL or by Heimdall Options.
         * Finally, fetch credential from the dialog (if any). This last will be
         * repeated until user cancel the dialog. */

        switch (p_credential->i_get_order)
        {
        case GET_FROM_URL:
            p_credential->psz_username = p_url->psz_username;
            p_credential->psz_password = p_url->psz_password;

            if (p_credential->psz_password)
                msg_Warn(p_parent, "Password in a URI is DEPRECATED");

            if (p_url->psz_username && protocol_is_smb(p_url))
                smb_split_domain(p_credential);
            p_credential->i_get_order++;
            break;

        case GET_FROM_OPTION:
            free(p_credential->psz_var_username);
            free(p_credential->psz_var_password);
            p_credential->psz_var_username =
            p_credential->psz_var_password = NULL;

            if (psz_option_username)
                p_credential->psz_var_username =
                    var_InheritString(p_parent, psz_option_username);
            if (psz_option_password)
                p_credential->psz_var_password =
                    var_InheritString(p_parent, psz_option_password);

            if (p_credential->psz_var_username)
                p_credential->psz_username = p_credential->psz_var_username;
            if (p_credential->psz_var_password)
                p_credential->psz_password = p_credential->psz_var_password;

            p_credential->i_get_order++;
            break;

        case GET_FROM_MEMORY_KEYSTORE:
        {
            heimdall_keystore *p_keystore = get_memory_keystore(p_parent);
            if (p_keystore != NULL)
                credential_find_keystore(p_credential, p_keystore);
            p_credential->i_get_order++;
            break;
        }

        case GET_FROM_KEYSTORE:
            if (p_credential->p_keystore == NULL)
                p_credential->p_keystore = heimdall_keystore_create(p_parent);
            if (p_credential->p_keystore != NULL)
                credential_find_keystore(p_credential, p_credential->p_keystore);

            if (heimdall_killed())
                return -EINTR;

            p_credential->i_get_order++;
            break;

        default:
        case GET_FROM_DIALOG:
            if (!psz_dialog_title || !psz_dialog_fmt)
                return -ENOENT;
            char *psz_dialog_username = NULL;
            char *psz_dialog_password = NULL;
            va_list ap;
            va_start(ap, psz_dialog_fmt);
            bool *p_store = p_credential->p_keystore != NULL ?
                            &p_credential->b_store : NULL;
            int i_ret =
                heimdall_dialog_wait_login_va(p_parent,
                                         &psz_dialog_username,
                                         &psz_dialog_password, p_store,
                                         p_credential->psz_username,
                                         psz_dialog_title, psz_dialog_fmt, ap);
            va_end(ap);

            /* Free previous dialog strings after heimdall_dialog_wait_login_va call
             * since p_credential->psz_username (default username) can be a
             * pointer to p_credential->psz_dialog_username */
            free(p_credential->psz_dialog_username);
            free(p_credential->psz_dialog_password);
            p_credential->psz_dialog_username = psz_dialog_username;
            p_credential->psz_dialog_password = psz_dialog_password;

            if (i_ret != 1)
            {
                p_credential->psz_username = p_credential->psz_password = NULL;
                return heimdall_killed() ? -EINTR : -ENOENT;
            }

            p_credential->psz_username = p_credential->psz_dialog_username;
            p_credential->psz_password = p_credential->psz_dialog_password;

            if (protocol_is_smb(p_url))
                smb_split_domain(p_credential);

            break;
        }
    }
    return is_credential_valid(p_credential) ? 0 : -ENOENT;
}

#undef heimdall_credential_store
bool
heimdall_credential_store(heimdall_credential *p_credential, heimdall_object_t *p_parent)
{
    if (!is_credential_valid(p_credential))
        return false;
    /* Don't need to store again */
    if (p_credential->b_from_keystore)
        return p_credential->b_from_keystore;

    heimdall_keystore *p_keystore;
    if (p_credential->b_store)
    {
        /* Store in permanent keystore */
        assert(p_credential->p_keystore != NULL);
        p_keystore = p_credential->p_keystore;
    }
    else
    {
        /* Store in memory keystore */
        p_keystore = get_memory_keystore(p_parent);
    }
    if (p_keystore == NULL)
        return false;

    const heimdall_url_t *p_url = p_credential->p_url;

    char *psz_path = NULL;
    if (protocol_store_path(p_url)
     && (psz_path =  heimdall_uri_decode_duplicate(p_url->psz_path)) != NULL)
    {
        char *p_slash;
        if (protocol_is_smb(p_url))
        {
            /* Remove all characters after the first slash (store the share but
             * not the path) */
            p_slash = strchr(psz_path + 1, '/');
        }
        else
        {
            /* Remove all characters after the last slash (store the path
             * without the filename) */
            p_slash = strrchr(psz_path + 1, '/');
        }
        if (p_slash && psz_path != p_slash)
            *p_slash = '\0';
    }

    const char *ppsz_values[KEY_MAX] = { 0 };
    ppsz_values[KEY_PROTOCOL] = p_url->psz_protocol;
    ppsz_values[KEY_USER] = p_credential->psz_username;
    ppsz_values[KEY_SERVER] = p_url->psz_host;
    ppsz_values[KEY_PATH] = psz_path;
    ppsz_values[KEY_REALM] = p_credential->psz_realm;
    ppsz_values[KEY_AUTHTYPE] = p_credential->psz_authtype;

    char psz_port[21];
    if (protocol_set_port(p_url, psz_port))
        ppsz_values[KEY_PORT] = psz_port;

    char *psz_label;
    if (asprintf(&psz_label, "LibHeimdall password for %s://%s%s",
                 p_url->psz_protocol, p_url->psz_host,
                 psz_path ? psz_path : "") == -1)
    {
        free(psz_path);
        return false;
    }

    const uint8_t *p_password = (const uint8_t *)
        (p_credential->psz_password != NULL ? p_credential->psz_password : "");

    bool b_ret = heimdall_keystore_store(p_keystore, ppsz_values, p_password,
                                    -1, psz_label) == HEIMDALL_SUCCESS;
    free(psz_label);
    free(psz_path);
    return b_ret;
}
