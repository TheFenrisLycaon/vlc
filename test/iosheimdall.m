/*****************************************************************************
 * iosheimdall.m: iOS specific development main executable for Heimdall media player
 *****************************************************************************
 * Copyright (C) 2020-2026 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Marvin Scholz <epirat07 at gmail dot com>
 *          Alexandre Janniaux <ajanni@videolabs.io>
 *          Felix Paul Kühne <fkuehne # videolabs.io>
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

#import <UIKit/UIKit.h>
#include <heimdall/heimdall.h>

#include <heimdall_common.h>
#include <heimdall_variables.h>
#include <heimdall_plugin.h>

#include <TargetConditionals.h>
#include <dispatch/dispatch.h>

#include "../lib/libheimdall_internal.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate> {
    @public
    libheimdall_instance_t *_libheimdall;
    dispatch_queue_t _intfQueue;
    UIWindow *window;
    UIView *subview;

#if !TARGET_OS_TV
    UIPinchGestureRecognizer *_pinchRecognizer;
#endif

    CGRect _pinchRect;
    CGPoint _pinchOrigin;
    CGPoint _pinchPreviousCenter;
}
@end

API_AVAILABLE(ios(13.0), tvos(13.0))
@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

static void heimdall_terminate(void *data)
{
    AppDelegate *d = (__bridge AppDelegate *)data;

    __block bool quitting = false;
    static dispatch_once_t quitToken = 0;
    dispatch_once(&quitToken, ^{
        quitting = true;
    });

    if (!quitting)
        return;

    dispatch_async(d->_intfQueue, ^{
        libheimdall_release(d->_libheimdall);
        d->_libheimdall = NULL;
    });
}

@implementation AppDelegate
#if !TARGET_OS_TV
- (void)pinchRecognized:(UIPinchGestureRecognizer *)pinchRecognizer
{
    UIGestureRecognizerState state = [pinchRecognizer state];

    switch (state)
    {
        case UIGestureRecognizerStateBegan:
            _pinchRect = [subview frame];
            _pinchOrigin = [pinchRecognizer locationInView:nil];
            _pinchPreviousCenter = [subview center];
            return;
        case UIGestureRecognizerStateEnded:
            return;
        case UIGestureRecognizerStateChanged:
            break;
        default:
            return;
    }

    CGFloat scale = pinchRecognizer.scale;
    CGRect viewBounds = _pinchRect;
    if (scale >= 1.0 && (viewBounds.size.width == 0 || viewBounds.size.height == 0))
            viewBounds.size.width = viewBounds.size.height = 1;
    viewBounds.size.width *= scale;
    viewBounds.size.height *= scale;
    subview.frame = viewBounds;
    CGPoint newPosition = [pinchRecognizer locationInView:nil];
    subview.center = CGPointMake(
            _pinchPreviousCenter.x + newPosition.x - _pinchOrigin.x,
            _pinchPreviousCenter.y + newPosition.y - _pinchOrigin.y);
}
#endif

/* Called after application launch */
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    _intfQueue = dispatch_queue_create("com.lycaria.heimdall.ios.intf",
                                       DISPATCH_QUEUE_SERIAL);

    /* Defer libHeimdall initialization until the main run loop is active.
     * This prevents interfaces created via libheimdall_InternalAddIntf from
     * creating and owning the eventloop themselves at plugin loading
     * time (which Qt qios platform does). We don't need those plugins
     * before the interface is ready to load with an eventloop anyway. */
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^{
        /* Store startup arguments to forward them to libheimdall */
        NSArray *arguments = [[NSProcessInfo processInfo] arguments];
        unsigned heimdall_argc = [arguments count] - 1;
        const char *intf_arg = "--intf=" MODULE_STRING;
        unsigned heimdall_argc_with_args = heimdall_argc + 1;
        const char **heimdall_argv = malloc(heimdall_argc_with_args * sizeof *heimdall_argv);
        if (heimdall_argv == NULL)
            return;

        heimdall_argv[0] = intf_arg;
        for (unsigned i = 0; i < heimdall_argc; i++)
            heimdall_argv[i + 1] = [[arguments objectAtIndex:i + 1] UTF8String];

        /* Initialize libHeimdall */
        self->_libheimdall = libheimdall_new(heimdall_argc_with_args, (const char * const*)heimdall_argv);
        free(heimdall_argv);

        if (self->_libheimdall == NULL) {
            NSLog(@"Failed to initialize libHeimdall");
            return;
        }

        libheimdall_SetExitHandler(self->_libheimdall->p_libheimdall_int, heimdall_terminate,
                              (__bridge void *)self);

        dispatch_async(self->_intfQueue, ^{
            @autoreleasepool {
                libheimdall_InternalAddIntf(self->_libheimdall->p_libheimdall_int, NULL);
                libheimdall_InternalPlay(self->_libheimdall->p_libheimdall_int);
            }
        });
    });
    CFRunLoopWakeUp(CFRunLoopGetMain());

    return YES;
}

- (UISceneConfiguration *)application:(UIApplication *)application
configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                              options:(UISceneConnectionOptions *)options API_AVAILABLE(ios(13.0), tvos(13.0))
{
    UISceneConfiguration *configuration = [[UISceneConfiguration alloc] initWithName:nil
                                                                         sessionRole:connectingSceneSession.role];
    configuration.delegateClass = [SceneDelegate class];
    return configuration;
}
@end

@implementation SceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
      options:(UISceneConnectionOptions *)connectionOptions
{
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];

    /* The window contents are set up once the glue interface opens */
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    appDelegate->window = _window;
}
@end

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

/* Glue interface code, define drawable-nsobject for display module */
static int Open(heimdall_object_t *obj)
{
    __block AppDelegate *d;

    dispatch_sync(dispatch_get_main_queue(), ^{
        d = (AppDelegate *)[[UIApplication sharedApplication] delegate];
        if (d == nil || d->subview != nil)
            return;

#if !TARGET_OS_VISION
        /* Initialize main window unless the scene delegate provided one */
        if (d->window == nil)
            d->window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
#endif

        d->window.rootViewController = [[UIViewController alloc] init];
        d->window.backgroundColor = [UIColor whiteColor];

        d->subview = d->window.rootViewController.view;
        d->subview.frame = d->window.bounds;
        d->subview.backgroundColor = [UIColor clearColor];
        [d->window makeKeyAndVisible];

#if !TARGET_OS_TV
        d->_pinchRecognizer = [[UIPinchGestureRecognizer alloc]
            initWithTarget:d action:@selector(pinchRecognized:)];
        [d->window addGestureRecognizer:d->_pinchRecognizer];
#endif
    } );

    if (d == nil)
        return HEIMDALL_EGENERIC;

    assert(d->subview != nil);
    var_SetAddress(heimdall_object_instance(obj), "drawable-nsobject",
                   (__bridge void *)d->subview);

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
