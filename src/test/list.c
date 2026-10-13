/*****************************************************************************
 * list.c: Test for heimdall_list
 *****************************************************************************
 * Copyright (C) 2018 Rémi Denis-Courmont
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
#include <string.h>
#undef NDEBUG
#include <assert.h>

#include <heimdall_common.h>
#include <heimdall_list.h>

struct test_elem
{
    int i;
    struct heimdall_list node;
};

static struct heimdall_list *make_elem(int i)
{
    struct test_elem *e = malloc(sizeof (*e));
    if (e == NULL)
        abort();

    e->i = i;
    return &e->node;
}

int main (void)
{
    struct heimdall_list head, *back;
    struct test_elem *elem;
    int count;

    heimdall_list_init(&head);
    heimdall_list_foreach(elem, &head, node)
        assert(0); /* No iteration on an empty list */
    heimdall_list_reverse_foreach(elem, &head, node)
        assert(0); /* No iteration on an empty list */
    assert(heimdall_list_is_empty(&head));

    heimdall_list_init(&head); /* List can be reinitialized */
    heimdall_list_prepend(make_elem(1), &head);
    count = 0;
    heimdall_list_foreach(elem, &head, node)
        assert(elem->i == 1), count++;
    assert(count == 1);
    count = 0;
    heimdall_list_reverse_foreach(elem, &head, node)
        assert(elem->i == 1), count++;
    assert(count == 1);

    back = make_elem(2);
    heimdall_list_append(back, &head);
    count = 0;
    heimdall_list_foreach(elem, &head, node)
        assert(elem->i == count + 1), count++;
    assert(count == 2);
    count = 0;
    heimdall_list_reverse_foreach(elem, &head, node)
        assert(elem->i == 2 - count), count++;
    assert(count == 2);

    heimdall_list_prepend(make_elem(3), &head);
    heimdall_list_remove(head.prev); /* remove number 2 */
    free(heimdall_list_entry(back, struct test_elem, node));
    count = 0;
    heimdall_list_foreach(elem, &head, node)
        assert(elem->i == (count ? 1 : 3)), count++;
    assert(count == 2);

    heimdall_list_foreach(elem, &head, node)
    {
        heimdall_list_remove(&elem->node);
        free(elem);
    }
    assert(heimdall_list_is_empty(&head));

    heimdall_list_prepend(make_elem(3), &head);
    heimdall_list_reverse_foreach(elem, &head, node)
    {
        heimdall_list_remove(&elem->node);
        free(elem);
    }
    assert(heimdall_list_is_empty(&head));

    /*
     * Create a list from 10 to 30, inserting the 10 last element first
     */
    for (int i = 20; i < 30; i++)
        heimdall_list_append(make_elem(i), &head);
    for (int i = 19; i >= 10; i--)
        heimdall_list_prepend(make_elem(i), &head);

    count = 0;
    heimdall_list_foreach(elem, &head, node)
        assert(elem->i == count + 10), count++;
    assert(count == 20);

    count = 0;
    for (elem = heimdall_list_first_entry_or_null(&head, struct test_elem, node);
         elem != NULL;
         elem = heimdall_list_next_entry_or_null(&head, elem,
                                            struct test_elem, node))
        assert(elem->i == count + 10), count++;
    assert(count == 20);

    count = 0;
    for (elem = heimdall_list_last_entry_or_null(&head, struct test_elem, node);
         elem != NULL;
         elem = heimdall_list_prev_entry_or_null(&head, elem,
                                            struct test_elem, node))
        assert(elem->i == 29 - count), count++;
    assert(count == 20);

    count = 0;
    heimdall_list_foreach(elem, &head, node)
    {
        if (count & 1)
        {
            heimdall_list_remove(&elem->node);
            free(elem);
        }
        count++;
    }
    assert(count == 20);

    count = 0;
    heimdall_list_foreach(elem, &head, node)
    {
        assert(elem->i == count * 2 + 10);
        heimdall_list_remove(&elem->node);
        free(elem);
        count++;
    }
    assert(count == 10);
    assert(heimdall_list_is_empty(&head));
    return 0;
}
