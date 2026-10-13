/*****************************************************************************
 * filters.c: test for the OpenGL filter client code
 *****************************************************************************
 * Copyright (C) 2023 VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Alexandre Janniaux <ajanni@videolabs.io>
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

#ifndef HEIMDALL_TEST_OPENGL_API
# error "Define HEIMDALL_TEST_OPENGL_API to the HEIMDALL_OPENGL API to use"
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_opengl
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../../libheimdall/test.h"
#include "../../../../lib/libheimdall_internal.h"
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_opengl.h>
#include <heimdall_filter.h>
#include <heimdall_modules.h>
#include <heimdall_vout_display.h>

#include "../../../../modules/video_output/opengl/filters.h"
#include "../../../../modules/video_output/opengl/gl_api.h"

static_assert(
    HEIMDALL_TEST_OPENGL_API == HEIMDALL_OPENGL ||
    HEIMDALL_TEST_OPENGL_API == HEIMDALL_OPENGL_ES2,
    "HEIMDALL_TEST_OPENGL_API must be assigned to HEIMDALL_OPENGL or HEIMDALL_OPENGL_ES2");

const char heimdall_module_name[] = MODULE_STRING;
static const uint8_t green[] = { 0x0, 0xff, 0x00, 0xff };
static const uint8_t red[] = { 0xff, 0x0, 0x0, 0xff };
static const uint8_t blue[] = { 0x00, 0x0, 0xff, 0xff };

struct test_point
{
    int x, y;
    const uint8_t *color;
};

static void test_opengl_offscreen(
        heimdall_object_t *root,
        video_orientation_t orientation,
        struct test_point *points,
        size_t points_count
){
    struct heimdall_decoder_device *device =
        heimdall_decoder_device_Create(root, NULL);
    heimdall_gl_t *gl = heimdall_gl_CreateOffscreen(
            root, device, 3, 3, HEIMDALL_TEST_OPENGL_API, NULL, NULL);
    assert(gl != NULL);
    if (device != NULL)
        heimdall_decoder_device_Release(device);

    int ret = heimdall_gl_MakeCurrent(gl);
    assert(ret == HEIMDALL_SUCCESS);

    struct heimdall_gl_api api;
    ret = heimdall_gl_api_Init(&api, gl);
    assert(ret == HEIMDALL_SUCCESS);

    GLuint out_tex;
    api.vt.GenTextures(1, &out_tex);
    api.vt.BindTexture(GL_TEXTURE_2D, out_tex);
    api.vt.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    api.vt.BindTexture(GL_TEXTURE_2D, 0);

    GLuint out_fb;
    api.vt.GenFramebuffers(1, &out_fb);
    api.vt.BindFramebuffer(GL_FRAMEBUFFER, out_fb);
    api.vt.BindFramebuffer(GL_READ_FRAMEBUFFER, out_fb);
    api.vt.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, out_tex, 0);

    assert(api.vt.CheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE);
    GL_ASSERT_NOERROR(&api.vt);

    video_format_t fmt;
    video_format_Init(&fmt, HEIMDALL_CODEC_RGBA);
    video_format_Setup(&fmt, HEIMDALL_CODEC_RGBA, 3, 3, 3, 3, 1, 1);
    fmt.primaries = COLOR_PRIMARIES_UNDEF;
    fmt.space = COLOR_SPACE_UNDEF;
    fmt.transfer = TRANSFER_FUNC_UNDEF;
    fmt.projection_mode = PROJECTION_MODE_RECTANGULAR;

    struct heimdall_gl_interop *interop =
        heimdall_gl_interop_New(gl, NULL, &fmt);
    assert(interop != NULL);
    GL_ASSERT_NOERROR(&api.vt);

    GLint fbo_binding;
    api.vt.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo_binding);
    assert((GLuint)fbo_binding == out_fb);
    assert(api.vt.CheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE);

    struct heimdall_gl_filters *filters =
        heimdall_gl_filters_New(gl, &api, interop, orientation);
    assert(filters != NULL);
    GL_ASSERT_NOERROR(&api.vt);

    api.vt.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo_binding);
    assert((GLuint)fbo_binding == out_fb);
    assert(api.vt.CheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE);

    struct heimdall_gl_filter *filter =
        heimdall_gl_filters_Append(filters, "renderer", NULL);
    assert(filter != NULL);
    GL_ASSERT_NOERROR(&api.vt);
    assert((GLuint)fbo_binding == out_fb);
    assert(api.vt.CheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE);
    (void)filter;

    api.vt.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo_binding);
    assert((GLuint)fbo_binding == out_fb);
    assert(api.vt.CheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE);

    ret = heimdall_gl_filters_InitFramebuffers(filters);
    assert(ret == HEIMDALL_SUCCESS);
    GL_ASSERT_NOERROR(&api.vt);

    api.vt.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo_binding);
    assert((GLuint)fbo_binding == out_fb);
    assert(api.vt.CheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE);

    heimdall_gl_filters_SetViewport(filters, 0, 0, 3, 3);
    GL_ASSERT_NOERROR(&api.vt);

    heimdall_gl_filters_SetOutputSize(filters, 3, 3);
    GL_ASSERT_NOERROR(&api.vt);

    picture_t *picture = picture_NewFromFormat(&fmt);
    assert(picture != NULL);

    size_t size = picture->p[0].i_lines * picture->p[0].i_pitch / picture->p[0].i_pixel_pitch;
    for (size_t i=0; i < size; ++i)
        memcpy(&picture->p[0].p_pixels[i * 4], green, sizeof(green));

    memcpy(&picture->p[0].p_pixels[0 * 4], red, sizeof(red));
    memcpy(&picture->p[0].p_pixels[2 * 4], blue, sizeof(blue));

    ret = heimdall_gl_filters_UpdatePicture(filters, picture);
    assert(ret == HEIMDALL_SUCCESS);
    GL_ASSERT_NOERROR(&api.vt);
    api.vt.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo_binding);
    assert((GLuint)fbo_binding == out_fb);

    ret = heimdall_gl_filters_Draw(filters);
    assert(ret == HEIMDALL_SUCCESS);
    GL_ASSERT_NOERROR(&api.vt);
    api.vt.GetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo_binding);
    assert((GLuint)fbo_binding == out_fb);

    api.vt.Finish();
    GL_ASSERT_NOERROR(&api.vt);

    /* Disable pixel packing to use glReadPixels. */
    api.vt.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    GL_ASSERT_NOERROR(&api.vt);

    uint8_t pixel[4];

    fprintf(stderr, "Results (vertically flipped):\n");
    for (size_t i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 3; ++j)
        {
            api.vt.ReadPixels(j, i, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            fprintf(stderr, "    %u:%u:%u:%u", pixel[0], pixel[1], pixel[2], pixel[3]);
        }
        fprintf(stderr, "\n");
    }

    for (size_t i = 0; i < points_count; ++i)
    {
        api.vt.ReadPixels(points[i].x, points[i].y, 1, 1, GL_RGBA,
                          GL_UNSIGNED_BYTE, &pixel);
        GL_ASSERT_NOERROR(&api.vt);
        assert(memcmp(pixel, points[i].color, sizeof(pixel)) == 0);
    }

    heimdall_gl_filters_Delete(filters);
    GL_ASSERT_NOERROR(&api.vt);

    heimdall_gl_interop_Delete(interop);
    GL_ASSERT_NOERROR(&api.vt);

    heimdall_gl_ReleaseCurrent(gl);
    heimdall_gl_Delete(gl);

    picture_Release(picture);
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

    const char *cap =
        (HEIMDALL_TEST_OPENGL_API == HEIMDALL_OPENGL)     ? "opengl offscreen" :
        (HEIMDALL_TEST_OPENGL_API == HEIMDALL_OPENGL_ES2) ? "opengl es2 offscreen" :
        NULL;
    assert(cap != NULL);

    fprintf(stderr, "Looking for cap %s\n", cap);

    module_t **providers;
    size_t strict_matches;
    ssize_t provider_available = heimdall_module_match(
            cap, NULL, false, &providers, &strict_matches);
    (void)strict_matches;
    free(providers);

    if (provider_available <= 0)
    {
        libheimdall_release(heimdall);
        return 77;
    }

    struct heimdall_decoder_device *device =
        heimdall_decoder_device_Create(root, NULL);
    heimdall_gl_t *gl = heimdall_gl_CreateOffscreen(
            root, device, 3, 3, HEIMDALL_TEST_OPENGL_API, NULL, NULL);
    if (device != NULL)
        heimdall_decoder_device_Release(device);

    if (gl == NULL)
    {
        libheimdall_release(heimdall);
        return 77;
    }
    heimdall_gl_Delete(gl);

    struct test_point points_normal[] = {
        { 1, 1, green },
        { 0, 2, red },
        { 2, 2, blue },
    };
    test_opengl_offscreen(root, ORIENT_NORMAL,
                          points_normal, ARRAY_SIZE(points_normal));

    struct test_point points_rotated_90[] = {
        { 1, 1, green },
        { 0, 0, red },
        { 0, 2, blue },
    };
    test_opengl_offscreen(root, ORIENT_ROTATED_90,
                          points_rotated_90, ARRAY_SIZE(points_rotated_90));

    struct test_point points_rotated_180[] = {
        { 1, 1, green },
        { 2, 0, red },
        { 0, 0, blue },
    };
    test_opengl_offscreen(root, ORIENT_ROTATED_180,
                          points_rotated_180, ARRAY_SIZE(points_rotated_180));

    struct test_point points_rotated_270[] = {
        { 1, 1, green },
        { 2, 2, red },
        { 2, 0, blue },
    };
    test_opengl_offscreen(root, ORIENT_ROTATED_270,
                          points_rotated_270, ARRAY_SIZE(points_rotated_270));

    struct test_point points_hflip[] = {
        { 1, 1, green },
        { 2, 2, red },
        { 0, 2, blue },
    };
    test_opengl_offscreen(root, ORIENT_HFLIPPED,
                          points_hflip, ARRAY_SIZE(points_hflip));

    struct test_point points_vflip[] = {
        { 1, 1, green },
        { 0, 0, red },
        { 2, 0, blue },
    };
    test_opengl_offscreen(root, ORIENT_VFLIPPED,
                          points_vflip, ARRAY_SIZE(points_vflip));

    struct test_point points_transposed[] = {
        { 1, 1, green },
        { 2, 0, red },
        { 2, 2, blue },
    };
    test_opengl_offscreen(root, ORIENT_TRANSPOSED,
                          points_transposed, ARRAY_SIZE(points_transposed));

    struct test_point points_antitransposed[] = {
        { 1, 1, green },
        { 0, 2, red },
        { 0, 0, blue },
    };
    test_opengl_offscreen(root, ORIENT_ANTI_TRANSPOSED,
                          points_antitransposed, ARRAY_SIZE(points_antitransposed));



    libheimdall_release(heimdall);
    return 0;
}
