/*****************************************************************************
 * HeimdallLibraryAlbumTableCellView.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Felix Paul Kühne <fkuehne # lycaria -dot- org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#import "HeimdallLibraryAlbumTableCellView.h"

#import "extensions/NSColor+HeimdallAdditions.h"
#import "extensions/NSFont+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryImageCache.h"
#import "library/HeimdallLibraryItemInternalMediaItemsDataSource.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryTableView.h"
#import "library/HeimdallLibraryWindow.h"

#import "library/audio-library/HeimdallLibraryAlbumTracksTableViewDelegate.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallImageView.h"
#import "views/HeimdallTrackingView.h"
#import "views/HeimdallUIUnits.h"

NSString * const HeimdallAudioLibraryCellIdentifier = @"HeimdallAudioLibraryCellIdentifier";
NSString * const HeimdallLibraryAlbumTableCellTableViewIdentifier = @"HeimdallLibraryAlbumTableCellTableViewIdentifier";
NSString * const HeimdallLibraryAlbumTableCellTableViewColumnIdentifier = @"HeimdallLibraryAlbumTableCellTableViewColumnIdentifier";

const CGFloat HeimdallLibraryAlbumTableCellViewDefaultHeight = 168.;
static const CGFloat HeimdallLibraryInternalMediaItemIntercellSpacing = 2.;

@interface HeimdallNonScrollableScrollView : NSScrollView
@end

@implementation HeimdallNonScrollableScrollView

- (void)scrollWheel:(NSEvent *)event
{
    // Pass the scroll event to the next responder instead of handling it
    [self.nextResponder scrollWheel:event];
}

@end

@interface HeimdallLibraryAlbumTableCellView ()
{
    HeimdallLibraryController *_libraryController;
    HeimdallLibraryItemInternalMediaItemsDataSource *_tracksDataSource;
    HeimdallLibraryAlbumTracksTableViewDelegate *_tracksTableViewDelegate;
    HeimdallLibraryTableView *_tracksTableView;
    NSScrollView *_tracksScrollView;
    NSLayoutConstraint *_tracksScrollViewHeightConstraint;
    NSTableColumn *_column;
}

@property (readwrite, atomic) HeimdallLibraryRepresentedItem *protectedRepresentedItem;

@end

@implementation HeimdallLibraryAlbumTableCellView

+ (CGFloat)defaultHeight
{
    return HeimdallLibraryAlbumTableCellViewDefaultHeight;
}

+ (CGFloat)heightForAlbum:(HeimdallMediaLibraryAlbum *)album
{
    static CGFloat albumNameHeight;
    static CGFloat artistNameHeight;
    static CGFloat artworkAndSecondaryLabelsHeight;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        HeimdallLibraryAlbumTableCellView *prototype = [HeimdallLibraryAlbumTableCellView fromNibWithOwner:NSNull.null];
        albumNameHeight = prototype.albumNameTextField.frame.size.height;
        artistNameHeight = prototype.artistNameTextButton.frame.size.height;
        artworkAndSecondaryLabelsHeight = HeimdallUIUnits.largeSpacing +
                                          prototype.representedImageView.frame.size.height +
                                          HeimdallUIUnits.mediumSpacing +
                                          prototype.summaryTextField.frame.size.height +
                                          HeimdallUIUnits.smallSpacing +
                                          prototype.yearTextField.frame.size.height +
                                          HeimdallUIUnits.largeSpacing;
    });

    const NSUInteger numberOfTracks = album.numberOfTracks;
    const CGFloat tracksHeight = numberOfTracks * (HeimdallLibraryInternalMediaItemRowHeight + HeimdallLibraryInternalMediaItemIntercellSpacing);

    const CGFloat titleAndTableViewHeight = HeimdallUIUnits.largeSpacing +
                                            albumNameHeight +
                                            HeimdallUIUnits.smallSpacing +
                                            artistNameHeight +
                                            HeimdallUIUnits.mediumSpacing +
                                            tracksHeight +
                                            HeimdallUIUnits.largeSpacing;

    return MAX(MAX(titleAndTableViewHeight, artworkAndSecondaryLabelsHeight),
               HeimdallLibraryAlbumTableCellViewDefaultHeight);
}

- (CGFloat)height
{
    if (self.representedItem == nil) {
        return -1;
    }

    const CGFloat artworkAndSecondaryLabelsHeight = HeimdallUIUnits.largeSpacing +
                                                    _representedImageView.frame.size.height +
                                                    HeimdallUIUnits.mediumSpacing +
                                                    _summaryTextField.frame.size.height +
                                                    HeimdallUIUnits.smallSpacing +
                                                    _yearTextField.frame.size.height +
                                                    HeimdallUIUnits.largeSpacing;

    if(_tracksTableView == nil) {
        return artworkAndSecondaryLabelsHeight;
    }

    const CGFloat titleAndTableViewHeight = HeimdallUIUnits.largeSpacing +
                                            _albumNameTextField.frame.size.height +
                                            HeimdallUIUnits.smallSpacing +
                                            _artistNameTextButton.frame.size.height +
                                            HeimdallUIUnits.mediumSpacing +
                                            [self expectedTableViewHeight] +
                                            HeimdallUIUnits.largeSpacing;

    return titleAndTableViewHeight > artworkAndSecondaryLabelsHeight ? titleAndTableViewHeight : artworkAndSecondaryLabelsHeight;
}

- (CGFloat)expectedTableViewWidth
{
    // We are positioning the table view to the right of the album art, which means we need
    // to take into account the album's left spacing, right spacing, and the table view's
    // right spacing. In this case we are using large spacing for all of these. We also
    // throw in a little bit extra spacing to compensate for some mysterious internal spacing.
    return self.frame.size.width -
           _representedImageView.frame.size.width -
           HeimdallUIUnits.largeSpacing * 3.75;
}

- (CGFloat)expectedTableViewHeight
{
    if (self.representedItem == nil) {
        return -1;
    }

    HeimdallMediaLibraryAlbum * const album = (HeimdallMediaLibraryAlbum *)self.representedItem.item;
    if (album == nil) {
        return -1;
    }

    const NSUInteger numberOfTracks = album.numberOfTracks;
    return numberOfTracks * (HeimdallLibraryInternalMediaItemRowHeight + _tracksTableView.intercellSpacing.height);
}

- (void)awakeFromNib
{
    [self setupTracksTableView];
    self.albumNameTextField.font = NSFont.HeimdallLibrarySubsectionHeaderFont;
    self.artistNameTextButton.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;
    self.genreNameTextButton.font = NSFont.HeimdallLibrarySubsectionSubheaderFont;
    self.artistNameTextButton.action = @selector(primaryDetailAction:);
    self.genreNameTextButton.action = @selector(secondaryDetailAction:);
    self.trackingView.viewToHide = self.playInstantlyButton;

    if (@available(macOS 26.0, *)) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
        self.playInstantlyButton.bordered = YES;
        self.playInstantlyButton.bezelStyle = NSBezelStyleGlass;
        self.playInstantlyButton.borderShape = NSControlBorderShapeCircle;
        self.playInstantlyButton.image = [NSImage imageWithSystemSymbolName:@"play.fill" accessibilityDescription:nil];
        self.playInstantlyButton.imageScaling = NSImageScaleProportionallyUpOrDown;
        self.playInstantlyButton.controlSize = NSControlSizeExtraLarge;
        [NSLayoutConstraint activateConstraints:@[
            [self.playInstantlyButton.widthAnchor constraintEqualToConstant:HeimdallUIUnits.mediumPlaybackControlButtonSize],
            [self.playInstantlyButton.heightAnchor constraintEqualToConstant:HeimdallUIUnits.mediumPlaybackControlButtonSize],
        ]];
#endif
    }

    if (@available(macOS 10.14, *)) {
        self.artistNameTextButton.contentTintColor = NSColor.HeimdallAccentColor;
        self.genreNameTextButton.contentTintColor = NSColor.secondaryLabelColor;
    }

    [self prepareForReuse];

    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(handleAlbumUpdated:)
                               name:HeimdallLibraryModelAlbumUpdated
                             object:nil];
}

- (void)setupTracksTableView
{
    // Create scroll view container that doesn't intercept scroll events
    _tracksScrollView = [[HeimdallNonScrollableScrollView alloc] initWithFrame:NSZeroRect];
    _tracksScrollView.borderType = NSNoBorder;
    _tracksScrollView.hasVerticalScroller = NO;
    _tracksScrollView.hasHorizontalScroller = NO;
    _tracksScrollView.drawsBackground = NO;
    _tracksScrollView.verticalScrollElasticity = NSScrollElasticityNone;
    _tracksScrollView.horizontalScrollElasticity = NSScrollElasticityNone;
    
    _tracksTableView = [[HeimdallLibraryTableView alloc] initWithFrame:NSZeroRect];
    _tracksTableView.identifier = HeimdallLibraryAlbumTableCellTableViewIdentifier;
    _tracksTableView.allowsMultipleSelection = YES;
    _tracksTableView.headerView = nil;
    _column = [[NSTableColumn alloc] initWithIdentifier:HeimdallLibraryAlbumTableCellTableViewColumnIdentifier];
    _column.width = [self expectedTableViewWidth];
    _column.maxWidth = MAXFLOAT;
    [_tracksTableView addTableColumn:_column];

    if(@available(macOS 11.0, *)) {
        _tracksTableView.style = NSTableViewStyleFullWidth;
    }
    _tracksTableView.gridStyleMask = NSTableViewSolidHorizontalGridLineMask;
    _tracksTableView.intercellSpacing = NSMakeSize(_tracksTableView.intercellSpacing.width,
                                                   HeimdallLibraryInternalMediaItemIntercellSpacing);
    _tracksTableView.rowSizeStyle = NSTableViewRowSizeStyleCustom;
    _tracksTableView.usesAutomaticRowHeights = NO;
    _tracksTableView.rowHeight = HeimdallLibraryInternalMediaItemRowHeight;
    _tracksTableView.backgroundColor = [NSColor clearColor];

    _tracksDataSource = [[HeimdallLibraryItemInternalMediaItemsDataSource alloc] init];
    _tracksTableViewDelegate = [[HeimdallLibraryAlbumTracksTableViewDelegate alloc] init];
    _tracksTableView.dataSource = _tracksDataSource;
    _tracksTableView.delegate = _tracksTableViewDelegate;
    _tracksTableView.doubleAction = @selector(tracksTableViewDoubleClickAction:);
    _tracksTableView.target = self;

    _tracksScrollView.documentView = _tracksTableView;
    _tracksScrollView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_tracksScrollView];
    NSString *horizontalVisualConstraints = [NSString stringWithFormat:@"H:|-%f-[_representedImageView]-%f-[_tracksScrollView]-%f-|",
                                             HeimdallUIUnits.largeSpacing,
                                             HeimdallUIUnits.largeSpacing,
                                             HeimdallUIUnits.largeSpacing];
    NSString *verticalVisualContraints = [NSString stringWithFormat:@"V:|-%f-[_albumNameTextField]-%f-[_artistNameTextButton]-%f-[_tracksScrollView]->=%f-|",
                                          HeimdallUIUnits.largeSpacing,
                                          HeimdallUIUnits.smallSpacing,
                                          HeimdallUIUnits.mediumSpacing,
                                          HeimdallUIUnits.largeSpacing];
    NSDictionary *dict = NSDictionaryOfVariableBindings(_tracksScrollView, _representedImageView, _albumNameTextField, _artistNameTextButton);
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:horizontalVisualConstraints options:0 metrics:0 views:dict]];
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:verticalVisualContraints options:0 metrics:0 views:dict]];
    _tracksScrollViewHeightConstraint = [_tracksScrollView.heightAnchor constraintEqualToConstant:0.];
    _tracksScrollViewHeightConstraint.active = YES;

    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(handleTableViewSelectionIsChanging:)
                               name:NSTableViewSelectionIsChangingNotification
                             object:nil];
}

- (void)prepareForReuse
{
    [super prepareForReuse];

    self.representedImageView.image = nil;
    self.albumNameTextField.stringValue = @"";
    self.artistNameTextButton.title = @"";
    self.genreNameTextButton.title = @"";
    self.yearTextField.stringValue = @"";
    self.summaryTextField.stringValue = @"";
    self.yearTextField.hidden = NO;
    self.playInstantlyButton.hidden = YES;

    if (@available(macOS 10.14, *)) {
        self.artistNameTextButton.contentTintColor = NSColor.HeimdallAccentColor;
        self.genreNameTextButton.contentTintColor = NSColor.secondaryLabelColor;
    }

    _tracksDataSource.representedItem = nil;
    _tracksScrollViewHeightConstraint.constant = 0.;
    [_tracksTableView reloadData];
}

- (void)updateTracksScrollViewHeight
{
    _tracksScrollViewHeightConstraint.constant = MAX(0., [self expectedTableViewHeight]);
}

- (void)handleAlbumUpdated:(NSNotification *)notification
{
    NSParameterAssert(notification);
    if (self.representedItem == nil) {
        return;
    }

    HeimdallMediaLibraryAlbum * const album = (HeimdallMediaLibraryAlbum *)notification.object;
    if (album == nil || self.representedItem.item.libraryID != album.libraryID) {
        return;
    }

    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:album parentType:self.representedItem.parentType];
    self.representedItem = representedItem;
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];

    // As it expects a scrollview as a parent, the table view will always resize itself and
    // we cannot directly set its size. However, it resizes itself according to its columns
    // and rows. We can therefore implicitly set its width by resizing the single column we
    // are using.
    //
    // Since a column is just an NSObject and not an actual NSView object, however, we cannot
    // use the normal autosizing/constraint systems and must instead calculate and set its
    // size manually.
    _column.width = [self expectedTableViewWidth];
}

- (IBAction)playInstantly:(id)sender
{
    [self.representedItem play];
}

- (void)primaryDetailAction:(id)sender
{
    HeimdallMediaLibraryAlbum * const album = (HeimdallMediaLibraryAlbum *)self.representedItem.item;
    if (album == nil || !album.primaryActionableDetail) {
        return;
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = album.primaryActionableDetailLibraryItem;
    [libraryWindow presentLibraryItem:libraryItem];
}

- (void)secondaryDetailAction:(id)sender
{
    HeimdallMediaLibraryAlbum * const album = (HeimdallMediaLibraryAlbum *)self.representedItem.item;
    if (album == nil || !album.secondaryActionableDetail) {
        return;
    }

    HeimdallLibraryWindow * const libraryWindow = HeimdallMain.sharedInstance.libraryWindow;
    id<HeimdallMediaLibraryItemProtocol> libraryItem = album.secondaryActionableDetailLibraryItem;
    [libraryWindow presentLibraryItem:libraryItem];
}

- (HeimdallLibraryRepresentedItem *)representedItem
{
    return self.protectedRepresentedItem;
}

- (void)setRepresentedItem:(HeimdallLibraryRepresentedItem *)representedItem
{
    if (representedItem == nil) {
        return;
    }

    self.protectedRepresentedItem = representedItem;

    HeimdallMediaLibraryAlbum * const album = (HeimdallMediaLibraryAlbum *)self.representedItem.item;
    NSAssert(album != nil, @"Represented item should be a medialibraryalbum!");

    self.albumNameTextField.stringValue = album.displayString;
    self.artistNameTextButton.title = album.artistName;
    self.genreNameTextButton.title = album.genreString;

    if (album.year > 0) {
        self.yearTextField.intValue = album.year;
    } else {
        self.yearTextField.hidden = YES;
    }

    if (album.summary.length > 0) {
        self.summaryTextField.stringValue = album.summary;
    } else {
        self.summaryTextField.stringValue = album.durationString;
    }

    const BOOL primaryActionableDetail = album.primaryActionableDetail;
    const BOOL secondaryActionableDetail = album.secondaryActionableDetail;
    self.artistNameTextButton.enabled = primaryActionableDetail;
    self.genreNameTextButton.enabled = secondaryActionableDetail;
    if (@available(macOS 10.14, *)) {
        self.artistNameTextButton.contentTintColor = primaryActionableDetail ? NSColor.HeimdallAccentColor : NSColor.secondaryLabelColor;
        self.genreNameTextButton.contentTintColor = secondaryActionableDetail ? NSColor.secondaryLabelColor : NSColor.tertiaryLabelColor;
    }

    [self updateTracksScrollViewHeight];

    __weak typeof(self) weakSelf = self;
    [HeimdallLibraryImageCache thumbnailForLibraryItem:album withCompletion:^(NSImage * const thumbnail) {
        if (!weakSelf || weakSelf.representedItem.item != album) {
            return;
        }
        weakSelf.representedImageView.image = thumbnail;
    }];

    [_tracksDataSource setRepresentedItem:album withCompletion:^{
        __strong typeof(self) strongSelf = weakSelf;

        if (strongSelf) {
            [strongSelf->_tracksTableView reloadData];
            [strongSelf updateTracksScrollViewHeight];
        }
    }];
}

- (void)tracksTableViewDoubleClickAction:(id)sender
{
    if (!_libraryController) {
        _libraryController = HeimdallMain.sharedInstance.libraryController;
    }

    HeimdallMediaLibraryAlbum * const album = (HeimdallMediaLibraryAlbum *)self.representedItem.item;
    NSArray * const tracks = album.mediaItems;
    const NSUInteger trackCount = tracks.count;
    const NSInteger clickedRow = _tracksTableView.clickedRow;
    if (clickedRow >= 0 && (NSUInteger)clickedRow < trackCount) {
        HeimdallMediaLibraryMediaItem * const mediaItem = tracks[clickedRow];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:mediaItem parentType:HeimdallMediaLibraryParentGroupTypeAlbum];

        [representedItem play];
    }
}

- (void)handleTableViewSelectionIsChanging:(NSNotification *)notification
{
    NSParameterAssert(notification);
    NSTableView * const tableView = notification.object;
    NSAssert(tableView, @"Table view selection changing notification should carry valid table view");

    if (tableView != _tracksTableView &&
        tableView.identifier == HeimdallLibraryAlbumTableCellTableViewIdentifier) {

        [_tracksTableView deselectAll:self];
    }
}

@end
