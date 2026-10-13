/*****************************************************************************
 * iosheimdall.m: watchOS specific development main executable for Heimdall media player
 *****************************************************************************
 * Copyright (C) 2020, 2024 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Marvin Scholz <epirat07 at gmail dot com>
 *          Alexandre Janniaux <ajanni@videolabs.io>
 *          Felix Paul Kühne <fkuehne@videolabs.io>
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
#undef HEIMDALL_DYNAMIC_PLUGINS

#define MODULE_NAME ios_interface
#undef HEIMDALL_DYNAMIC_PLUGINS

#include <heimdall/heimdall.h>

#include <heimdall_common.h>
#include <heimdall_variables.h>
#include <heimdall_plugin.h>

#include "../lib/libheimdall_internal.h"

#import <WatchKit/WatchKit.h>

@interface InterfaceController : WKInterfaceController

@end

@implementation InterfaceController

- (void)awakeWithContext:(id)context {
    [super awakeWithContext:context];
}

- (void)willActivate {
    [super willActivate];
}

- (void)didDeactivate {
    [super didDeactivate];
}

@end

@interface AppDelegate : NSObject <WKApplicationDelegate> {
    @public
    libheimdall_instance_t *_libheimdall;
}
@end

@implementation AppDelegate
/* Called after application launch */
- (void)applicationDidFinishLaunching {
    /* Store startup arguments to forward them to libheimdall */
    NSArray *arguments = [[NSProcessInfo processInfo] arguments];
    unsigned heimdall_argc = [arguments count] - 1;
    const char **heimdall_argv = malloc(heimdall_argc * sizeof *heimdall_argv);
    if (heimdall_argv == NULL)
        return;

    for (unsigned i = 0; i < heimdall_argc; i++)
        heimdall_argv[i] = [[arguments objectAtIndex:i + 1] UTF8String];

    /* Initialize libHeimdall */
    _libheimdall = libheimdall_new(heimdall_argc, (const char * const*)heimdall_argv);
    free(heimdall_argv);

    if (_libheimdall == NULL)
        return;

    /* Start glue interface, see code below */
    libheimdall_InternalAddIntf(_libheimdall->p_libheimdall_int, "ios_interface,none");

    /* Start parsing arguments and eventual playback */
    libheimdall_InternalPlay(_libheimdall->p_libheimdall_int);
}

- (Class)applicationRootInterfaceControllerClass {
    return [InterfaceController class];
}
@end

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return WKApplicationMain(argc, argv, NSStringFromClass([AppDelegate class]));
    }
}

/* Glue interface code */
static int Open(heimdall_object_t *obj)
{
    return HEIMDALL_SUCCESS;
}

#include <heimdall_stream.h>
#include <heimdall_access.h>

static int OpenAssetDemux(heimdall_object_t *obj)
{
    stream_t *access = (stream_t *)obj;

    if (access->psz_location == NULL)
        return HEIMDALL_EGENERIC;

    /* Store startup arguments to forward them to libheimdall */
    NSString *bundle_path = [[NSBundle mainBundle] resourcePath];
    const char *resource_path = [bundle_path UTF8String];
    size_t resource_path_length = strlen(resource_path);

    char *url;
    if (asprintf(&url, "file://%s/%s", resource_path, access->psz_location) < 0)
        return HEIMDALL_ENOMEM;

    access->psz_url = url;

    return HEIMDALL_ACCESS_REDIRECT;
}

heimdall_module_begin()
    set_capability("interface", 0)
    set_callback(Open)

    add_submodule()
    set_capability("access", 1)
    set_callback(OpenAssetDemux)
    add_shortcut("asset")
heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};
