/*****************************************************************************
 * HeimdallLibraryAudioDataSource.m: MacOS X interface module
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

#import "HeimdallLibraryAudioDataSource.h"

#import "extensions/NSPasteboardItem+HeimdallAdditions.h"
#import "extensions/NSString+Helpers.h"
#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallInputItem.h"
#import "library/HeimdallLibraryCarouselViewItemView.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewItem.h"
#import "library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryDataTypes.h"
#import "library/HeimdallLibraryModel.h"
#import "library/HeimdallLibraryRepresentedItem.h"
#import "library/HeimdallLibraryTableCellView.h"
#import "library/HeimdallLibraryWindow.h"

#import "library/audio-library/HeimdallLibraryAlbumTableCellView.h"
#import "library/audio-library/HeimdallLibraryAllAudioGroupsMediaLibraryItem.h"
#import "library/audio-library/HeimdallLibraryAudioGroupDataSource.h"
#import "library/audio-library/HeimdallLibrarySongsTableViewSongPlayingTableCellView.h"

#import "library/home-library/HeimdallLibraryHomeViewBaseCarouselContainerView.h"

#import "main/HeimdallMain.h"

#import "playqueue/HeimdallPlayerController.h"
#import "playqueue/HeimdallPlayQueueController.h"
#import "playqueue/HeimdallPlayQueueItem.h"
#import "playqueue/HeimdallPlayQueueModel.h"

#import "views/HeimdallImageView.h"
#import "views/HeimdallSubScrollView.h"
#import "views/HeimdallUIUnits.h"

NSString * const HeimdallLibrarySongsTableViewAutosaveName = @"HeimdallLibrarySongsTableView";
NSString * const HeimdallLibrarySongsTableViewHiddenColumnsPreferenceKey = @"HeimdallLibrarySongsTableViewHiddenColumns";

NSString * const HeimdallLibrarySongsTableViewSongPlayingColumnIdentifier = @"HeimdallLibrarySongsTableViewSongPlayingColumnIdentifier";
NSString * const HeimdallLibrarySongsTableViewTitleColumnIdentifier = @"HeimdallLibrarySongsTableViewTitleColumnIdentifier";
NSString * const HeimdallLibrarySongsTableViewDurationColumnIdentifier = @"HeimdallLibrarySongsTableViewDurationColumnIdentifier";
NSString * const HeimdallLibrarySongsTableViewArtistColumnIdentifier = @"HeimdallLibrarySongsTableViewArtistColumnIdentifier";
NSString * const HeimdallLibrarySongsTableViewAlbumColumnIdentifier = @"HeimdallLibrarySongsTableViewAlbumColumnIdentifier";
NSString * const HeimdallLibrarySongsTableViewGenreColumnIdentifier = @"HeimdallLibrarySongsTableViewGenreColumnIdentifier";
NSString * const HeimdallLibrarySongsTableViewPlayCountColumnIdentifier = @"HeimdallLibrarySongsTableViewPlayCountColumnIdentifier";
NSString * const HeimdallLibrarySongsTableViewYearColumnIdentifier = @"HeimdallLibrarySongsTableViewYearColumnIdentifier";

NSString * const HeimdallLibraryTitleSortDescriptorKey = @"HeimdallLibraryTitleSortDescriptorKey";
NSString * const HeimdallLibraryDurationSortDescriptorKey = @"HeimdallLibraryDurationSortDescriptorKey";
NSString * const HeimdallLibraryArtistSortDescriptorKey = @"HeimdallLibraryArtistSortDescriptorKey";
NSString * const HeimdallLibraryAlbumSortDescriptorKey = @"HeimdallLibraryAlbumSortDescriptorKey";
NSString * const HeimdallLibraryPlayCountSortDescriptorKey = @"HeimdallLibraryPlayCountSortDescriptorKey";
NSString * const HeimdallLibraryYearSortDescriptorKey = @"HeimdallLibraryYearSortDescriptorKey";
// TODO: Add sorting by genre

NSString * const HeimdallLibraryAudioDataSourceDisplayedCollectionChangedNotification = @"HeimdallLibraryAudioDataSourceDisplayedCollectionChangedNotification";

@interface HeimdallLibraryAudioDataSource () <NSMenuDelegate>

@property (readwrite, atomic) NSMutableArray *displayedCollection;
@property (readonly) BOOL isSearchActive;
@property (readonly) BOOL displayAllArtistsGenresTableEntry;
@property (readwrite, strong) NSMenu *songsTableHeaderMenu;

@end

@implementation HeimdallLibraryAudioDataSource

@synthesize currentParentType = _currentParentType;

- (void)setCollectionView:(NSCollectionView *)collectionView
{
    _collectionView = collectionView;
    if (_collectionView != nil) {
        [HeimdallLibraryAudioDataSource setupCollectionView:_collectionView];
    }
}

- (void)setSongsTableView:(NSTableView *)songsTableView
{
    _songsTableView = songsTableView;
    if (_songsTableView != nil) {
        [self setupSongsTableView];
    }
}

- (void)setCollectionSelectionTableView:(NSTableView *)collectionSelectionTableView
{
    _collectionSelectionTableView = collectionSelectionTableView;
    if (_collectionSelectionTableView != nil) {
        _collectionSelectionTableView.target = self;
        _collectionSelectionTableView.doubleAction = @selector(collectionSelectionDoubleClickAction:);
    }
}

- (void)setGridModeListTableView:(NSTableView *)gridModeListTableView
{
    _gridModeListTableView = gridModeListTableView;
    if (_gridModeListTableView != nil) {
        _gridModeListTableView.target = self;
        _gridModeListTableView.doubleAction = @selector(groupSelectionDoubleClickAction:);
    }
}

- (instancetype)init
{
    self = [super init];
    if(self) {
        _displayedCollectionUpdating = NO;
    }

    return self;
}

- (void)currentlyPlayingItemChanged:(NSNotification *)aNotification
{
    HeimdallPlayerController * const playerController = HeimdallMain.sharedInstance.playQueueController.playerController;
    HeimdallInputItem * const currentInputItem = playerController.currentMedia;
    if (!currentInputItem) {
        return;
    }

    if (self.currentParentType == HeimdallMediaLibraryParentGroupTypeAudioLibrary) {
        NSString * const currentItemMrl = currentInputItem.MRL;

        const NSUInteger itemIndexInDisplayedCollection = [self.displayedCollection indexOfObjectPassingTest:^BOOL(id element, NSUInteger __unused idx, BOOL * const __unused stop) {
            HeimdallMediaLibraryMediaItem * const mediaItem = (HeimdallMediaLibraryMediaItem *)element;
            return [mediaItem.inputItem.MRL isEqualToString:currentItemMrl];
        }];

        if (itemIndexInDisplayedCollection != NSNotFound) {
            [self.songsTableView scrollRowToVisible:itemIndexInDisplayedCollection];
        }
    }
}

- (size_t)collectionToDisplayCount
{
    switch(_currentParentType) {
    case HeimdallMediaLibraryParentGroupTypeAudioLibrary:
        return self.libraryModel.numberOfAudioMedia;
    case HeimdallMediaLibraryParentGroupTypeRecentAudios:
        return self.libraryModel.numberOfRecentAudioMedia;
    case HeimdallMediaLibraryParentGroupTypeAlbum:
        return self.libraryModel.numberOfAlbums;
    case HeimdallMediaLibraryParentGroupTypeArtist:
        return self.libraryModel.numberOfArtists;
    case HeimdallMediaLibraryParentGroupTypeGenre:
        return self.libraryModel.numberOfGenres;
    default:
        NSAssert(NO, @"current parent type should not be unknown, no collection to display");
        return 0;
    }
}

- (NSInteger)displayedCollectionCount
{
    return self.displayedCollection.count;
}

- (NSArray *)collectionToDisplay
{
    switch(_currentParentType) {
    case HeimdallMediaLibraryParentGroupTypeAudioLibrary:
        return self.libraryModel.listOfAudioMedia;
    case HeimdallMediaLibraryParentGroupTypeRecentAudios:
        return self.libraryModel.listOfRecentAudioMedia;
    case HeimdallMediaLibraryParentGroupTypeAlbum:
        return self.libraryModel.listOfAlbums;
    case HeimdallMediaLibraryParentGroupTypeArtist:
        return self.libraryModel.listOfArtists;
    case HeimdallMediaLibraryParentGroupTypeGenre:
        return self.libraryModel.listOfGenres;
    default:
        NSAssert(1, @"current parent type should not be unknown, no collection to display");
        return nil;
    }
}

- (void)libraryModelReset:(NSNotification * const)aNotification
{
    if(self.libraryModel == nil) {
        return;
    }

    [self reloadData];
}

- (void)libraryModelAudioMediaItemsReset:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeAudioLibrary
        && self.currentParentType != HeimdallMediaLibraryParentGroupTypeRecentAudios) {
        return;
    }

    [self libraryModelReset:aNotification];
}

- (void)libraryModelArtistsReset:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeArtist) {
        return;
    }

    [self libraryModelReset:aNotification];
}

- (void)libraryModelAlbumsReset:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeAlbum) {
        return;
    }

    [self libraryModelReset:aNotification];
}

- (void)libraryModelGenresReset:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeGenre) {
        return;
    }

    [self libraryModelReset:aNotification];
}

- (void)libraryModelAudioItemUpdated:(NSNotification * const)aNotification
{
    NSParameterAssert(aNotification);
    NSParameterAssert([aNotification.object conformsToProtocol:@protocol(HeimdallMediaLibraryItemProtocol)]);

    if(self.libraryModel == nil) {
        return;
    }

    const id<HeimdallMediaLibraryItemProtocol> item = (id<HeimdallMediaLibraryItemProtocol>)aNotification.object;
    [self reloadDataForMediaLibraryItem:item];
}

- (void)libraryModelAudioMediaItemUpdated:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeAudioLibrary
        && self.currentParentType != HeimdallMediaLibraryParentGroupTypeRecentAudios) {
        return;
    }

    [self libraryModelAudioItemUpdated:aNotification];
}

- (void)libraryModelArtistUpdated:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeArtist) {
        return;
    }

    [self libraryModelAudioItemUpdated:aNotification];
}

- (void)libraryModelAlbumUpdated:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeAlbum) {
        return;
    }

    [self libraryModelAudioItemUpdated:aNotification];
}

- (void)libraryModelGenreUpdated:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeGenre) {
        return;
    }

    [self libraryModelAudioItemUpdated:aNotification];
}

- (void)libraryModelAudioItemDeleted:(NSNotification * const)aNotification
{
    NSParameterAssert(aNotification);
    NSParameterAssert([aNotification.object conformsToProtocol:@protocol(HeimdallMediaLibraryItemProtocol)]);

    if(self.libraryModel == nil) {
        return;
    }

    const id <HeimdallMediaLibraryItemProtocol> item = (id<HeimdallMediaLibraryItemProtocol>)aNotification.object;
    [self deleteDataForMediaLibraryItem:item];
}

- (void)libraryModelAudioMediaItemDeleted:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeAudioLibrary
        && self.currentParentType != HeimdallMediaLibraryParentGroupTypeRecentAudios) {
        return;
    }

    [self libraryModelAudioItemDeleted:aNotification];
}

- (void)libraryModelArtistDeleted:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeArtist) {
        return;
    }

    [self libraryModelAudioItemDeleted:aNotification];
}

- (void)libraryModelAlbumDeleted:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeAlbum) {
        return;
    }

    [self libraryModelAudioItemDeleted:aNotification];
}

- (void)libraryModelGenreDeleted:(NSNotification * const)aNotification
{
    if (self.currentParentType != HeimdallMediaLibraryParentGroupTypeGenre) {
        return;
    }

    [self libraryModelAudioMediaItemDeleted:aNotification];
}

- (id<HeimdallMediaLibraryItemProtocol>)selectedCollectionViewItem
{
    NSIndexPath * const indexPath = self.collectionView.selectionIndexPaths.anyObject;
    if (!indexPath) {
        return nil;
    }

    return self.displayedCollection[indexPath.item];
}

- (NSUInteger)findSelectedItemNewIndex:(id<HeimdallMediaLibraryItemProtocol>)item
{
    return [self.displayedCollection indexOfObjectPassingTest:^BOOL(const id element, const NSUInteger __unused idx, BOOL * const __unused stop) {
        const id<HeimdallMediaLibraryItemProtocol> itemElement = (id<HeimdallMediaLibraryItemProtocol>)element;
        return itemElement.libraryID == item.libraryID;
    }];
}

- (void)setup
{
    // Force setAudioLibrarySegment to do something always on first try
    _audioLibrarySegment = HeimdallAudioLibraryUnknownSegment;
    [self connect];
}

- (void)connect
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelAudioMediaItemsReset:)
                               name:HeimdallLibraryModelAudioMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelReset:)
                               name:HeimdallLibraryModelAllCachesDropped
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelAudioMediaItemUpdated:)
                               name:HeimdallLibraryModelAudioMediaItemUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelAudioMediaItemDeleted:)
                               name:HeimdallLibraryModelAudioMediaItemDeleted
                             object:nil];

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelArtistsReset:)
                               name:HeimdallLibraryModelArtistListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelArtistUpdated:)
                               name:HeimdallLibraryModelArtistUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelArtistDeleted:)
                               name:HeimdallLibraryModelArtistDeleted
                             object:nil];

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelAlbumsReset:)
                               name:HeimdallLibraryModelAlbumListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelAlbumUpdated:)
                               name:HeimdallLibraryModelAlbumUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelAlbumDeleted:)
                               name:HeimdallLibraryModelAlbumDeleted
                             object:nil];

    [notificationCenter addObserver:self
                           selector:@selector(libraryModelGenresReset:)
                               name:HeimdallLibraryModelGenreListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelGenreUpdated:)
                               name:HeimdallLibraryModelGenreUpdated
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(libraryModelGenreDeleted:)
                               name:HeimdallLibraryModelGenreDeleted
                             object:nil];

    [notificationCenter addObserver:self
                           selector:@selector(currentlyPlayingItemChanged:)
                               name:HeimdallPlayerCurrentMediaItemChanged
                             object:nil];

    [self.audioGroupDataSource connect];
    [self reloadData];
}

- (void)disconnect
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [self.audioGroupDataSource disconnect];
}

+ (void)setupCollectionView:(NSCollectionView *)collectionView
{
    NSAssert(collectionView != nil, @"Collection view must not be nil");

    [collectionView registerClass:HeimdallLibraryCollectionViewItem.class
            forItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier];

    NSNib * const albumSupplementaryDetailView =
        [[NSNib alloc] initWithNibNamed:@"HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView" bundle:nil];
    [collectionView registerNib:albumSupplementaryDetailView
      forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind
                  withIdentifier:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewIdentifier];

    NSNib * const mediaItemSupplementaryDetailView = 
        [[NSNib alloc] initWithNibNamed:@"HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView" bundle:nil];
    [collectionView registerNib:mediaItemSupplementaryDetailView
      forSupplementaryViewOfKind:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind
                  withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewIdentifier];
}

- (void)setupSongsTableView
{
    NSAssert(self.songsTableView != nil, @"Songs table view must not be nil");

    self.songsTableView.target = self;
    self.songsTableView.doubleAction = @selector(songDoubleClickAction:);
    self.songsTableView.autosaveName = HeimdallLibrarySongsTableViewAutosaveName;
    self.songsTableView.autosaveTableColumns = YES;

    [self setupPrototypeSortDescriptorsForTableView:_songsTableView];
    [self setupExistingSortForTableView:_songsTableView];
    [self setupColumnVisibilityMenuForSongsTableView];
    [self restoreHiddenColumnsForSongsTableView];
}

- (void)setupPrototypeSortDescriptorsForTableView:(NSTableView *)tableView
{
    for(NSTableColumn * const column in tableView.tableColumns) {
        NSSortDescriptor * const columnSortDescriptor = [self sortDescriptorPrototypeForSongsTableViewColumnIdentifier:column.identifier];

        if(columnSortDescriptor) {
            column.sortDescriptorPrototype = columnSortDescriptor;
        }
    }
}

- (NSSortDescriptor *)sortDescriptorPrototypeForSongsTableViewColumnIdentifier:(NSString *)columnIdentifier
{
    if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewTitleColumnIdentifier]) {
        return [[NSSortDescriptor alloc] initWithKey:HeimdallLibraryTitleSortDescriptorKey ascending:true];

    } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewDurationColumnIdentifier]) {
        return [[NSSortDescriptor alloc] initWithKey:HeimdallLibraryDurationSortDescriptorKey ascending:true];

    } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewArtistColumnIdentifier]) {
        return [[NSSortDescriptor alloc] initWithKey:HeimdallLibraryArtistSortDescriptorKey ascending:true];

    } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewAlbumColumnIdentifier]) {
        return [[NSSortDescriptor alloc] initWithKey:HeimdallLibraryAlbumSortDescriptorKey ascending:true];

    } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewPlayCountColumnIdentifier]) {
        return [[NSSortDescriptor alloc] initWithKey:HeimdallLibraryPlayCountSortDescriptorKey ascending:true];

    } else if ([columnIdentifier isEqualToString:HeimdallLibrarySongsTableViewYearColumnIdentifier]) {
        return [[NSSortDescriptor alloc] initWithKey:HeimdallLibraryYearSortDescriptorKey ascending:true];

    }

    return nil;
}

- (void)setupExistingSortForTableView:(NSTableView *)tableView
{
    HeimdallLibraryController * const libraryController = HeimdallMain.sharedInstance.libraryController;
    const heimdall_ml_sorting_criteria_t existingSortCriteria = libraryController.lastSortingCriteria;

    NSString * const sortDescriptorKey = [self sortDescriptorKeyFromHeimdallMlSortingCriteria:existingSortCriteria];
    NSSortDescriptor * const sortDescriptor = [[NSSortDescriptor alloc] initWithKey:sortDescriptorKey
                                                                          ascending:!libraryController.descendingLibrarySorting];

    tableView.sortDescriptors = @[sortDescriptor];
}

- (NSString *)sortDescriptorKeyFromHeimdallMlSortingCriteria:(heimdall_ml_sorting_criteria_t)existingSortCriteria
{
    if (existingSortCriteria == HEIMDALL_ML_SORTING_DEFAULT) {
        return HeimdallLibraryTitleSortDescriptorKey;

    } else if (existingSortCriteria == HEIMDALL_ML_SORTING_DURATION) {
        return HeimdallLibraryDurationSortDescriptorKey;

    } else if (existingSortCriteria == HEIMDALL_ML_SORTING_ARTIST) {
        return HeimdallLibraryArtistSortDescriptorKey;

    } else if (existingSortCriteria == HEIMDALL_ML_SORTING_ALBUM) {
        return HeimdallLibraryAlbumSortDescriptorKey;

    } else if (existingSortCriteria == HEIMDALL_ML_SORTING_PLAYCOUNT) {
        return HeimdallLibraryPlayCountSortDescriptorKey;

    } else if (existingSortCriteria == HEIMDALL_ML_SORTING_RELEASEDATE) {
        return HeimdallLibraryYearSortDescriptorKey;

    }

    return HeimdallLibraryTitleSortDescriptorKey;
}

- (void)setupColumnVisibilityMenuForSongsTableView
{
    NSMenu * const menu = [[NSMenu alloc] initWithTitle:_NS("Columns")];
    menu.delegate = self;

    for (NSTableColumn * const column in self.songsTableView.tableColumns) {
        if ([column.identifier isEqualToString:HeimdallLibrarySongsTableViewSongPlayingColumnIdentifier]) {
            continue;
        }

        NSString * const title = column.headerCell.stringValue;
        NSMenuItem * const item = [[NSMenuItem alloc] initWithTitle:title
                                                             action:@selector(toggleSongsTableColumnVisibility:)
                                                      keyEquivalent:@""];
        item.target = self;
        item.representedObject = column;
        [menu addItem:item];
    }

    self.songsTableHeaderMenu = menu;
    self.songsTableView.headerView.menu = menu;
}

- (void)restoreHiddenColumnsForSongsTableView
{
    NSArray<NSString *> * const hiddenColumnIdentifiers =
        [NSUserDefaults.standardUserDefaults arrayForKey:HeimdallLibrarySongsTableViewHiddenColumnsPreferenceKey];
    NSSet<NSString *> * const hiddenColumns = [NSSet setWithArray:hiddenColumnIdentifiers];
    for (NSTableColumn * const column in self.songsTableView.tableColumns) {
        if ([column.identifier isEqualToString:HeimdallLibrarySongsTableViewSongPlayingColumnIdentifier] ||
            [column.identifier isEqualToString:HeimdallLibrarySongsTableViewTitleColumnIdentifier]) {
            column.hidden = NO;
            continue;
        }

        column.hidden = [hiddenColumns containsObject:column.identifier];
    }
}

- (void)persistHiddenColumnsForSongsTableView
{
    NSMutableArray<NSString *> * const hiddenColumnIdentifiers = NSMutableArray.array;
    for (NSTableColumn * const column in self.songsTableView.tableColumns) {
        if (column.isHidden &&
            ![column.identifier isEqualToString:HeimdallLibrarySongsTableViewSongPlayingColumnIdentifier] &&
            ![column.identifier isEqualToString:HeimdallLibrarySongsTableViewTitleColumnIdentifier]) {
            [hiddenColumnIdentifiers addObject:column.identifier];
        }
    }

    [NSUserDefaults.standardUserDefaults setObject:hiddenColumnIdentifiers
                                            forKey:HeimdallLibrarySongsTableViewHiddenColumnsPreferenceKey];
}

- (void)toggleSongsTableColumnVisibility:(NSMenuItem *)sender
{
    NSTableColumn * const column = sender.representedObject;
    if (![column isKindOfClass:NSTableColumn.class]) {
        return;
    }

    if ([column.identifier isEqualToString:HeimdallLibrarySongsTableViewTitleColumnIdentifier]) {
        return;
    }

    column.hidden = !column.isHidden;
    [self persistHiddenColumnsForSongsTableView];
}

- (void)menuNeedsUpdate:(NSMenu *)menu
{
    if (menu != self.songsTableHeaderMenu) {
        return;
    }

    for (NSMenuItem * const item in menu.itemArray) {
        NSTableColumn * const column = item.representedObject;
        if (![column isKindOfClass:NSTableColumn.class]) {
            continue;
        }

        item.state = column.isHidden ? NSControlStateValueOff : NSControlStateValueOn;
        item.enabled = ![column.identifier isEqualToString:HeimdallLibrarySongsTableViewTitleColumnIdentifier];
    }
}

- (void)resetLayoutsForOperation:(void(^)(void))operation
{
    HeimdallLibraryCollectionViewFlowLayout * const collectionViewFlowLayout = (HeimdallLibraryCollectionViewFlowLayout *)self.collectionView.collectionViewLayout;
    if (collectionViewFlowLayout) {
        [collectionViewFlowLayout resetLayout];
    }

    operation();
    [self setupExistingSortForTableView:self.songsTableView];
}

- (void)reloadData
{
    _displayedCollectionUpdating = YES;

    dispatch_async(dispatch_get_main_queue(), ^{
        NSTableView * const gridModeListTableView = self.gridModeListTableView;
        const NSInteger selectedGridModeRow = gridModeListTableView != nil
            ? gridModeListTableView.selectedRow
            : -1;
        const id<HeimdallMediaLibraryItemProtocol> selectedGridModeItem =
            selectedGridModeRow >= 0 && (NSUInteger)selectedGridModeRow < self.displayedCollection.count
                ? self.displayedCollection[selectedGridModeRow]
                : nil;

        NSTableView * const collectionSelectionTableView = self.collectionSelectionTableView;
        const NSInteger selectedListModeRow = collectionSelectionTableView != nil
            ? collectionSelectionTableView.selectedRow
            : -1;
        const id<HeimdallMediaLibraryItemProtocol> selectedListModeItem =
            selectedListModeRow >= 0 && (NSUInteger)selectedListModeRow < self.displayedCollection.count
                ? self.displayedCollection[selectedListModeRow]
                : nil;

        self.displayedCollection = [[self collectionToDisplay] mutableCopy];

        if (self.displayAllArtistsGenresTableEntry) {
            HeimdallLibraryAllAudioGroupsMediaLibraryItem *group;

            if (self->_currentParentType == HeimdallMediaLibraryParentGroupTypeGenre) {
                group = [[HeimdallLibraryAllAudioGroupsMediaLibraryItem alloc] initWithDisplayString:_NS("All genres")];
            } else if (self->_currentParentType == HeimdallMediaLibraryParentGroupTypeArtist) {
                group = [[HeimdallLibraryAllAudioGroupsMediaLibraryItem alloc] initWithDisplayString:_NS("All artists")];
            }

            NSAssert(group != nil, @"All items group should not be nil");
            [self.displayedCollection insertObject:group atIndex:0];
        }

        self->_displayedCollectionUpdating = NO;

        [self resetLayoutsForOperation:^{
            NSAssert(self.collectionView == nil || self.collectionView.dataSource == self, @"Cannot reload a collection view with a different data source");
            [self.collectionView reloadData];

            NSAssert(self.gridModeListTableView == nil || self.gridModeListTableView.dataSource == self, @"Cannot reload a table view with a different data source");
            [self.gridModeListTableView reloadData];

            NSAssert(self.collectionSelectionTableView == nil || self.collectionSelectionTableView.dataSource == self, @"Cannot reload a table view with a different data source");
            [self.collectionSelectionTableView reloadData];

            NSAssert(self.songsTableView == nil || self.songsTableView.dataSource == self, @"Cannot reload a table view with a different data source");
            [self.songsTableView reloadData];

            NSAssert(self.carouselView == nil || self.carouselView.dataSource == (id)self, @"Cannot reload a carousel view with a different data source");
            [self.carouselView reloadData];
        }];

        if (selectedGridModeItem != nil) {
            const NSUInteger selectedGridModeItemIndex =
                [self indexForMediaLibraryItemWithId:selectedGridModeItem.libraryID];
            if (selectedGridModeItemIndex != NSNotFound) {
                [self tableView:gridModeListTableView selectRowIndices:[NSIndexSet indexSetWithIndex:selectedGridModeItemIndex]];
            }
        }

        if (selectedListModeItem != nil) {
            const NSUInteger selectedListModeItemIndex =
                [self indexForMediaLibraryItemWithId:selectedListModeItem.libraryID];
            if (selectedListModeItemIndex != NSNotFound) {
                [self tableView:collectionSelectionTableView selectRowIndices:[NSIndexSet indexSetWithIndex:selectedListModeItemIndex]];
            }
        }

        if (self.isSearchActive &&
            (self.currentParentType == HeimdallMediaLibraryParentGroupTypeArtist ||
             self.currentParentType == HeimdallMediaLibraryParentGroupTypeAlbum ||
             self.currentParentType == HeimdallMediaLibraryParentGroupTypeGenre) &&
            self.displayedCollection.count > 0 &&
            self.collectionSelectionTableView.selectedRow < 0 &&
            self.gridModeListTableView.selectedRow < 0) {
            NSIndexSet * const firstRowIndexSet = [NSIndexSet indexSetWithIndex:0];
            [self tableView:collectionSelectionTableView selectRowIndices:firstRowIndexSet];
            [self tableView:gridModeListTableView selectRowIndices:firstRowIndexSet];
        }

        [NSNotificationCenter.defaultCenter postNotificationName:HeimdallLibraryAudioDataSourceDisplayedCollectionChangedNotification object:self];
    });
}

- (NSUInteger)indexForMediaLibraryItemWithId:(const int64_t)itemId
{
    return [self.displayedCollection indexOfObjectPassingTest:^BOOL(const id<HeimdallMediaLibraryItemProtocol> item, const NSUInteger __unused idx, BOOL * const __unused stop) {
        NSAssert(item != nil, @"Cache list should not contain nil items");
        return item.libraryID == itemId;
    }];
}

- (void)reloadDataForMediaLibraryItem:(const id<HeimdallMediaLibraryItemProtocol>)item
{
    const NSUInteger index = [self indexForMediaLibraryItemWithId:item.libraryID];
    if (index == NSNotFound) {
        return;
    }

    [self.displayedCollection replaceObjectAtIndex:index withObject:item];

    NSIndexPath * const indexPath = [NSIndexPath indexPathForItem:index inSection:0];
    NSIndexSet * const rowIndexSet = [NSIndexSet indexSetWithIndex:index];

    const NSRange songsTableColumnRange = NSMakeRange(0, self->_songsTableView.numberOfColumns);
    NSIndexSet * const songsTableColumnIndexSet = [NSIndexSet indexSetWithIndexesInRange:songsTableColumnRange];

    [self.collectionView reloadItemsAtIndexPaths:[NSSet setWithObject:indexPath]];
    [self.songsTableView reloadDataForRowIndexes:rowIndexSet columnIndexes:songsTableColumnIndexSet];

    // Don't update gridModeListSelectionCollectionView, let its HeimdallLibraryAudioGroupDataSource do it.
    // Also don't update collectionSelectionTableView, as this will only show artists/genres/albums

    [self.carouselView reloadData];
}

- (void)deleteDataForMediaLibraryItem:(const id<HeimdallMediaLibraryItemProtocol>)item
{
    [self resetLayoutsForOperation:^{
        const NSUInteger index = [self indexForMediaLibraryItemWithId:item.libraryID];
        if (index == NSNotFound) {
            return;
        }

        NSIndexPath * const indexPath = [NSIndexPath indexPathForItem:index inSection:0];
        NSIndexSet * const rowIndexSet = [NSIndexSet indexSetWithIndex:index];

        [self.collectionView performBatchUpdates:^{
            [self.displayedCollection removeObjectAtIndex:index];
            [self.collectionView deleteItemsAtIndexPaths:[NSSet setWithObject:indexPath]];
        } completionHandler:^(BOOL __unused finished) {
            [self.songsTableView removeRowsAtIndexes:rowIndexSet withAnimation:NSTableViewAnimationSlideUp];
            [self.carouselView reloadData];
        }];
    }];
}

- (void)setAudioLibrarySegment:(HeimdallAudioLibrarySegment)audioLibrarySegment
{
    if (audioLibrarySegment == _audioLibrarySegment) {
        return;
    }

    _displayedCollectionUpdating = YES;

    _audioLibrarySegment = audioLibrarySegment;
    switch (self.audioLibrarySegment) {
        case HeimdallAudioLibraryArtistsSegment:
            _currentParentType = HeimdallMediaLibraryParentGroupTypeArtist;
            break;
        case HeimdallAudioLibraryAlbumsSegment:
            _currentParentType = HeimdallMediaLibraryParentGroupTypeAlbum;
            break;
        case HeimdallAudioLibrarySongsSegment:
            _currentParentType = HeimdallMediaLibraryParentGroupTypeAudioLibrary;
            break;
        case HeimdallAudioLibraryRecentsSegment:
            _currentParentType = HeimdallMediaLibraryParentGroupTypeRecentAudios;
            break;
        case HeimdallAudioLibraryGenresSegment:
            _currentParentType = HeimdallMediaLibraryParentGroupTypeGenre;
            break;
        default:
            NSAssert(1, @"reached the unreachable");
            break;
    }

    self.audioGroupDataSource.representedAudioGroup = nil; // Clear whatever was being shown before

    [self.songsTableView deselectAll:self];
    [self.collectionSelectionTableView deselectAll:self];
    [self.collectionView deselectAll:self];

    [self reloadData];
}

#pragma mark - table view data source and delegation

- (BOOL)isSearchActive
{
    return self.libraryModel.filterString.length > 0;
}

- (BOOL)displayAllArtistsGenresTableEntry
{
    return !self.isSearchActive &&
           (self.currentParentType == HeimdallMediaLibraryParentGroupTypeGenre ||
            self.currentParentType == HeimdallMediaLibraryParentGroupTypeArtist);
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.displayedCollection.count;
}

- (NSInteger)rowForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return NSNotFound;
    }

    return [self indexForMediaLibraryItemWithId:libraryItem.libraryID];
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtRow:(NSInteger)row
                                       forTableView:(NSTableView *)tableView
{
    if (row < 0 || (NSUInteger)row >= self.displayedCollection.count) {
        return nil;
    }

    return self.displayedCollection[row];
}

- (void)applySelectionForTableView:(NSTableView *)tableView
{
    if (tableView == nil ||
        (tableView != self.collectionSelectionTableView && tableView != self.gridModeListTableView)) {
        return;
    }

    const NSInteger selectedRow = tableView.selectedRow;
    if (selectedRow >= 0 && (NSUInteger)selectedRow >= self.displayedCollection.count) {
        return;
    }

    const BOOL shouldClearSelection = self.currentParentType == HeimdallMediaLibraryParentGroupTypeAudioLibrary ||
                                      self.currentParentType == HeimdallMediaLibraryParentGroupTypeRecentAudios ||
                                      selectedRow < 0 ||
                                      self.displayedCollectionUpdating;

    const id<HeimdallMediaLibraryAudioGroupProtocol> selectedItem =
        shouldClearSelection ? nil : self.displayedCollection[selectedRow];
    self.audioGroupDataSource.representedAudioGroup = selectedItem;

    HeimdallLibraryRepresentedItem *representedItem = nil;
    NSString *fallbackTitle = nil;
    NSString *fallbackDetail = nil;

    if (selectedItem != nil) {
        representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:selectedItem parentType:self.currentParentType];
        fallbackDetail = selectedItem.primaryDetailString;

        if (self.displayAllArtistsGenresTableEntry && selectedRow == 0) {
            fallbackTitle = (self.currentParentType == HeimdallMediaLibraryParentGroupTypeGenre)
                ? _NS("All genres")
                : _NS("All artists");
        } else {
            fallbackTitle = selectedItem.displayString;
        }
    }

    [self.headerDelegate updateHeaderForTableView:tableView
                              withRepresentedItem:representedItem
                                    fallbackTitle:fallbackTitle
                                   fallbackDetail:fallbackDetail];
}

- (void)tableView:(NSTableView * const)tableView selectRowIndices:(NSIndexSet * const)indices
{
    NSParameterAssert(tableView);

    if (tableView != self.collectionSelectionTableView && tableView != self.gridModeListTableView) {
        return;
    }

    const NSInteger selectedRow = indices.firstIndex;
    if (tableView.selectedRowIndexes != indices) {
        [tableView selectRowIndexes:indices byExtendingSelection:NO];
        [tableView scrollRowToVisible:selectedRow];
    }

    if (selectedRow >= 0 && (NSUInteger)selectedRow >= self.displayedCollection.count) {
        return;
    }

    [self applySelectionForTableView:tableView];
}

- (void)tableView:(NSTableView *)tableView sortDescriptorsDidChange:(NSArray<NSSortDescriptor *> *)oldDescriptors
{
    const NSSortDescriptor * const sortDescriptor = tableView.sortDescriptors.firstObject;
    const heimdall_ml_sorting_criteria_t sortCriteria = [self sortDescriptorKeyToHeimdallMlSortingCriteria:sortDescriptor.key];

    [HeimdallMain.sharedInstance.libraryController sortByCriteria:sortCriteria andDescending:!sortDescriptor.ascending];
    [self reloadData];
}

- (heimdall_ml_sorting_criteria_t)sortDescriptorKeyToHeimdallMlSortingCriteria:(NSString *)sortDescriptorKey
{
    if ([sortDescriptorKey isEqualToString:HeimdallLibraryTitleSortDescriptorKey]) {
        return HEIMDALL_ML_SORTING_DEFAULT;

    } else if ([sortDescriptorKey isEqualToString:HeimdallLibraryDurationSortDescriptorKey]) {
        return HEIMDALL_ML_SORTING_DURATION;

    } else if ([sortDescriptorKey isEqualToString:HeimdallLibraryArtistSortDescriptorKey]) {
        return HEIMDALL_ML_SORTING_ARTIST;

    } else if ([sortDescriptorKey isEqualToString:HeimdallLibraryAlbumSortDescriptorKey]) {
        return HEIMDALL_ML_SORTING_ALBUM;

    } else if ([sortDescriptorKey isEqualToString:HeimdallLibraryPlayCountSortDescriptorKey]) {
        return HEIMDALL_ML_SORTING_PLAYCOUNT;

    } else if ([sortDescriptorKey isEqualToString:HeimdallLibraryYearSortDescriptorKey]) {
        return HEIMDALL_ML_SORTING_RELEASEDATE;

    }

    return HEIMDALL_ML_SORTING_DEFAULT;
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = [self libraryItemAtRow:row forTableView:tableView];

    return [NSPasteboardItem pasteboardItemWithLibraryItem:libraryItem];
}

#pragma mark - table view double click actions

- (void)groupSelectionDoubleClickAction:(id)sender
{
    NSTableView * const tableView = (NSTableView *)sender;
    NSParameterAssert(tableView != nil);

    const NSInteger clickedRow = tableView.clickedRow;
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = self.displayedCollection[clickedRow];
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem parentType:self.currentParentType];

    [representedItem play];
}

- (void)collectionSelectionDoubleClickAction:(id)sender
{
    const id<HeimdallMediaLibraryItemProtocol> libraryItem = self.displayedCollection[self.collectionSelectionTableView.selectedRow];
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem parentType:self.currentParentType];

    [representedItem play];
}

- (void)songDoubleClickAction:(id)sender
{
    NSAssert(_audioLibrarySegment == HeimdallAudioLibrarySongsSegment, @"Should not be possible to trigger this action from a non-song library view");
    HeimdallMediaLibraryMediaItem * const mediaItem = self.displayedCollection[_songsTableView.selectedRow];
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:mediaItem parentType:self.currentParentType];

    [representedItem play];
}

#pragma mark - collection view data source

- (NSInteger)collectionView:(NSCollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
    return self.displayedCollection.count;
}

- (NSInteger)numberOfSectionsInCollectionView:(NSCollectionView *)collectionView
{
    return 1;
}

- (NSCollectionViewItem *)collectionView:(NSCollectionView *)collectionView
     itemForRepresentedObjectAtIndexPath:(NSIndexPath *)indexPath
{
    HeimdallLibraryCollectionViewItem * const viewItem =
        [collectionView makeItemWithIdentifier:HeimdallLibraryCollectionViewItemIdentifier
                                  forIndexPath:indexPath];
    const id<HeimdallMediaLibraryItemProtocol> actualItem = self.displayedCollection[indexPath.item];
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:actualItem parentType:_currentParentType];

    viewItem.representedItem = representedItem;
    return viewItem;
}

- (NSView *)collectionView:(NSCollectionView *)collectionView
viewForSupplementaryElementOfKind:(NSCollectionViewSupplementaryElementKind)kind
               atIndexPath:(NSIndexPath *)indexPath
{
    if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind]) {

        HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView * const albumSupplementaryDetailView =
            [collectionView makeSupplementaryViewOfKind:kind 
                                         withIdentifier:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind
                                           forIndexPath:indexPath];

        HeimdallMediaLibraryAlbum * const album = self.displayedCollection[indexPath.item];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:album parentType:_currentParentType];

        albumSupplementaryDetailView.representedItem = representedItem;
        albumSupplementaryDetailView.selectedItem = [collectionView itemAtIndex:indexPath.item];
        albumSupplementaryDetailView.internalScrollView.scrollParentY = YES;

        HeimdallLibraryCollectionViewFlowLayout *flowLayout = (HeimdallLibraryCollectionViewFlowLayout*)collectionView.collectionViewLayout;
        if (flowLayout != nil) {
            albumSupplementaryDetailView.layoutScrollDirection = flowLayout.scrollDirection;
        }

        return albumSupplementaryDetailView;

    } else if ([kind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind]) {

        HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView* mediaItemSupplementaryDetailView = [collectionView makeSupplementaryViewOfKind:kind withIdentifier:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind forIndexPath:indexPath];

        const id<HeimdallMediaLibraryItemProtocol> actualItem = self.displayedCollection[indexPath.item];
        HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:actualItem parentType:_currentParentType];
        mediaItemSupplementaryDetailView.representedItem = representedItem;
        mediaItemSupplementaryDetailView.selectedItem = [collectionView itemAtIndex:indexPath.item];

        HeimdallLibraryCollectionViewFlowLayout *flowLayout = (HeimdallLibraryCollectionViewFlowLayout*)collectionView.collectionViewLayout;
        if (flowLayout != nil) {
            mediaItemSupplementaryDetailView.layoutScrollDirection = flowLayout.scrollDirection;
        }

        return mediaItemSupplementaryDetailView;
    }

    return nil;
}

- (id<HeimdallMediaLibraryItemProtocol>)libraryItemAtIndexPath:(NSIndexPath *)indexPath
                                        forCollectionView:(NSCollectionView *)collectionView
{
    const NSUInteger indexPathItem = indexPath.item;

    if (indexPathItem < 0 || indexPathItem >= self.displayedCollection.count) {
        return nil;
    }

    return self.displayedCollection[indexPathItem];
}

- (NSIndexPath *)indexPathForLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    const NSInteger libraryItemRow = [self rowForLibraryItem:libraryItem];
    if (libraryItemRow == NSNotFound) {
        return nil;
    }

    return [NSIndexPath indexPathForItem:libraryItemRow inSection:0];
}

// pragma mark: iCarouselDataSource methods
- (NSInteger)numberOfItemsInCarousel:(iCarousel *)carousel
{
    return self.displayedCollection.count;
}

- (NSView *)carousel:(iCarousel *)carousel viewForItemAtIndex:(NSInteger)index reusingView:(NSView *)view
{
    HeimdallLibraryCarouselViewItemView *carouselItemView = (HeimdallLibraryCarouselViewItemView *)view;
    if (carouselItemView == nil) {
        const NSRect itemFrame = NSMakeRect(0,
                                            0,
                                            HeimdallUIUnits.carouselViewItemViewHeight,
                                            HeimdallUIUnits.carouselViewItemViewHeight);
        carouselItemView = [HeimdallLibraryCarouselViewItemView fromNibWithOwner:self];
        carouselItemView.frame = itemFrame;
    }

    const id<HeimdallMediaLibraryItemProtocol> libraryItem = [self libraryItemAtRow:index forTableView:nil];
    HeimdallLibraryRepresentedItem * const representedItem = [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem
                                                                                             parentType:self.currentParentType];
    carouselItemView.representedItem = representedItem;
    return carouselItemView;
 }

- (NSArray<HeimdallLibraryRepresentedItem *> *)representedItemsAtIndexPaths:(NSSet<NSIndexPath *> *const)indexPaths
                                                     forCollectionView:(NSCollectionView *)collectionView
{
    NSMutableArray<HeimdallLibraryRepresentedItem *> * const representedItems =
        [NSMutableArray arrayWithCapacity:indexPaths.count];
    
    for (NSIndexPath * const indexPath in indexPaths) {
        const id<HeimdallMediaLibraryItemProtocol> libraryItem = 
            [self libraryItemAtIndexPath:indexPath forCollectionView:collectionView];
        HeimdallLibraryRepresentedItem * const representedItem = 
            [[HeimdallLibraryRepresentedItem alloc] initWithItem:libraryItem 
                                                 parentType:self.currentParentType];
        [representedItems addObject:representedItem];
    }

    return representedItems;
}


- (NSString *)supplementaryDetailViewKind
{
    switch (self.audioLibrarySegment) {
        case HeimdallAudioLibraryArtistsSegment:
        case HeimdallAudioLibraryGenresSegment:
            return nil;
        case HeimdallAudioLibraryAlbumsSegment:
            return HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind;
        case HeimdallAudioLibrarySongsSegment:
        default:
            return HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind;
    }
}

@end
