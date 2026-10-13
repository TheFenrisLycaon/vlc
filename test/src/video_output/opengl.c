/*****************************************************************************
 * opengl.c: test for the OpenGL provider
 *****************************************************************************
 * Copyright (C) 2022 VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_offscreen_mock
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"
#include "../../../lib/libheimdall_internal.h"
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_opengl.h>
#include <heimdall_filter.h>
#include <heimdall_modules.h>
#include <heimdall_vout_display.h>

static char dummy;
static bool swapped;
static bool swapped_offscreen;

/**
 * Dummy decoder device implementation
 */

static void DecoderDeviceClose(struct heimdall_decoder_device *device)
    { HEIMDALL_UNUSED(device); }

static const struct heimdall_decoder_device_operations decoder_device_ops =
    { .close = DecoderDeviceClose, };

static int OpenDecoderDevice(
        struct heimdall_decoder_device *device,
        heimdall_window_t *window
) {
    HEIMDALL_UNUSED(window);

    /* Note: the decoder device type could be anything since we'll hook
     *       it in the test code below. */
    device->type = HEIMDALL_DECODER_DEVICE_VAAPI;
    device->ops = &decoder_device_ops;
    return HEIMDALL_SUCCESS;
}

/**
 * Dummy vout_window implementation
 */
static const struct heimdall_window_operations wnd_ops = {
    .destroy = NULL,
};

static int OpenWindow(heimdall_window_t *wnd)
{
    wnd->type = HEIMDALL_WINDOW_TYPE_DUMMY;
    wnd->ops = &wnd_ops;
    return HEIMDALL_SUCCESS;
}

/**
 * Dummy OpenGL provider
 */

static int OpenGLMakeCurrent(heimdall_gl_t *gl)
{
    HEIMDALL_UNUSED(gl);
    return HEIMDALL_SUCCESS;
}

static void OpenGLReleaseCurrent(heimdall_gl_t *gl)
{
    (void)gl;
}

static void OpenGLSwap(heimdall_gl_t *gl)
{
    (void)gl;
    swapped = true;
}

static picture_t *OpenGLSwapOffscreen(heimdall_gl_t *gl)
{
    (void)gl;
    swapped_offscreen = true;
    return NULL;
}

static void *OpenGLGetSymbol(heimdall_gl_t *gl, const char *procname)
{
    (void)gl; (void)procname;
    if (strcmp(procname, "dummy") == 0)
        return &dummy;
    return NULL;
}

static void OpenGLClose(heimdall_gl_t *gl)
{
    (void)gl;
}

static int
OpenOpenGLCommon(
        heimdall_gl_t *gl, unsigned width, unsigned height,
        bool offscreen, enum heimdall_gl_api_type api_type,
        const struct heimdall_gl_cfg *cfg)
{
    (void)width; (void)height; (void) cfg;
    assert(gl->api_type == api_type);

    static const struct heimdall_gl_operations onscreen_ops =
    {
        .make_current = OpenGLMakeCurrent,
        .release_current = OpenGLReleaseCurrent,
        .resize = NULL,
        .get_proc_address = OpenGLGetSymbol,
        .swap = OpenGLSwap,
        .close = OpenGLClose,
    };

    static const struct heimdall_gl_operations offscreen_ops =
    {
        .make_current = OpenGLMakeCurrent,
        .release_current = OpenGLReleaseCurrent,
        .resize = NULL,
        .get_proc_address = OpenGLGetSymbol,
        .swap_offscreen = OpenGLSwapOffscreen,
        .close = OpenGLClose,
    };

    gl->ops = offscreen ? &offscreen_ops : &onscreen_ops;
    return HEIMDALL_SUCCESS;
}

static int
OpenOpenGL(heimdall_gl_t *gl, unsigned width, unsigned height,
           const struct heimdall_gl_cfg *cfg)
    { return OpenOpenGLCommon(gl, width, height, false, HEIMDALL_OPENGL, cfg); };

static int
OpenOpenGLES(heimdall_gl_t *gl, unsigned width, unsigned height,
             const struct heimdall_gl_cfg *cfg)
    { return OpenOpenGLCommon(gl, width, height, false, HEIMDALL_OPENGL_ES2, cfg); };

static int
OpenOpenGLOffscreen(heimdall_gl_t *gl, unsigned width, unsigned height,
                    const struct heimdall_gl_cfg *cfg)
    { return OpenOpenGLCommon(gl, width, height, true, HEIMDALL_OPENGL, cfg); };

static int
OpenOpenGLESOffscreen(heimdall_gl_t *gl, unsigned width, unsigned height,
                      const struct heimdall_gl_cfg *cfg)
    { return OpenOpenGLCommon(gl, width, height, true, HEIMDALL_OPENGL_ES2, cfg); };

/**
 * Inject the mocked modules as a static plugin:
 *  - decoder device for generating video context and testing release
 *  - opengl offscreen for generating video context and using decoder device
 **/
heimdall_module_begin()
    set_callback_dec_device(OpenDecoderDevice, 1000)
    add_shortcut("test_offscreen")

    add_submodule()
        set_callback(OpenWindow)
        set_capability("vout window", 1)

    add_submodule()
        set_callback_opengl(OpenOpenGL, 1)

    add_submodule()
        set_callback_opengl_es2(OpenOpenGLES, 1)

    add_submodule()
        set_callback_opengl_offscreen(OpenOpenGLOffscreen, 1)

    add_submodule()
        set_callback_opengl_es2_offscreen(OpenOpenGLESOffscreen, 1)
heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};

static void test_opengl_offscreen(heimdall_object_t *root, enum heimdall_gl_api_type api_type)
{
    struct heimdall_decoder_device *device =
        heimdall_decoder_device_Create(root, NULL);
    assert(device != NULL);

    heimdall_gl_t *gl = heimdall_gl_CreateOffscreen(
            root, device, 800, 600, api_type, MODULE_STRING, NULL);
    assert(gl != NULL);
    heimdall_decoder_device_Release(device);

    assert(heimdall_gl_MakeCurrent(gl) == HEIMDALL_SUCCESS);
    assert(heimdall_gl_GetProcAddress(gl, "dummy") == &dummy);
    heimdall_gl_ReleaseCurrent(gl);

    swapped_offscreen = false;
    heimdall_gl_SwapOffscreen(gl);
    assert(swapped_offscreen == true);

    heimdall_gl_Delete(gl);
}

static void test_opengl(heimdall_object_t *root, enum heimdall_gl_api_type api_type)
{
    const heimdall_window_cfg_t wnd_cfg = { .width = 800, .height = 600, };
    const heimdall_window_owner_t owner = { .sys = NULL };
    heimdall_window_t *wnd = heimdall_window_New(root, MODULE_STRING, &owner, &wnd_cfg);
    assert(wnd != NULL && wnd->ops == &wnd_ops);

    const vout_display_cfg_t cfg = {
        .window = wnd,
        .display.width = wnd_cfg.width,
        .display.height = wnd_cfg.height,
    };
    heimdall_gl_t *gl = heimdall_gl_Create(&cfg, api_type, MODULE_STRING, NULL);
    assert(gl != NULL);

    assert(heimdall_gl_MakeCurrent(gl) == HEIMDALL_SUCCESS);
    assert(heimdall_gl_GetProcAddress(gl, "dummy") == &dummy);
    heimdall_gl_ReleaseCurrent(gl);

    swapped = false;
    heimdall_gl_Swap(gl);
    assert(swapped == true);

    heimdall_gl_Delete(gl);
    heimdall_window_Delete(wnd);
}

int main( int argc, char **argv )
{
    (void)argc; (void)argv;
    test_init();

    const char * const heimdall_argv[] = {
        "-vvv", "--aout=dummy", "--text-renderer=dummy",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(heimdall_argv), heimdall_argv);
    heimdall_object_t *root = &heimdall->p_libheimdall_int->obj;

    test_opengl(root, HEIMDALL_OPENGL);
    test_opengl(root, HEIMDALL_OPENGL_ES2);
    test_opengl_offscreen(root, HEIMDALL_OPENGL);
    test_opengl_offscreen(root, HEIMDALL_OPENGL_ES2);

    libheimdall_release(heimdall);
    return 0;
}
