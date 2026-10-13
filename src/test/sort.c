/*****************************************************************************
 * sort.c: Test for sorting
 *****************************************************************************
 * Copyright (C) 2017 Rémi Denis-Courmont
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

#include <stdio.h>
#include <stdlib.h>

#include <heimdall_common.h>
#include <heimdall_strings.h>

const char heimdall_module_name[] = "test_sort";

static void test_smaller(const char *small, const char *big,
                         int (*cmp)(const char *, const char *))
{
    int ret = cmp(small, big);
    if (ret >= 0) {
        fprintf(stderr, "Failure: \"%s\" %s \"%s\"\n",
                small, ret ? ">" : "==", big);
        exit(1);
    }
}

static void test_equal(const char *s, int (*cmp)(const char *, const char *))
{
    int ret = cmp(s, s);
    if (ret != 0) {
        fprintf(stderr, "Failure: \"%s\" %s \"%s\"\n",
                s, (ret < 0) ? "<" : ">", s);
        exit(1);
    }
}


int main (void)
{
    test_smaller("", "a", heimdall_filenamecmp);
    test_smaller("a", "b", heimdall_filenamecmp);
    test_smaller("foo1.ogg", "foo2.ogg", heimdall_filenamecmp);
    test_smaller("foo2.ogg", "foo10.ogg", heimdall_filenamecmp);
    test_smaller("foo1.ogg", "foo10.ogg", heimdall_filenamecmp);
    test_smaller("foo01.ogg", "foo1.ogg", heimdall_filenamecmp);
    test_equal("", heimdall_filenamecmp);
    test_equal("a", heimdall_filenamecmp);
    test_equal("123", heimdall_filenamecmp);
    return 0;
}
