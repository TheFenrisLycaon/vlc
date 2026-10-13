/*****************************************************************************
+ * gl_scale.h
+ *****************************************************************************
+ * Copyright (C) 2021 VLC authors and VideoLAN
+ *
+ * This program is free software; you can redistribute it and/or modify it
+ * under the terms of the GNU Lesser General Public License as published by
+ * the Free Software Foundation; either version 2.1 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
+ * GNU Lesser General Public License for more details.
+ *
+ * You should have received a copy of the GNU Lesser General Public License
+ * along with this program; if not, write to the Free Software Foundation,
+ * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
+ *****************************************************************************/

#ifndef HEIMDALL_GL_SCALE_H
#define HEIMDALL_GL_SCALE_H

enum heimdall_gl_scale {
    HEIMDALL_GLSCALE_BUILTIN,
    HEIMDALL_GLSCALE_SPLINE16,
    HEIMDALL_GLSCALE_SPLINE36,
    HEIMDALL_GLSCALE_SPLINE64,
    HEIMDALL_GLSCALE_MITCHELL,
    HEIMDALL_GLSCALE_BICUBIC,
    HEIMDALL_GLSCALE_EWA_LANCZOS,
    HEIMDALL_GLSCALE_NEAREST,
    HEIMDALL_GLSCALE_BILINEAR,
    HEIMDALL_GLSCALE_GAUSSIAN,
    HEIMDALL_GLSCALE_LANCZOS,
    HEIMDALL_GLSCALE_GINSENG,
    HEIMDALL_GLSCALE_EWA_GINSENG,
    HEIMDALL_GLSCALE_EWA_HANN,
    HEIMDALL_GLSCALE_CATMULL_ROM,
    HEIMDALL_GLSCALE_ROBIDOUX,
    HEIMDALL_GLSCALE_ROBIDOUXSHARP,
    HEIMDALL_GLSCALE_EWA_ROBIDOUX,
    HEIMDALL_GLSCALE_EWA_ROBIDOUXSHARP,
    HEIMDALL_GLSCALE_SINC,
    HEIMDALL_GLSCALE_EWA_JINC,
};

static const int heimdall_glscale_values[] = {
    HEIMDALL_GLSCALE_BUILTIN,
    HEIMDALL_GLSCALE_SPLINE16,
    HEIMDALL_GLSCALE_SPLINE36,
    HEIMDALL_GLSCALE_SPLINE64,
    HEIMDALL_GLSCALE_MITCHELL,
    HEIMDALL_GLSCALE_BICUBIC,
    HEIMDALL_GLSCALE_EWA_LANCZOS,
    HEIMDALL_GLSCALE_NEAREST,
    HEIMDALL_GLSCALE_BILINEAR,
    HEIMDALL_GLSCALE_GAUSSIAN,
    HEIMDALL_GLSCALE_LANCZOS,
    HEIMDALL_GLSCALE_GINSENG,
    HEIMDALL_GLSCALE_EWA_GINSENG,
    HEIMDALL_GLSCALE_EWA_HANN,
    HEIMDALL_GLSCALE_CATMULL_ROM,
    HEIMDALL_GLSCALE_ROBIDOUX,
    HEIMDALL_GLSCALE_ROBIDOUXSHARP,
    HEIMDALL_GLSCALE_EWA_ROBIDOUX,
    HEIMDALL_GLSCALE_EWA_ROBIDOUXSHARP,
    HEIMDALL_GLSCALE_SINC,
    HEIMDALL_GLSCALE_EWA_JINC,
};

static const char *const heimdall_glscale_text[] = {
    "Built-in / fixed function (fast)",
    "Spline 2 taps",
    "Spline 3 taps (recommended upscaler)",
    "Spline 4 taps",
    "Mitchell-Netravali (recommended downscaler)",
    "Bicubic",
    "Jinc / EWA Lanczos 3 taps (high quality, slow)",
    "Nearest neighbor",
    "Bilinear",
    "Gaussian",
    "Lanczos 3 taps",
    "Ginseng 3 taps",
    "EWA Ginseng",
    "EWA Hann",
    "Catmull-Rom",
    "Robidoux",
    "RobidouxSharp",
    "EWA Robidoux",
    "EWA RobidouxSharp",
    "Unwindowed sinc (clipped)",
    "Unwindowed EWA Jinc (clipped)",
};

#define HEIMDALL_GL_UPSCALER_TEXT "OpenGL upscaler"
#define HEIMDALL_GL_UPSCALER_LONGTEXT "Upscaler filter to apply during rendering"

#define HEIMDALL_GL_DOWNSCALER_TEXT "OpenGL downscaler"
#define HEIMDALL_GL_DOWNSCALER_LONGTEXT "Downscaler filter to apply during rendering"

#define add_glscale_opts() \
    set_section(N_("Scaling"), NULL) \
    add_integer("gl-upscaler", HEIMDALL_GLSCALE_BUILTIN, HEIMDALL_GL_UPSCALER_TEXT, \
                HEIMDALL_GL_UPSCALER_LONGTEXT) \
        change_integer_list(heimdall_glscale_values, heimdall_glscale_text) \
    add_integer("gl-downscaler", HEIMDALL_GLSCALE_BUILTIN, HEIMDALL_GL_DOWNSCALER_TEXT, \
                HEIMDALL_GL_DOWNSCALER_LONGTEXT) \
        change_integer_list(heimdall_glscale_values, heimdall_glscale_text) \

#endif
