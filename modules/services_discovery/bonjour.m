/*****************************************************************************
 * bonjour.m: mDNS services discovery module based on Bonjour
 *****************************************************************************
 * Copyright (C) 2016-2026 VLC authors, VideoLAN and VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne@videolan.org>
 *          Marvin Scholz <epirat07@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_modules.h>
#include <heimdall_services_discovery.h>
#include <heimdall_renderer_discovery.h>
#include <heimdall_network.h>

#import <Foundation/Foundation.h>
#import <arpa/inet.h>

#pragma mark Function declarations

static int OpenSD( heimdall_object_t * );
static void CloseSD( heimdall_object_t * );

static int OpenRD( heimdall_object_t * );
static void CloseRD( heimdall_object_t * );

HEIMDALL_SD_PROBE_HELPER( "Bonjour", N_("Bonjour Network Discovery"), SD_CAT_LAN )
HEIMDALL_RD_PROBE_HELPER( "Bonjour_renderer", "Bonjour Renderer Discovery" )

typedef struct services_discovery_sys_t
{
    CFTypeRef _Nullable discoveryController;
} services_discovery_sys_t;

struct heimdall_renderer_discovery_sys
{
    CFTypeRef _Nullable discoveryController;
};

/*
 * Module descriptor
 */
heimdall_module_begin()
    set_shortname( "Bonjour" )
    set_description( N_( "Bonjour Network Discovery" ) )
    set_subcategory( SUBCAT_PLAYLIST_SD )
    set_capability( "services_discovery", 0 )
    set_callbacks( OpenSD, CloseSD )
    add_shortcut( "mdns", "bonjour" )
    HEIMDALL_SD_PROBE_SUBMODULE
    add_submodule() \
        set_description( N_( "Bonjour Renderer Discovery" ) )
        set_subcategory( SUBCAT_SOUT_RENDERER )
        set_capability( "renderer_discovery", 0 )
        set_callbacks( OpenRD, CloseRD )
        add_shortcut( "mdns_renderer", "bonjour_renderer" )
        HEIMDALL_RD_PROBE_SUBMODULE
heimdall_module_end()

NSString *const HeimdallBonjourProtocolName          = @"HeimdallBonjourProtocolName";
NSString *const HeimdallBonjourProtocolServiceName   = @"HeimdallBonjourProtocolServiceName";
NSString *const HeimdallBonjourProtocolModules       = @"HeimdallBonjourProtocolModules";
NSString *const HeimdallBonjourInputType             = @"HeimdallBonjourInputType";
NSString *const HeimdallBonjourIsRenderer            = @"HeimdallBonjourIsRenderer";
NSString *const HeimdallBonjourRendererFlags         = @"HeimdallBonjourRendererFlags";
NSString *const HeimdallBonjourRendererDemux         = @"HeimdallBonjourRendererDemux";

/*
 * For chromecast, the `ca=` is composed from (at least)
 * 0x01 to indicate video support
 * 0x04 to indivate audio support
 */
#define CHROMECAST_FLAG_VIDEO 0x01
#define CHROMECAST_FLAG_AUDIO 0x04

#pragma mark -
#pragma mark Interface definition
@interface HeimdallNetServiceDiscoveryController : NSObject <NSNetServiceBrowserDelegate, NSNetServiceDelegate>
{
    /* Stores all used service browsers, one for each protocol, usually */
    NSArray *_serviceBrowsers;

    /* Holds a required reference to all NSNetServices */
    NSMutableArray *_rawNetServices;

    /* Holds all successfully resolved NSNetServices */
    NSMutableArray *_resolvedNetServices;

    /* Holds the respective pointers to a heimdall_object for each resolved and added NSNetService */
    NSMutableArray *_inputItemsForNetServices;

    /* Stores all protocols that are currently discovered */
    NSArray *_activeProtocols;
}

@property (readonly) BOOL isRendererDiscovery;
@property (readonly, nonatomic) heimdall_object_t *p_this;

- (instancetype)initWithRendererDiscoveryObject:(heimdall_renderer_discovery_t *)p_rd;
- (instancetype)initWithServicesDiscoveryObject:(services_discovery_t *)p_sd;

- (void)startDiscovery;
- (void)stopDiscovery;

@end

static NSString * ipAddressAsStringForData(NSData * data)
{
    char addressBuffer[INET6_ADDRSTRLEN] = { 0 };
    NSString *returnValue = nil;

    if (data == nil) {
        return returnValue;
    }

    const heimdall_sockaddr *socketAddress = (const heimdall_sockaddr *)[data bytes];

    if (socketAddress) {
        const char *addressStr = NULL;
        if (socketAddress->sa.sa_family == AF_INET) {
            addressStr = inet_ntop(socketAddress->sa.sa_family,
                                           &(socketAddress->sin.sin_addr),
                                           addressBuffer,
                                           sizeof(addressBuffer));
        } else if (socketAddress->sa.sa_family == AF_INET6) {
            addressStr = inet_ntop(socketAddress->sa.sa_family,
                                           &(socketAddress->sin6.sin6_addr),
                                           addressBuffer,
                                           sizeof(addressBuffer));
        }
        else heimdall_assert_unreachable();

        if (addressStr != NULL) {
            returnValue = [NSString stringWithUTF8String:addressStr];
        }
    }
    return returnValue;
}

@implementation HeimdallNetServiceDiscoveryController

- (instancetype)initWithRendererDiscoveryObject:(heimdall_renderer_discovery_t *)p_rd
{
    self = [super init];
    if (self) {
        _p_this = HEIMDALL_OBJECT( p_rd );
        _isRendererDiscovery = YES;
    }

    return self;
}

- (instancetype)initWithServicesDiscoveryObject:(services_discovery_t *)p_sd
{
    self = [super init];
    if (self) {
        _p_this = HEIMDALL_OBJECT( p_sd );
        _isRendererDiscovery = NO;
    }

    return self;
}

- (void)startDiscovery
{
    NSDictionary *HeimdallFtpProtocol = @{ HeimdallBonjourProtocolName        : @"ftp",
                                      HeimdallBonjourProtocolServiceName : @"_ftp._tcp.",
                                      HeimdallBonjourIsRenderer          : @(NO),
                                      HeimdallBonjourInputType           : @(ITEM_TYPE_DIRECTORY),
                                      HeimdallBonjourProtocolModules     : @[@"ftp"]
                                      };
    NSDictionary *HeimdallSmbProtocol = @{ HeimdallBonjourProtocolName        : @"smb",
                                      HeimdallBonjourProtocolServiceName : @"_smb._tcp.",
                                      HeimdallBonjourIsRenderer          : @(NO),
                                      HeimdallBonjourInputType           : @(ITEM_TYPE_DIRECTORY),
                                      HeimdallBonjourProtocolModules     : @[@"dsm", @"smb2"]
                                      };
    NSDictionary *HeimdallNfsProtocol = @{ HeimdallBonjourProtocolName        : @"nfs",
                                      HeimdallBonjourProtocolServiceName : @"_nfs._tcp.",
                                      HeimdallBonjourIsRenderer          : @(NO),
                                      HeimdallBonjourInputType           : @(ITEM_TYPE_DIRECTORY),
                                      HeimdallBonjourProtocolModules     : @[@"nfs"]
                                      };
    NSDictionary *HeimdallSftpProtocol = @{ HeimdallBonjourProtocolName       : @"sftp",
                                       HeimdallBonjourProtocolServiceName: @"_sftp-ssh._tcp.",
                                       HeimdallBonjourIsRenderer         : @(NO),
                                       HeimdallBonjourInputType          : @(ITEM_TYPE_DIRECTORY),
                                       HeimdallBonjourProtocolModules     : @[@"sftp"]
                                       };
    NSDictionary *HeimdallWebDavProtocol = @{ HeimdallBonjourProtocolName       : @"webdav",
                                         HeimdallBonjourProtocolServiceName: @"_webdav._tcp.",
                                         HeimdallBonjourIsRenderer         : @(NO),
                                         HeimdallBonjourInputType          : @(ITEM_TYPE_DIRECTORY),
                                         HeimdallBonjourProtocolModules    : @[@"webdav"]
                                         };
    NSDictionary *HeimdallWebDavsProtocol = @{ HeimdallBonjourProtocolName       : @"webdavs",
                                          HeimdallBonjourProtocolServiceName: @"_webdavs._tcp.",
                                          HeimdallBonjourIsRenderer         : @(NO),
                                          HeimdallBonjourInputType          : @(ITEM_TYPE_DIRECTORY),
                                          HeimdallBonjourProtocolModules    : @[@"webdavs"]
                                          };
    NSDictionary *HeimdallCastProtocol = @{ HeimdallBonjourProtocolName       : @"chromecast",
                                       HeimdallBonjourProtocolServiceName: @"_googlecast._tcp.",
                                       HeimdallBonjourIsRenderer         : @(YES),
                                       HeimdallBonjourInputType          : @(ITEM_TYPE_DIRECTORY),
                                       HeimdallBonjourRendererFlags      : @(HEIMDALL_RENDERER_CAN_AUDIO),
                                       HeimdallBonjourRendererDemux      : @"cc_demux"
                                       };
    NSDictionary *HeimdallNdiProtocol = @{ HeimdallBonjourProtocolName        : @"ndi",
                                      HeimdallBonjourProtocolServiceName : @"_ndi._tcp.",
                                      HeimdallBonjourIsRenderer          : @(NO),
                                      HeimdallBonjourInputType           : @(ITEM_TYPE_STREAM),
                                      HeimdallBonjourProtocolModules     : @[@"noidea"]
                                      };

    NSArray *HeimdallSupportedProtocols = @[HeimdallFtpProtocol,
                                      HeimdallSmbProtocol,
                                      HeimdallNfsProtocol,
                                      HeimdallSftpProtocol,
                                      HeimdallWebDavProtocol,
                                      HeimdallWebDavsProtocol,
                                      HeimdallCastProtocol,
                                      HeimdallNdiProtocol];

    _rawNetServices = [[NSMutableArray alloc] init];
    _resolvedNetServices = [[NSMutableArray alloc] init];
    _inputItemsForNetServices = [[NSMutableArray alloc] init];

    NSMutableArray *discoverers = [[NSMutableArray alloc] init];
    NSMutableArray *protocols = [[NSMutableArray alloc] init];

    msg_Info(_p_this, "starting discovery");
    for (NSDictionary *protocol in HeimdallSupportedProtocols) {
        if (!_isRendererDiscovery) {
            /* Only discover services if we actually have at least one module that can handle their protocol */
            BOOL canHandle = NO;
            for (NSString *name in [protocol objectForKey: HeimdallBonjourProtocolModules]) {
                if (module_exists([name UTF8String])) {
                    canHandle = YES;
                    break;
                }
            }
            if (!canHandle) {
                msg_Dbg(_p_this, "no module to handle %s, skipping", [[protocol objectForKey: HeimdallBonjourProtocolName] UTF8String]);
                continue;
            }
        }

        /* Only discover hosts it they match the current mode (renderer or service) */
        if ([[protocol objectForKey: HeimdallBonjourIsRenderer] boolValue] != _isRendererDiscovery) {
            msg_Dbg(_p_this, "%s does not match current discovery mode, skipping", [[protocol objectForKey: HeimdallBonjourProtocolName] UTF8String]);
            continue;
        }

        NSNetServiceBrowser *serviceBrowser = [[NSNetServiceBrowser alloc] init];
        [serviceBrowser setDelegate:self];
        msg_Dbg(_p_this, "starting discovery for type %s", [[protocol objectForKey: HeimdallBonjourProtocolServiceName] UTF8String]);
        [serviceBrowser searchForServicesOfType:[protocol objectForKey: HeimdallBonjourProtocolServiceName] inDomain:@"local."];
        [discoverers addObject:serviceBrowser];
        [protocols addObject:protocol];
    }

    _serviceBrowsers = [discoverers copy];
    _activeProtocols = [protocols copy];
}

- (void)stopDiscovery
{
    [_serviceBrowsers makeObjectsPerformSelector:@selector(stop)];

    /* Work around a macOS 10.12 bug, see https://openradar.appspot.com/28943305 */
    [_serviceBrowsers makeObjectsPerformSelector:@selector(setDelegate:) withObject:nil];
    [_resolvedNetServices makeObjectsPerformSelector:@selector(setDelegate:) withObject:nil];

    for (NSValue *item in _inputItemsForNetServices) {
        if (_isRendererDiscovery) {
            [self removeRawRendererItem:item];
        } else {
            [self removeRawInputItem:item];
        }
    }

    [_inputItemsForNetServices removeAllObjects];
    [_resolvedNetServices removeAllObjects];
    msg_Info(_p_this, "stopped discovery");
}

#pragma mark - 
#pragma mark Delegate methods

- (void)netServiceBrowser:(NSNetServiceBrowser *)aNetServiceBrowser didFindService:(NSNetService *)aNetService moreComing:(BOOL)moreComing
{
    msg_Dbg(_p_this, "service found: %s (%s), resolving", [aNetService.name UTF8String], [aNetService.type UTF8String]);
    [_rawNetServices addObject:aNetService];
    aNetService.delegate = self;
    [aNetService resolveWithTimeout:5.];
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)aNetServiceBrowser didRemoveService:(NSNetService *)aNetService moreComing:(BOOL)moreComing
{
    msg_Dbg(self.p_this, "service disappeared: %s (%s), removing", [aNetService.name UTF8String], [aNetService.type UTF8String]);

    /* If the item was not looked-up yet, just remove it */
    if ([_rawNetServices containsObject:aNetService])
        [_rawNetServices removeObject:aNetService];

    /* If the item was already resolved, the associated input or renderer items needs to be removed as well */
    if ([_resolvedNetServices containsObject:aNetService]) {
        NSInteger index = [_resolvedNetServices indexOfObject:aNetService];
        if (index == NSNotFound) {
            return;
        }

        [_resolvedNetServices removeObjectAtIndex:index];

        if (_isRendererDiscovery) {
            [self removeRawRendererItem:[_inputItemsForNetServices objectAtIndex:index]];
        } else {
            [self removeRawInputItem:[_inputItemsForNetServices objectAtIndex:index]];
        }

        /* Remove item pointer from our lookup array */
        [_inputItemsForNetServices removeObjectAtIndex:index];
    }
}

- (void)netServiceDidResolveAddress:(NSNetService *)aNetService
{
    msg_Dbg(_p_this, "service resolved: %s", [aNetService.name UTF8String]);
    if (![_resolvedNetServices containsObject:aNetService]) {
        NSString *serviceType = aNetService.type;
        NSString *protocol = nil;
        enum input_item_type_e inputType = ITEM_TYPE_DIRECTORY;
        for (NSDictionary *protocolDefinition in _activeProtocols) {
            if ([serviceType isEqualToString:[protocolDefinition objectForKey:HeimdallBonjourProtocolServiceName]]) {
                protocol = [protocolDefinition objectForKey:HeimdallBonjourProtocolName];
                inputType = [[protocolDefinition objectForKey:HeimdallBonjourInputType] intValue];
            }
        }

        if (_isRendererDiscovery) {
            [self addResolvedRendererItem:aNetService withProtocol:protocol];
        } else {
            [self addResolvedInputItem:aNetService withProtocol:protocol inputType:inputType];
        }
    }

    [_rawNetServices removeObject:aNetService];
}

- (void)netService:(NSNetService *)aNetService didNotResolve:(NSDictionary *)errorDict
{
    msg_Warn(_p_this, "service resolution failed: %s, removing", [aNetService.name UTF8String]);
    [_rawNetServices removeObject:aNetService];
}

#pragma mark -
#pragma mark Helper methods

- (void)addResolvedRendererItem:(NSNetService *)netService withProtocol:(NSString *)protocol
{
    heimdall_renderer_discovery_t *p_rd = (heimdall_renderer_discovery_t *)_p_this;

    NSURLComponents *components = [[NSURLComponents alloc] init];
    components.scheme = protocol;
    components.host = netService.hostName;
    components.port = @(netService.port);
    NSString *uri = components.URL.absoluteString;
    NSDictionary *txtDict = [NSNetService dictionaryFromTXTRecordData:[netService TXTRecordData]];
    NSString *displayName = netService.name;
    int rendererFlags = 0;

    if ([netService.type isEqualToString:@"_googlecast._tcp."]) {
        NSData *modelData = [txtDict objectForKey:@"md"];
        NSData *nameData = [txtDict objectForKey:@"fn"];
        NSData *flagsData = [txtDict objectForKey:@"ca"];

        // Get CC capability flags from TXT data
        if (flagsData) {
            NSString *flagsString = [[NSString alloc] initWithData:flagsData encoding:NSUTF8StringEncoding];
            NSInteger flags = [flagsString intValue];

            if ((flags & CHROMECAST_FLAG_VIDEO) != 0) {
                rendererFlags |= HEIMDALL_RENDERER_CAN_VIDEO;
            }
            if ((flags & CHROMECAST_FLAG_AUDIO) != 0) {
                rendererFlags |= HEIMDALL_RENDERER_CAN_AUDIO;
            }
        }

        // Get CC model and name from TXT data
        if (modelData && nameData) {
            NSString *model = [[NSString alloc] initWithData:modelData encoding:NSUTF8StringEncoding];
            NSString *name = [[NSString alloc] initWithData:nameData encoding:NSUTF8StringEncoding];
            displayName = [NSString stringWithFormat:@"%@ (%@)", name, model];
        }
    }

    const char *extra_uri = rendererFlags & HEIMDALL_RENDERER_CAN_VIDEO ? NULL : "no-video";

    // TODO: Adapt to work with not just chromecast!
    heimdall_renderer_item_t *p_renderer_item = heimdall_renderer_item_new("chromecast", [displayName UTF8String],
                                                                 [uri UTF8String], extra_uri, "cc_demux",
                                                                 "", rendererFlags );
    if (p_renderer_item != NULL) {
        heimdall_rd_add_item( p_rd, p_renderer_item );
        [_inputItemsForNetServices addObject:[NSValue valueWithPointer:p_renderer_item]];
        [_resolvedNetServices addObject:netService];
    }
}

- (void)removeRawRendererItem:(NSValue *)item
{
    heimdall_renderer_discovery_t *p_rd = (heimdall_renderer_discovery_t *)_p_this;
    heimdall_renderer_item_t *input_item = [item pointerValue];

    if (input_item != NULL) {
        heimdall_rd_remove_item( p_rd, input_item );
        heimdall_renderer_item_release( input_item );
    }
}

- (void)addResolvedInputItem:(NSNetService *)netService withProtocol:(NSString *)protocol inputType:(enum input_item_type_e)inputType
{
    services_discovery_t *p_sd = (services_discovery_t *)_p_this;
    NSString *host = netService.hostName;

    if ([protocol isEqualToString:@"smb"]) {
        host = ipAddressAsStringForData(netService.addresses.firstObject);
    }
    NSURLComponents *components = [[NSURLComponents alloc] init];
    components.scheme = protocol;
    components.host = host;
    components.port = @(netService.port);
    NSString *uri = components.URL.absoluteString;

    input_item_t *p_input_item;
    if (inputType == ITEM_TYPE_STREAM)
        p_input_item = input_item_NewStream([uri UTF8String], [netService.name UTF8String], INPUT_DURATION_UNSET );
    else
        p_input_item = input_item_NewDirectory([uri UTF8String], [netService.name UTF8String], ITEM_NET );
    if (p_input_item != NULL) {
        services_discovery_AddItem(p_sd, p_input_item);
        [_inputItemsForNetServices addObject:[NSValue valueWithPointer:p_input_item]];
        [_resolvedNetServices addObject:netService];
    }
}

- (void)removeRawInputItem:(NSValue *)item
{
    services_discovery_t *p_sd = (services_discovery_t *)_p_this;
    input_item_t *input_item = [item pointerValue];

    if (input_item != NULL) {
        services_discovery_RemoveItem( p_sd, input_item );
        input_item_Release( input_item );
    }
}

@end

static int OpenSD(heimdall_object_t *p_this)
{
    services_discovery_t *p_sd = (services_discovery_t *)p_this;
    services_discovery_sys_t *p_sys = NULL;

    p_sd->p_sys = p_sys = calloc(1, sizeof(services_discovery_sys_t));
    if (!p_sys) {
        return HEIMDALL_ENOMEM;
    }

    p_sd->description = _("Bonjour Network Discovery");

    HeimdallNetServiceDiscoveryController *discoveryController = [[HeimdallNetServiceDiscoveryController alloc] initWithServicesDiscoveryObject:p_sd];

    p_sys->discoveryController = CFBridgingRetain(discoveryController);

    [discoveryController startDiscovery];

    return HEIMDALL_SUCCESS;
}

static void CloseSD(heimdall_object_t *p_this)
{
    services_discovery_t *p_sd = (services_discovery_t *)p_this;
    services_discovery_sys_t *p_sys = p_sd->p_sys;

    HeimdallNetServiceDiscoveryController *discoveryController = (__bridge HeimdallNetServiceDiscoveryController *)(p_sys->discoveryController);
    [discoveryController stopDiscovery];

    CFBridgingRelease(p_sys->discoveryController);
    discoveryController = nil;

    free(p_sys);
}

static int OpenRD(heimdall_object_t *p_this)
{
    heimdall_renderer_discovery_t *p_rd = (heimdall_renderer_discovery_t *)p_this;
    struct heimdall_renderer_discovery_sys *p_sys = NULL;

    p_rd->p_sys = p_sys = calloc(1, sizeof(struct heimdall_renderer_discovery_sys));
    if (!p_sys) {
        return HEIMDALL_ENOMEM;
    }

    HeimdallNetServiceDiscoveryController *discoveryController = [[HeimdallNetServiceDiscoveryController alloc] initWithRendererDiscoveryObject:p_rd];

    p_sys->discoveryController = CFBridgingRetain(discoveryController);

    [discoveryController startDiscovery];

    return HEIMDALL_SUCCESS;
}

static void CloseRD(heimdall_object_t *p_this)
{
    heimdall_renderer_discovery_t *p_rd = (heimdall_renderer_discovery_t *)p_this;
    struct heimdall_renderer_discovery_sys *p_sys = p_rd->p_sys;

    HeimdallNetServiceDiscoveryController *discoveryController = (__bridge HeimdallNetServiceDiscoveryController *)(p_sys->discoveryController);
    [discoveryController stopDiscovery];

    CFBridgingRelease(p_sys->discoveryController);
    discoveryController = nil;

    free(p_sys);
}
