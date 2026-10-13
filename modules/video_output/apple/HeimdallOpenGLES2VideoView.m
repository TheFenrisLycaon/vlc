/*****************************************************************************
 * HeimdallOpenGLES2VideoView.m: iOS OpenGL ES provider through CAEAGLLayer
 *****************************************************************************
 * Copyright (C) 2001-2017 VLC authors and VideoLAN
 * Copyright (C) 2021 Videolabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Pierre d'Herbemont <pdherbemont at lycaria dot org>
 *          Felix Paul Kühne <fkuehne at lycaria dot org>
 *          David Fuhrmann <david dot fuhrmann at googlemail dot com>
 *          Rémi Denis-Courmont
 *          Laurent Aimar <fenrir _AT_ lycaria _DOT_ org>
 *          Eric Petit <titer@m0k.org>
 *          Alexandre Janniaux <ajanni@videolabs.io>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/QuartzCore.h>
#import <dlfcn.h>

#ifdef HAVE_CONFIG_H
# import "config.h"
#endif

#import <heimdall_common.h>
#import <heimdall_threads.h>
#import <heimdall_plugin.h>
#import <heimdall_vout_display.h>
#import <heimdall_opengl.h>
#import <heimdall_dialog.h>
#import "../opengl/vout_helper.h"
#import "../opengl/gl_api.h"

@interface HeimdallOpenGLES2VideoView : UIView {
    heimdall_gl_t *_gl;

    EAGLContext *_eaglContext;
    EAGLContext *_previousEaglContext;
    CAEAGLLayer *_layer;

    heimdall_mutex_t _mutex;
    heimdall_cond_t  _gl_attached_wait;
    BOOL        _gl_attached;

    BOOL _bufferNeedReset;
    BOOL _appActive;
    BOOL _eaglEnabled;

    GLuint _renderBuffer;
    GLuint _frameBuffer;

    struct heimdall_gl_api _api;
}

- (id)initWithFrame:(CGRect)frame gl:(heimdall_gl_t*)gl;
- (BOOL)makeCurrent;
- (void)releaseCurrent;
- (void)presentRenderbuffer;
- (void)didMoveToWindow;
- (void)detachFromWindow;
@end

static void heimdall_dispatch_sync(void (^block_function)())
{
    CFRunLoopRef runloop = CFRunLoopGetMain();

    __block heimdall_sem_t performed;
    heimdall_sem_init(&performed, 0);

    CFStringRef modes_cfstrings[] = {
        kCFRunLoopCommonModes,
        CFSTR("com.lycaria.heimdallcore.window"),
    };

    CFArrayRef modes = CFArrayCreate(NULL, (const void **)modes_cfstrings,
            ARRAY_SIZE(modes_cfstrings),
            &kCFTypeArrayCallBacks);

    /* NOTE: we're using CFRunLoopPerformBlock with a custom mode tag
     * to avoid deadlocks between the window module (main thread) and the
     * display module, which would happen when using dispatch_sycn here. */
    CFRunLoopPerformBlock(runloop, modes, ^{
        (block_function)();
        heimdall_sem_post(&performed);
    });
    CFRunLoopWakeUp(runloop);

    heimdall_sem_wait(&performed);
    CFRelease(modes);
}

/*****************************************************************************
 * heimdall_gl_t callbacks
 *****************************************************************************/
static void *GetSymbol(heimdall_gl_t *gl, const char *name)
{
    HEIMDALL_UNUSED(gl);
    return dlsym(RTLD_DEFAULT, name);
}

static int MakeCurrent(heimdall_gl_t *gl)
{
    HeimdallOpenGLES2VideoView *view = (__bridge HeimdallOpenGLES2VideoView *)gl->sys;

    if (![view makeCurrent])
        return HEIMDALL_EGENERIC;
    return HEIMDALL_SUCCESS;
}

static void ReleaseCurrent(heimdall_gl_t *gl)
{
    HeimdallOpenGLES2VideoView *view = (__bridge HeimdallOpenGLES2VideoView *)gl->sys;
    [view releaseCurrent];
}

static void Swap(heimdall_gl_t *gl)
{
    HeimdallOpenGLES2VideoView *view = (__bridge HeimdallOpenGLES2VideoView *)gl->sys;
    [view presentRenderbuffer];
}

static void Resize(heimdall_gl_t *gl, unsigned width, unsigned height)
{
    HEIMDALL_UNUSED(gl); HEIMDALL_UNUSED(width); HEIMDALL_UNUSED(height);
    /* Use the parent frame size for now, resize is smoother and called
     * automatically from the main thread queue. */
}

static void Close(heimdall_gl_t *gl)
{
    /* Transfer ownership back from Heimdall to ARC so that it can be released. */
    HeimdallOpenGLES2VideoView *view = (__bridge_transfer HeimdallOpenGLES2VideoView*)gl->sys;

    /* We need to detach because the superview has a reference to our view. */
    [view detachFromWindow];
}

/*****************************************************************************
 * Our UIView object
 *****************************************************************************/
@implementation HeimdallOpenGLES2VideoView

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame gl:(heimdall_gl_t*)gl
{
    _gl = gl;

    _appActive = ([UIApplication sharedApplication].applicationState != UIApplicationStateBackground);
    if (unlikely(!_appActive))
        return nil;

    self = [super initWithFrame:frame];
    if (!self)
        return nil;

    _eaglEnabled = YES;
    _bufferNeedReset = YES;

    heimdall_mutex_init(&_mutex);
    heimdall_cond_init(&_gl_attached_wait);

    /* The following creates a new OpenGL ES context with the API version we
     * need. If there is already an active context created by another OpenGL
     * provider we cache it and restore analog to the
     * makeCurrent/releaseCurrent pattern used through-out the class */
    _eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    _previousEaglContext = nil;

    _layer = (CAEAGLLayer *)self.layer;
    _layer.drawableProperties = [NSDictionary dictionaryWithObject:kEAGLColorFormatRGBA8 forKey: kEAGLDrawablePropertyColorFormat];
    _layer.opaque = YES;

    /* Resize is done accordingly to the parent frame directly. */
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth
                          | UIViewAutoresizingFlexibleHeight;
    self.contentMode = UIViewContentModeScaleToFill;

    /* Connect to the parent UIView which will contain this surface.
     * Using a parent UIView makes it easier to handle window resize and
     * still have full control over the display object, since layers don't
     * need to draw anything to exist. */
    if (![self attachToWindow: _gl->surface])
        return nil;

    /* Listen application state change because we cannot use OpenGL in the
     * background. This should probably move to the vout_window reports in
     * the future, which could even signal that we need to disable the whole
     * display and potentially adapt playback for that. */

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationStateChanged:)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationStateChanged:)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationStateChanged:)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationStateChanged:)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];

    /* Setup the usual heimdall_gl_t callbacks before loading the API since we need
     * the get_proc_address symbol and a current context. */
    static const struct heimdall_gl_operations gl_ops =
    {
        .make_current = MakeCurrent,
        .release_current = ReleaseCurrent,
        .resize = Resize,
        .swap = Swap,
        .get_proc_address = GetSymbol,
        .close = Close,
    };
    gl->ops = &gl_ops;

    return self;
}

- (BOOL)attachToWindow:(heimdall_window_t*)wnd
{
    @try {
        UIView *viewContainer = (__bridge UIView*)wnd->handle.nsobject;
        /* get the object we will draw into */
        if (unlikely(viewContainer == nil)) {
            msg_Err(_gl, "provided view container is nil");
            return NO;
        }

        if (unlikely(![viewContainer respondsToSelector:@selector(isKindOfClass:)])) {
            msg_Err(_gl, "void pointer not an ObjC object");
            return NO;
        }

        if (unlikely(![viewContainer isKindOfClass:[UIView class]])) {
            msg_Err(_gl, "passed ObjC object not of class UIView");
            return NO;
        }

        /* Initial size setup */
        self.frame = viewContainer.bounds;

        [viewContainer addSubview:self];

        return YES;
    } @catch (NSException *exception) {
        msg_Err(_gl, "Handling the view container failed due to an Obj-C exception (%s, %s", [exception.name UTF8String], [exception.reason UTF8String]);
        return NO;
    }
}

- (void)detachFromWindow
{
    /* Flush the OpenGL pipeline before leaving. */
    heimdall_mutex_lock(&_mutex);
    if (_eaglEnabled)
        [self flushEAGLLocked];
    _eaglEnabled = NO;
    heimdall_mutex_unlock(&_mutex);

    /* We can drop the context as fast as possible since we don't use it
     * anymore. It avoids crashes with the context modifying destroyed
     * resources when it is being deallocated in the main thread. */
    _eaglContext = nil;

    /* This cannot be a synchronous dispatch because player is usually running
     * in the main thread and block the main thread unless we accept our fate
     * and exit here. */
    dispatch_async(dispatch_get_main_queue(), ^{
         /* Remove the external references to the view so that
          * dealloc can be called by ARC. */

         [[NSNotificationCenter defaultCenter] removeObserver:self
                                                         name:UIApplicationWillResignActiveNotification
                                                       object:nil];

         [[NSNotificationCenter defaultCenter] removeObserver:self
                                                         name:UIApplicationDidBecomeActiveNotification
                                                       object:nil];

         [[NSNotificationCenter defaultCenter] removeObserver:self
                                                         name:UIApplicationWillEnterForegroundNotification
                                                       object:nil];

         [[NSNotificationCenter defaultCenter] removeObserver:self
                                                         name:UIApplicationDidEnterBackgroundNotification
                                                       object:nil];
        assert(!_gl_attached);
        [self removeFromSuperview];
    });
}

- (void)didMoveToWindow
{
    self.contentScaleFactor = self.window.screen.scale;

    heimdall_mutex_lock(&_mutex);
    _bufferNeedReset = YES;
    heimdall_mutex_unlock(&_mutex);
}

- (BOOL)doResetBuffers:(heimdall_gl_t *)gl
{
    if (_frameBuffer != 0)
    {
        /* clear frame buffer */
        glDeleteFramebuffers(1, &_frameBuffer);
        _frameBuffer = 0;
    }

    if (_renderBuffer != 0)
    {
        /* clear render buffer */
        glDeleteRenderbuffers(1, &_renderBuffer);
        _renderBuffer = 0;
    }

    glGenFramebuffers(1, &_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);

    glGenRenderbuffers(1, &_renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);

    [_eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:_layer];

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        msg_Err(_gl, "Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return NO;
    }
    return YES;
}

- (BOOL)makeCurrent
{
    heimdall_mutex_lock(&_mutex);
    assert(!_gl_attached);

    if (unlikely(!_appActive))
    {
        heimdall_mutex_unlock(&_mutex);
        return NO;
    }

    assert(_eaglEnabled);
    _previousEaglContext = [EAGLContext currentContext];

    assert(_eaglContext);
    if (![EAGLContext setCurrentContext:_eaglContext])
    {
        heimdall_mutex_unlock(&_mutex);
        return NO;
    }

    BOOL resetBuffers = NO;


    if (unlikely(_bufferNeedReset))
    {
        _bufferNeedReset = NO;
        resetBuffers = YES;
    }

    _gl_attached = YES;

    heimdall_mutex_unlock(&_mutex);

    if (resetBuffers && ![self doResetBuffers:_gl])
    {
        [self releaseCurrent];
        return NO;
    }
    return YES;
}

- (void)releaseCurrent
{
    heimdall_mutex_lock(&_mutex);
    assert(_gl_attached);
    _gl_attached = NO;
    [EAGLContext setCurrentContext:_previousEaglContext];
    _previousEaglContext = nil;
    heimdall_cond_signal(&_gl_attached_wait);
    heimdall_mutex_unlock(&_mutex);
}

- (void)presentRenderbuffer
{
    [_eaglContext presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)layoutSubviews
{
    heimdall_mutex_lock(&_mutex);
    _bufferNeedReset = YES;
    heimdall_mutex_unlock(&_mutex);
}

- (void)flushEAGLLocked
{
    assert(_eaglEnabled);

    /* Ensure that all previously submitted commands are drained from the
     * command buffer and are executed by OpenGL ES before moving to the
     * background.*/
    EAGLContext *previousEaglContext = [EAGLContext currentContext];
    if ([EAGLContext setCurrentContext:_eaglContext])
        glFinish();
    [EAGLContext setCurrentContext:previousEaglContext];
}

- (void)applicationStateChanged:(NSNotification *)notification
{
    heimdall_mutex_lock(&_mutex);

    if ([[notification name] isEqualToString:UIApplicationWillResignActiveNotification])
        _appActive = NO;
    else if ([[notification name] isEqualToString:UIApplicationDidEnterBackgroundNotification])
    {
        _appActive = NO;

        /* Wait for the vout to unlock the eagl context before releasing
         * it. */
        while (_gl_attached && _eaglEnabled)
            heimdall_cond_wait(&_gl_attached_wait, &_mutex);

        /* _eaglEnabled can change during the heimdall_cond_wait
         * as the mutex is unlocked during that, so this check
         * has to be done after the heimdall_cond_wait! */
        if (_eaglEnabled) {
            [self flushEAGLLocked];
            _eaglEnabled = NO;
        }
    }
    else if ([[notification name] isEqualToString:UIApplicationWillEnterForegroundNotification])
        _eaglEnabled = YES;
    else
    {
        assert([[notification name] isEqualToString:UIApplicationDidBecomeActiveNotification]);
        _appActive = YES;
    }

    heimdall_mutex_unlock(&_mutex);
}

- (void)updateConstraints
{
    [super updateConstraints];
}

- (BOOL)isOpaque
{
    return YES;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
    /* Disable events for this view, as the vout_window view will be the one
     * handling them. */
    return nil;
}
@end



static int Open(heimdall_gl_t *gl, unsigned width, unsigned height,
                const struct heimdall_gl_cfg *gl_cfg)
{
    heimdall_window_t *wnd = gl->surface;

    if (gl_cfg->need_alpha)
    {
        msg_Err(gl, "Cannot support alpha yet");
        return HEIMDALL_ENOTSUP;
    }

    /* We only support UIView container window. */
    if (wnd->type != HEIMDALL_WINDOW_TYPE_NSOBJECT)
        return HEIMDALL_EGENERIC;

    @autoreleasepool {
        /* NOTE: we're using CFRunLoopPerformBlock with the "heimdall_runloop" tag
         * to avoid deadlocks between the window module (main thread) and the
         * display module, which would happen when using dispatch_sycn here. */
        heimdall_dispatch_sync(^{
            gl->sys = (__bridge_retained void*)[[HeimdallOpenGLES2VideoView alloc]
               initWithFrame:CGRectMake(0.,0.,width,height) gl:gl];
        });

        if (gl->sys == NULL)
        {
            msg_Err(gl, "Creating OpenGL ES 2 view failed");
            return HEIMDALL_EGENERIC;
        }
    }

    return HEIMDALL_SUCCESS;
}

heimdall_module_begin ()
    set_shortname (N_("CAEAGL"))
    set_description (N_("CAEAGL provider for OpenGL"))
    set_subcategory (SUBCAT_VIDEO_VOUT)
    set_callback_opengl_es2 (Open, 50)
    add_shortcut ("caeagl")
heimdall_module_end ()
