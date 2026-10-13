/*****************************************************************************
 * playlist_parser.c: test for the lua playlist parser module
 *****************************************************************************
 * Copyright (C) 2026 VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Bipul Lamsal <bipullamsal _at_ gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_lua_playlist_parser
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"

#include <heimdall_common.h>
#include <heimdall_demux.h>
#include <heimdall_input_item.h>
#include <heimdall_interface.h>
#include <heimdall_plugin.h>
#include <heimdall_stream.h>

#include "../lib/libheimdall_internal.h"

const char heimdall_module_name[] = MODULE_STRING;

static int OpenIntf(heimdall_object_t *root) {
  // setup
  const char *test_parsed_value = "Lycaria";
  const char *sample_test_markup = "<html><title>%s</title></html>\n";
  char sample_test[100];
  snprintf(sample_test, sizeof(sample_test), sample_test_markup,
           test_parsed_value);

  stream_t *p_s = heimdall_stream_MemoryNew(root, (uint8_t *)sample_test,
                                       strlen(sample_test), true);
  assert(p_s != NULL);

  p_s->psz_url = strdup("mock://length=100");

  // exercise
  demux_t *p_d = demux_New(root, "luaplaylist", p_s->psz_url, p_s, NULL);

  // verification
  assert(p_d != NULL);

  // next behavior is parse() execution
  input_item_t *p_item = input_item_New(INPUT_ITEM_URI_NOP, NULL);
  input_item_node_t *p_node = input_item_node_Create(p_item);

  int probe_ret = heimdall_stream_ReadDir(p_d, p_node);

  assert(probe_ret == 0);
  assert(p_node->i_children == 1);

  char *test_title = p_node->pp_children[0]->p_item->psz_name;
  assert(strcmp(test_title, test_parsed_value) == 0);

  input_item_node_Delete(p_node);

  // cleanup
  // this calls demux_DestroyDemux(p_d) callback unloading the module and
  // deleting the stream as well
  demux_Delete(p_d);
  return HEIMDALL_SUCCESS;
}

/** Inject the mocked modules as a static plugin: **/
heimdall_module_begin()
    set_callback(OpenIntf)
    set_capability("interface", 0)
heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};

int main(void) {
  setenv("HEIMDALL_USERDATA_PATH", TOP_SRCDIR "/test/modules/", 1);
  test_init();

  const char *const args[] = {
      "-vvv",
      "--vout=dummy",
      "--aout=dummy",
      "--text-renderer=dummy",
      "--no-auto-preparse",
  };

  libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);

  libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
  libheimdall_InternalPlay(heimdall->p_libheimdall_int);

  libheimdall_release(heimdall);
  return 0;
}
