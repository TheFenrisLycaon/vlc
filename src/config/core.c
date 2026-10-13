/*****************************************************************************
 * core.c management of the modules configuration
 *****************************************************************************
 * Copyright (C) 2001-2007 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
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

#include <stdatomic.h>
#include <heimdall_common.h>
#include <heimdall_actions.h>
#include <heimdall_modules.h>
#include <heimdall_plugin.h>
#include <heimdall_threads.h>

#include "heimdall_configuration.h"

#include <errno.h>
#include <assert.h>

#include "configuration.h"
#include "modules/modules.h"
#include "misc/rcu.h"

static heimdall_mutex_t config_lock = HEIMDALL_STATIC_MUTEX;
static atomic_bool config_dirty = false;

void config_Lock(void)
{
    heimdall_mutex_lock(&config_lock);
}

void config_Unlock(void)
{
    heimdall_mutex_unlock(&config_lock);
}

int config_GetType(const char *name)
{
    const struct heimdall_param *param = heimdall_param_Find(name);

    if (param == NULL)
        return 0;

    switch (CONFIG_CLASS(param->item.i_type))
    {
        case CONFIG_ITEM_FLOAT:
            return HEIMDALL_VAR_FLOAT;
        case CONFIG_ITEM_INTEGER:
            return HEIMDALL_VAR_INTEGER;
        case CONFIG_ITEM_BOOL:
            return HEIMDALL_VAR_BOOL;
        case CONFIG_ITEM_STRING:
            return HEIMDALL_VAR_STRING;
        default:
            return 0;
    }
}

bool config_IsSafe( const char *name )
{
    const struct heimdall_param *param = heimdall_param_Find(name);

    return (param != NULL) ? param->safe : false;
}

int64_t config_GetInt(const char *name)
{
    const struct heimdall_param *param = heimdall_param_Find(name);

    /* sanity checks */
    assert(param != NULL);
    assert(IsConfigIntegerType(param->item.i_type));

    return atomic_load_explicit(&param->value.i, memory_order_relaxed);
}

float config_GetFloat(const char *name)
{
    const struct heimdall_param *param = heimdall_param_Find(name);

    /* sanity checks */
    assert(param != NULL);
    assert(IsConfigFloatType(param->item.i_type));

    return atomic_load_explicit(&param->value.f, memory_order_relaxed);
}

char *config_GetPsz(const char *name)
{
    const struct heimdall_param *param = heimdall_param_Find(name);
    char *str;

    /* sanity checks */
    assert(param != NULL);
    assert(IsConfigStringType(param->item.i_type));

    /* return a copy of the string */
    heimdall_rcu_read_lock();
    str = atomic_load_explicit(&param->value.str, memory_order_acquire);
    if (str != NULL)
        str = strdup(str);
    heimdall_rcu_read_unlock();
    return str;
}

int heimdall_param_SetString(struct heimdall_param *param, const char *value)
{
    char *str = NULL, *oldstr;

    assert(param != NULL);
    assert(IsConfigStringType(param->item.i_type));

    if (value != NULL && value[0] != '\0') {
        str = strdup(value);
        if (unlikely(str == NULL))
            return -1;
    }

    oldstr = atomic_load_explicit(&param->value.str, memory_order_relaxed);
    atomic_store_explicit(&param->value.str, str, memory_order_release);
    param->item.value.psz = str;
    heimdall_rcu_synchronize();
    free(oldstr);
    return 0;
}

void config_PutPsz(const char *psz_name, const char *psz_value)
{
    heimdall_mutex_lock(&config_lock);
    heimdall_param_SetString(heimdall_param_Find(psz_name), psz_value);
    heimdall_mutex_unlock(&config_lock);
    atomic_store_explicit(&config_dirty, true, memory_order_release);
}

void config_PutInt(const char *name, int64_t i_value)
{
    struct heimdall_param *param = heimdall_param_Find(name);
    module_config_t *p_config = &param->item;

    /* sanity checks */
    assert(param != NULL);
    assert(IsConfigIntegerType(param->item.i_type));

    if (i_value < p_config->min.i)
        i_value = p_config->min.i;
    if (i_value > p_config->max.i)
        i_value = p_config->max.i;

    atomic_store_explicit(&param->value.i, i_value, memory_order_relaxed);
    heimdall_mutex_lock(&config_lock);
    p_config->value.i = i_value;
    heimdall_mutex_unlock(&config_lock);
    atomic_store_explicit(&config_dirty, true, memory_order_release);
}

void config_PutFloat(const char *name, float f_value)
{
    struct heimdall_param *param = heimdall_param_Find(name);
    module_config_t *p_config = &param->item;

    /* sanity checks */
    assert(param != NULL);
    assert(IsConfigFloatType(param->item.i_type));

    /* if f_min == f_max == 0, then do not use them */
    if ((p_config->min.f == 0.f) && (p_config->max.f == 0.f))
        ;
    else if (f_value < p_config->min.f)
        f_value = p_config->min.f;
    else if (f_value > p_config->max.f)
        f_value = p_config->max.f;

    atomic_store_explicit(&param->value.f, f_value, memory_order_relaxed);
    heimdall_mutex_lock(&config_lock);
    p_config->value.f = f_value;
    heimdall_mutex_unlock(&config_lock);
    atomic_store_explicit(&config_dirty, true, memory_order_release);
}

ssize_t config_GetIntChoices(const char *name,
                             int64_t **restrict values, char ***restrict texts)
{
    *values = NULL;
    *texts = NULL;

    struct heimdall_param *param = heimdall_param_Find(name);
    if (param == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    module_config_t *cfg = &param->item;
    size_t count = cfg->list_count;
    if (count == 0)
    {
        int (*cb)(const char *, int64_t **, char ***);

        cb = heimdall_plugin_Symbol(NULL, param->owner, "heimdall_entry_cfg_int_enum");
        if (cb == NULL)
            return 0;

        return cb(name, values, texts);
    }

    int64_t *vals = heimdall_alloc (count, sizeof (*vals));
    char **txts = heimdall_alloc (count, sizeof (*txts));
    if (vals == NULL || txts == NULL)
    {
        errno = ENOMEM;
        goto error;
    }

    for (size_t i = 0; i < count; i++)
    {
        vals[i] = cfg->list.i[i];
        /* FIXME: use module_gettext() instead */
        txts[i] = strdup ((cfg->list_text[i] != NULL)
                                       ? heimdall_gettext (cfg->list_text[i]) : "");
        if (unlikely(txts[i] == NULL))
        {
            for (int j = i - 1; j >= 0; --j)
                free(txts[j]);
            errno = ENOMEM;
            goto error;
        }
    }

    *values = vals;
    *texts = txts;
    return count;
error:

    free(vals);
    free(txts);
    return -1;
}


static ssize_t config_ListModules (const char *cap, char ***restrict values,
                                   char ***restrict texts)
{
    module_t *const *list;
    size_t n = module_list_cap(&list, cap);
    char **vals = malloc ((n + 2) * sizeof (*vals));
    char **txts = malloc ((n + 2) * sizeof (*txts));
    if (!vals || !txts)
    {
        free (vals);
        free (txts);
        *values = *texts = NULL;
        return -1;
    }

    size_t i = 0;

    vals[i] = strdup ("any");
    txts[i] = strdup (_("Automatic"));
    if (!vals[i] || !txts[i])
        goto error;

    ++i;
    for (; i <= n; i++)
    {
        vals[i] = strdup (module_get_object (list[i - 1]));
        txts[i] = strdup (module_gettext (list[i - 1],
                               module_GetLongName (list[i - 1])));
        if( !vals[i] || !txts[i])
            goto error;
    }
    vals[i] = strdup ("none");
    txts[i] = strdup (_("Disable"));
    if( !vals[i] || !txts[i])
        goto error;

    *values = vals;
    *texts = txts;
    return i + 1;

error:
    for (size_t j = 0; j <= i; ++j)
    {
        free (vals[j]);
        free (txts[j]);
    }
    free(vals);
    free(txts);
    *values = *texts = NULL;
    return -1;
}

ssize_t config_GetPszChoices(const char *name,
                             char ***restrict values, char ***restrict texts)
{
    *values = *texts = NULL;

    struct heimdall_param *param = heimdall_param_Find(name);
    if (param == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    module_config_t *cfg = &param->item;
    switch (cfg->i_type)
    {
        case CONFIG_ITEM_MODULE:
            return config_ListModules (cfg->psz_type, values, texts);
        default:
            if (!IsConfigStringType (cfg->i_type))
            {
                errno = EINVAL;
                return -1;
            }
            break;
    }

    size_t count = cfg->list_count;
    if (count == 0)
    {
        int (*cb)(const char *, char ***, char ***);

        cb = heimdall_plugin_Symbol(NULL, param->owner, "heimdall_entry_cfg_str_enum");
        if (cb == NULL)
            return 0;

        return cb(name, values, texts);
    }

    char **vals = malloc (sizeof (*vals) * count);
    char **txts = malloc (sizeof (*txts) * count);
    if (!vals || !txts)
    {
        free (vals);
        free (txts);
        errno = ENOMEM;
        return -1;
    }

    size_t i;
    for (i = 0; i < count; i++)
    {
        vals[i] = strdup ((cfg->list.psz[i] != NULL) ? cfg->list.psz[i] : "");
        /* FIXME: use module_gettext() instead */
        txts[i] = strdup ((cfg->list_text[i] != NULL)
                                       ? heimdall_gettext (cfg->list_text[i]) : "");
        if (!vals[i] || !txts[i])
            goto error;
    }

    *values = vals;
    *texts = txts;
    return count;

error:
    for (size_t j = 0; j <= i; ++j)
    {
        free (vals[j]);
        free (txts[j]);
    }
    free(vals);
    free(txts);
    errno = ENOMEM;
    return -1;
}

static int confcmp (const void *a, const void *b)
{
    const struct heimdall_param *const *ca = a, *const *cb = b;

    return strcmp ((*ca)->item.psz_name, (*cb)->item.psz_name);
}

static int confnamecmp (const void *key, const void *elem)
{
    const struct heimdall_param *const *conf = elem;

    return strcmp (key, (*conf)->item.psz_name);
}

static struct
{
    struct heimdall_param **list;
    size_t count;
} config = { NULL, 0 };

/**
 * Index the configuration items by name for faster lookups.
 */
int config_SortConfig (void)
{
    heimdall_plugin_t *p;
    size_t nconf = 0;

    for (p = heimdall_plugins; p != NULL; p = p->next)
        nconf += p->conf.count;

    struct heimdall_param **clist = heimdall_alloc(nconf, sizeof (*clist));
    if (unlikely(clist == NULL))
        return HEIMDALL_ENOMEM;

    size_t index = 0;
    for (p = heimdall_plugins; p != NULL; p = p->next)
    {
        for (size_t i = 0; i < p->conf.size; i++)
        {
            struct heimdall_param *param = p->conf.params + i;
            module_config_t *item = &param->item;

            if (!CONFIG_ITEM(item->i_type))
                continue; /* ignore hints */
            assert(index < nconf);
            clist[index++] = param;
        }
    }

    qsort (clist, nconf, sizeof (*clist), confcmp);

    config.list = clist;
    config.count = nconf;
    return HEIMDALL_SUCCESS;
}

void config_UnsortConfig (void)
{
    struct heimdall_param **clist;

    clist = config.list;
    config.list = NULL;
    config.count = 0;

    free (clist);
}

struct heimdall_param *heimdall_param_Find(const char *name)
{
    struct heimdall_param *const *p;

    assert(name != NULL);
    p = bsearch (name, config.list, config.count, sizeof (*p), confnamecmp);
    return (p != NULL) ? *p : NULL;
}

module_config_t *config_FindConfig(const char *name)
{
    if (unlikely(name == NULL))
        return NULL;

    struct heimdall_param *param = heimdall_param_Find(name);

    return (param != NULL) ? &param->item : NULL;
}

/**
 * Destroys an array of configuration items.
 * \param tab start of array of items
 * \param confsize number of items in the array
 */
void config_Free(struct heimdall_param *tab, size_t confsize)
{
    for (size_t j = 0; j < confsize; j++)
    {
        struct heimdall_param *param = &tab[j];
        module_config_t *p_item = &param->item;

        if (IsConfigStringType (p_item->i_type))
        {
            free(atomic_load_explicit(&param->value.str,
                                      memory_order_relaxed));
            if (p_item->list_count)
                free (p_item->list.psz);
        }

        free (p_item->list_text);
    }

    free (tab);
}

void config_ResetAll(void)
{
    heimdall_mutex_lock(&config_lock);
    for (heimdall_plugin_t *p = heimdall_plugins; p != NULL; p = p->next)
    {
        for (size_t i = 0; i < p->conf.size; i++ )
        {
            struct heimdall_param *param = p->conf.params + i;
            module_config_t *p_config = &param->item;

            if (IsConfigIntegerType (p_config->i_type))
            {
                atomic_store_explicit(&param->value.i, p_config->orig.i,
                                      memory_order_relaxed);
                p_config->value.i = p_config->orig.i;
            }
            else
            if (IsConfigFloatType (p_config->i_type))
            {
                atomic_store_explicit(&param->value.f, p_config->orig.f,
                                      memory_order_relaxed);
                p_config->value.f = p_config->orig.f;
            }
            else
            if (IsConfigStringType (p_config->i_type))
                heimdall_param_SetString(param, p_config->orig.psz);
        }
    }
    heimdall_mutex_unlock(&config_lock);
    atomic_store_explicit(&config_dirty, true, memory_order_release);
}


int config_AutoSaveConfigFile( libheimdall_int_t *p_this )
{
    assert( p_this );

    if (!atomic_exchange_explicit(&config_dirty, false, memory_order_acquire))
        return 0;

    int ret = config_SaveConfigFile (p_this);

    if (unlikely(ret != 0))
        /*
         * On write failure, set the dirty flag back again. While it looks
         * racy, it really means to retry later in hope that it does not
         * fail again. Concurrent write attempts would not succeed anyway.
         */
        atomic_store_explicit(&config_dirty, true, memory_order_relaxed);

    return ret;
}
