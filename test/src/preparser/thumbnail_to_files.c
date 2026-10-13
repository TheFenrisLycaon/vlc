/*****************************************************************************
 * thumbnail_to_files.c: test thumbnailing to files API
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#include "../../libheimdall/test.h"
#include "../lib/libheimdall_internal.h"

#include <heimdall_common.h>
#include <heimdall_preparser.h>
#include <heimdall_input_item.h>
#include <heimdall_picture.h>
#include <heimdall_modules.h>
#include <heimdall_fs.h>

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#define MOCK_WIDTH 640
#define MOCK_HEIGHT 480

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define MOCK_URL "mock://video_track_count=1;" \
                 "video_width="STRINGIFY(MOCK_WIDTH)";" \
                 "video_height="STRINGIFY(MOCK_HEIGHT)

struct test_entry
{
    int in_width;
    int in_height;
    bool in_crop;

    bool should_succeed;
    unsigned out_width;
    unsigned out_height;
};

static const struct test_entry test_entries[] =
{
    {
        .in_width = 0, .in_height = 0, .in_crop = false,
        .should_succeed = true,
        .out_width = MOCK_WIDTH, .out_height = MOCK_HEIGHT
    },

    {
        .in_width = 0, .in_height = 0, .in_crop = false,
        .should_succeed = false,
    },
    {
        .in_width = 320, .in_height = 0, .in_crop = false,
        .should_succeed = true,
        .out_width = 320, .out_height = 240,
    },
};

struct context
{
    size_t test_count;
    const struct heimdall_thumbnailer_output *entries;
    const struct test_entry *test_entries;
    heimdall_preparser_t *preparser;

    heimdall_fourcc_t parsed_fourcc;
    const char *forced_demux;

    heimdall_sem_t sem;
};

static void parser_on_ended(heimdall_preparser_req *req, int status, void *userdata)
{
    input_item_t *item = heimdall_preparser_req_GetItem(req);
    struct context *context = userdata;
    assert(status == HEIMDALL_SUCCESS);

    /* Retrieve the test_entry from the uri */
    const struct test_entry *test_entry = NULL;
    for (size_t i = 0; i < context->test_count; ++i)
    {
        if (strcmp(item->psz_uri + strlen("file://"),
                   context->entries[i].file_path) == 0)
        {
            test_entry = &context->test_entries[i];
            break;
        }
    }
    assert(test_entry != NULL);

    assert(item->es_vec.size == 1);
    const struct input_item_es *item_es = &item->es_vec.data[0];
    const es_format_t *es_fmt = &item_es->es;

    /* Check if the video format of the thumbnail is expected */
    assert(es_fmt->i_cat == VIDEO_ES);
    assert(es_fmt->i_codec == context->parsed_fourcc);
    assert(es_fmt->video.i_width == test_entry->out_width);
    assert(es_fmt->video.i_height == test_entry->out_height);

    heimdall_sem_post(&context->sem);
    heimdall_preparser_req_Release(req);
}

static void on_ended(heimdall_preparser_req *thumbnailer_req, int status,
                     const bool *result_array, size_t result_count, void *data)
{
    (void) thumbnailer_req;
    struct context *context = data;
    assert(status == HEIMDALL_SUCCESS);
    assert(context->test_count == result_count);

    static const struct heimdall_preparser_cbs parser_cbs = {
        .on_ended = parser_on_ended,
    };

    for (size_t i = 0; i < result_count; ++i)
    {
        const struct heimdall_thumbnailer_output *entry = &context->entries[i];
        const struct test_entry *test_entry = &context->test_entries[i];

        assert(test_entry->should_succeed == result_array[i]);

        if (!result_array[i])
            heimdall_sem_post(&context->sem);
        else
        {
            /* Parse the thumbnail to check its validity */
            char *uri;
            int ret = asprintf(&uri, "file://%s", entry->file_path);
            assert(ret > 0);
            input_item_t *thumb = input_item_New(uri, "thumb");
            free(uri);
            assert(thumb != NULL);

            if (context->forced_demux != NULL)
            {
                /* Setup the forced demux */
                char *option;
                ret = asprintf(&option, ":demux=%s", context->forced_demux);
                assert(ret > 0);
                input_item_AddOption(thumb, option, HEIMDALL_INPUT_OPTION_TRUSTED);
                free(option);
            }

            heimdall_preparser_req *parser_req =
                heimdall_preparser_req_NewParse(context->preparser, thumb,
                                           HEIMDALL_PREPARSER_TYPE_PARSE,
                                           &parser_cbs, context);
            assert(parser_req != NULL);
            assert(heimdall_preparser_Submit(context->preparser, parser_req)
                   == HEIMDALL_SUCCESS);
            input_item_Release(thumb);
        }
    }
}

static int get_formats(enum heimdall_thumbnailer_format *out_format,
                       heimdall_fourcc_t *parsed_fourcc, const char **forced_demux,
                       const char **ext)
{
    /* This test require any of these images encoders */
    static const struct format
    {
        enum heimdall_thumbnailer_format format;
        heimdall_fourcc_t parsed_fourcc;
        const char *ext;
        const char *forced_demux;
    } formats[] = {
        {
            .format = HEIMDALL_THUMBNAILER_FORMAT_PNG,
            .parsed_fourcc = HEIMDALL_CODEC_RGB24,
            .ext = "png"
        },
        {
            .format = HEIMDALL_THUMBNAILER_FORMAT_WEBP,
            .parsed_fourcc = HEIMDALL_CODEC_I420,
            .ext = "webp"
        },
        {
            .format = HEIMDALL_THUMBNAILER_FORMAT_JPEG,
            .parsed_fourcc = HEIMDALL_CODEC_MJPG,
            .ext = "jpg",
            /* XXX: The mjpeg demux won't parse width/height */
            .forced_demux = "avformat"
        },
    };

    /* Retrieve the best format */
    enum heimdall_thumbnailer_format format;
    int ret = heimdall_preparser_GetBestThumbnailerFormat(&format, ext);
    if (ret != 0)
        return ret;

    /* Check that the best format is checked and succeed */
    ret = heimdall_preparser_CheckThumbnailerFormat(format);
    assert(ret == 0);

    for (size_t i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        if (formats[i].format != format)
            continue;

        /* Check if the forced demux module (for preparsing) is present */
        if (formats[i].forced_demux != NULL
         && !module_exists(formats[i].forced_demux))
            return HEIMDALL_ENOENT;

        assert(strcmp(*ext, formats[i].ext) == 0);

        *out_format = format;
        *parsed_fourcc = formats[i].parsed_fourcc;
        *forced_demux = formats[i].forced_demux;
        return 0;
    }

    heimdall_assert_unreachable();
}

static int run_test(libheimdall_instance_t *heimdall, bool external)
{
    size_t test_count = ARRAY_SIZE(test_entries);
    struct heimdall_thumbnailer_output entries[test_count];
    int fd_array[test_count];
    char path_array[test_count][sizeof("/tmp/libheimdall_XXXXXX")];

    enum heimdall_thumbnailer_format format;
    const char *forced_demux, *ext;
    heimdall_fourcc_t parsed_fourcc;
    int ret = get_formats(&format, &parsed_fourcc, &forced_demux, &ext);
    if (ret == HEIMDALL_ENOENT)
    {
        fprintf(stderr, "skip: no \"image encoder\" modules\n");
        goto skip;
    }
    assert(ret == 0);

    fprintf(stderr, "thumbnail_to_files: using format: %s\n", ext);

    /* Fill output entries */
    for (size_t i = 0; i < test_count; ++i)
    {
        const struct test_entry *test_entry = &test_entries[i];
        struct heimdall_thumbnailer_output *entry = &entries[i];

        if (test_entry->should_succeed)
        {
            /* Fill the output entry with a valid tmp path */
            strcpy(path_array[i], "/tmp/libheimdall_XXXXXX");
            fd_array[i] = heimdall_mkstemp(path_array[i]);
            if (i == 0 && fd_array[i] == -1)
            {
                fprintf(stderr, "skip: heimdall_mkstemp failed\n");
                goto skip;
            }
            assert(fd_array[i] != -1);
            entry->file_path = path_array[i];
        }
        else
        {
            fd_array[i] = -1;
            entry->file_path = "/this/path/does/not/exist/and/should/fail";
        }
        entry->format = format;
        entry->width = test_entry->in_width;
        entry->height = test_entry->in_height;
        entry->crop = test_entry->in_crop;
        entry->creat_mode = 0666;
    }

    /* Preparser configuration: thumbnailer + parser */
    const struct heimdall_preparser_cfg cfg = {
        .types = HEIMDALL_PREPARSER_TYPE_THUMBNAIL_TO_FILES|HEIMDALL_PREPARSER_TYPE_PARSE,
        .max_parser_threads = 1,
        .max_thumbnailer_threads = 1,
        .timeout = 0,
        .external_process = external,
    };
    heimdall_preparser_t *preparser = heimdall_preparser_New(HEIMDALL_OBJECT(heimdall->p_libheimdall_int),
                                                   &cfg);
    assert(preparser != NULL);

    struct context context = {
        .test_count = test_count,
        .entries = entries,
        .test_entries = test_entries,
        .preparser = preparser,
        .parsed_fourcc = parsed_fourcc,
        .forced_demux = forced_demux,
    };
    heimdall_sem_init(&context.sem, 0);

    /* Common arguments */
    const struct heimdall_thumbnailer_arg arg =
    {
        .seek = {
            .type = HEIMDALL_THUMBNAILER_SEEK_POS,
            .pos = 0.2,
            .speed = HEIMDALL_THUMBNAILER_SEEK_FAST,
        },
        .hw_dec = false,
    };
    static const struct heimdall_thumbnailer_to_files_cbs cbs = {
        .on_ended = on_ended,
    };

    input_item_t *item = input_item_New(MOCK_URL, "mock");
    assert(item != NULL);

    heimdall_preparser_req *req =
        heimdall_preparser_req_NewThumbnailToFiles(preparser, item, &arg,
                                              entries, test_count,
                                              &cbs, &context);
    assert(req != NULL);
    assert(heimdall_preparser_Submit(preparser, req) == HEIMDALL_SUCCESS);

    /* Wait for all tests */
    for (size_t i = 0; i < test_count; ++i)
        heimdall_sem_wait(&context.sem);

    size_t count = heimdall_preparser_Cancel(preparser, req);
    assert(count == 0); /* Should not be cancelled and already processed */
    heimdall_preparser_req_Release(req);

    for (size_t i = 0; i < test_count; ++i)
        if (fd_array[i] != -1)
        {
            unlink(path_array[i]);
            close(fd_array[i]);
        }

    input_item_Release(item);

    heimdall_preparser_Delete(preparser);
    return 0;

skip:
    return 77;
}

struct alpha_context
{
    heimdall_sem_t sem;
    bool success;
};

static void alpha_on_ended(heimdall_preparser_req *req, int status,
                           const bool *result_array, size_t result_count,
                           void *data)
{
    struct alpha_context *ctx = data;
    (void) req;
    assert(status == HEIMDALL_SUCCESS);
    assert(result_count == 1);
    ctx->success = result_array[0];
    heimdall_sem_post(&ctx->sem);
}

/* Regression test for:
 *  - test_align=false: "swscale: fix OOB read when scaling a cropped alpha plane"
 *  - test_align=true: "swscale: extract the alpha plane from the cropped origin"
 */
static int run_alpha_crop_test(libheimdall_instance_t *heimdall, bool test_align)
{
    /* Needs an RGBA image encoder (avcodec); skip otherwise. */
    if (heimdall_preparser_CheckThumbnailerFormat(HEIMDALL_THUMBNAILER_FORMAT_RGBA) != 0)
    {
        fprintf(stderr, "skip: no RGBA thumbnailer format\n");
        return 0;
    }

    char path[] = "/tmp/libheimdall_XXXXXX";
    int fd = heimdall_mkstemp(path);
    if (fd == -1)
    {
        fprintf(stderr, "skip: heimdall_mkstemp failed\n");
        return 0;
    }

    /* 256x128 (2:1) from a 640x480 (4:3) source forces a non-zero centered crop
     * (640x320, y_offset 80), which the colorbar alignment check below relies
     * on to keep the bottom opaque band. */
    const unsigned ow = 256, oh = 128;
    const struct heimdall_thumbnailer_output output = {
        .format = HEIMDALL_THUMBNAILER_FORMAT_RGBA,
        .width = ow, .height = oh, .crop = true,
        .file_path = path, .creat_mode = 0666,
    };

    const struct heimdall_preparser_cfg cfg = {
        .types = HEIMDALL_PREPARSER_TYPE_THUMBNAIL_TO_FILES,
        .max_parser_threads = 1,
        .max_thumbnailer_threads = 1,
        .timeout = 0,
        .external_process = false,
    };
    heimdall_preparser_t *preparser = heimdall_preparser_New(HEIMDALL_OBJECT(heimdall->p_libheimdall_int),
                                                   &cfg);
    assert(preparser != NULL);

    struct alpha_context ctx = { .success = false };
    heimdall_sem_init(&ctx.sem, 0);

    const struct heimdall_thumbnailer_arg arg = {
        .seek = { .type = HEIMDALL_THUMBNAILER_SEEK_POS, .pos = 0.2,
                  .speed = HEIMDALL_THUMBNAILER_SEEK_FAST },
        .hw_dec = false,
    };
    static const struct heimdall_thumbnailer_to_files_cbs cbs = {
        .on_ended = alpha_on_ended,
    };

    /* RGBA source so the conversion exercises swscale's alpha path. */
    input_item_t *item = input_item_New(
        test_align ? MOCK_URL ";video_chroma=RGBA;video_colorbar=true"
                   : MOCK_URL ";video_chroma=RGBA", "mock");
    assert(item != NULL);

    heimdall_preparser_req *req =
        heimdall_preparser_req_NewThumbnailToFiles(preparser, item, &arg, &output, 1,
                                              &cbs, &ctx);
    assert(req != NULL);
    assert(heimdall_preparser_Submit(preparser, req) == HEIMDALL_SUCCESS);
    heimdall_sem_wait(&ctx.sem);

    heimdall_preparser_req_Release(req);
    input_item_Release(item);
    heimdall_preparser_Delete(preparser);

    assert(ctx.success);


    const size_t size = (size_t)ow * oh * 4;
    uint8_t *buf = malloc(size);
    assert(buf != NULL);
    assert(lseek(fd, 0, SEEK_SET) == 0);
    for (size_t off = 0; off < size; )
    {
        ssize_t r = read(fd, buf + off, size - off);
        assert(r > 0);
        off += r;
    }
    if (test_align)
    {
        /* With the mock colorbar, a 640x480 RGBA frame has 4 horizontal bands
         * of 120 lines; only the bottom band (rows 360..479) is opaque (alpha
         * 0xFF), the rest is transparent. A 2:1 output crops to 640x320
         * centered (rows 80..399), so the cropped region includes the bottom
         * opaque band. If the alpha is (wrongly) extracted from (0,0), it
         * covers rows 0..319 which are fully transparent, and the output alpha
         * is 0 everywhere. */
        uint8_t max_alpha = 0;
        for (size_t i = 0; i < (size_t)ow * oh; i++)
            if (buf[i * 4 + 3] > max_alpha)
                max_alpha = buf[i * 4 + 3];
        /* Loose midpoint: the band is kept (alpha ~0xFF) or dropped (alpha 0).
         * The exact scaled value is implementation-dependent. */
        assert(max_alpha > 200
               && "swscale alpha-crop regression: alpha extracted from wrong origin");
    }
    else
    {
        /* Read back the raw RGBA. The mock fills every byte with the same
         * value, so R == G == B == A on input. The colour planes go through
         * the in-bounds main rescale, the alpha through the cropped alpha
         * path, so on a correct conversion the output alpha still equals the
         * red channel everywhere. The out-of-bounds bug leaves the alpha
         * garbage (it differs from red). */
        for (size_t i = 0; i < (size_t)ow * oh; i++)
        assert(buf[i * 4 + 3] == buf[i * 4 + 0]
               && "swscale alpha-crop regression: alpha does not match colour");
    }
    free(buf);

    close(fd);
    unlink(path);
    return 0;
}

int main(int argc, const char *argv[])
{
    test_init();
    argc--;
    argv++;

    libheimdall_instance_t *heimdall = libheimdall_new(argc, argv);
    assert(heimdall);

    int ret = 0;

    /* This test require swscale */
    if (!module_exists("swscale"))
    {
        fprintf(stderr, "skip: no \"swscale\" module\n");
        goto end;
    }

    fprintf(stderr, "Run with internal preparser...\n");
    ret = run_test(heimdall, false);
    if (ret != 0) {
        goto end;
    }
    fprintf(stderr, "Run alpha crop test with internal preparser...\n");
    ret = run_alpha_crop_test(heimdall, false);
    if (ret != 0) {
        goto end;
    }
    fprintf(stderr, "Run alpha align test with internal preparser...\n");
    ret = run_alpha_crop_test(heimdall, true);
    if (ret != 0) {
        goto end;
    }
#if !defined(HAVE_HEIMDALL_PROCESS_SPAWN)
    fprintf(stderr, "Run with external preparser...\n");
    ret = run_test(heimdall, true);
    if (ret != 0) {
        goto end;
    }
#endif

end:
    libheimdall_release(heimdall);
    return ret;
}
