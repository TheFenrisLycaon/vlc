/*****************************************************************************
 * HeimdallLibraryVideoCollectionViewsStackViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
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

#import "HeimdallLibraryHomeViewStackViewController.h"

#import "extensions/NSView+HeimdallAdditions.h"

#import "library/HeimdallLibraryCollectionViewDelegate.h"
#import "library/HeimdallLibraryCollectionViewFlowLayout.h"
#import "library/HeimdallLibraryCollectionViewSupplementaryElementView.h"
#import "library/HeimdallLibraryController.h"
#import "library/HeimdallLibraryHeroView.h"
#import "library/HeimdallLibraryModel.h"

#import "library/home-library/HeimdallLibraryHomeViewActionsView.h"
#import "library/home-library/HeimdallLibraryHomeViewAudioCarouselContainerView.h"
#import "library/home-library/HeimdallLibraryHomeViewContainerView.h"
#import "library/home-library/HeimdallLibraryHomeViewVideoCarouselContainerView.h"
#import "library/home-library/HeimdallLibraryHomeViewVideoContainerView.h"
#import "library/home-library/HeimdallLibraryHomeViewVideoContainerViewDataSource.h"
#import "library/home-library/HeimdallLibraryHomeViewVideoGridContainerView.h"

#import "library/video-library/HeimdallLibraryVideoGroupDescriptor.h"

#import "main/HeimdallMain.h"

#import "views/HeimdallSubScrollView.h"
#import "views/HeimdallUIUnits.h"

@interface HeimdallLibraryHomeViewStackViewController()
{
    NSMutableArray<NSView<HeimdallLibraryHomeViewContainerView> *> *_containers;
    NSUInteger _leadingContainerCount;
}
@end

@implementation HeimdallLibraryHomeViewStackViewController

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self setup];
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)setup
{
    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                            selector:@selector(recentsChanged:)
                                name:HeimdallLibraryModelRecentsMediaListReset
                                object:nil];
    [notificationCenter addObserver:self
                            selector:@selector(recentsChanged:)
                                name:HeimdallLibraryModelAllCachesDropped
                                object:nil];
    [notificationCenter addObserver:self
                            selector:@selector(recentsChanged:)
                                name:HeimdallLibraryModelAllCachesDropped
                                object:nil];
    [notificationCenter addObserver:self
                            selector:@selector(recentsChanged:)
                                name:HeimdallLibraryModelRecentsMediaItemDeleted
                                object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(audioRecentsChanged:)
                               name:HeimdallLibraryModelRecentAudioMediaListReset
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(audioRecentsChanged:)
                               name:HeimdallLibraryModelRecentAudioMediaItemDeleted
                             object:nil];

    _containers = [NSMutableArray array];
    _leadingContainerCount = 0;
    [self generateCustomContainers];
    [self generateGenericCollectionViewContainers];
}

- (void)generateCustomContainers
{
    _actionsView = [HeimdallLibraryHomeViewActionsView fromNibWithOwner:self];
    _heroView = [HeimdallLibraryHeroView fromNibWithOwner:self];
    _leadingContainerCount += 2;

    [self addCustomContainerViews];
    [self audioRecentsChanged:nil];
    [self recentsChanged:nil];
}

- (void)addCustomContainerViews
{
    [self addView:self.actionsView toStackView:self.collectionsStackView];
    [self addView:self.heroView toStackView:self.collectionsStackView];
    [self.heroView setOptimalRepresentedItem];
}

- (BOOL)recentMediaPresent
{
    HeimdallLibraryModel * const model = HeimdallMain.sharedInstance.libraryController.libraryModel;
    return model.numberOfRecentMedia > 0;
}

- (BOOL)recentAudioMediaPresent
{
    HeimdallLibraryModel * const model = HeimdallMain.sharedInstance.libraryController.libraryModel;
    return model.numberOfRecentAudioMedia > 0;
}

- (void)prependRecentItemsContainer:(NSView<HeimdallLibraryHomeViewContainerView> *)container
{
    [self.collectionsStackView insertArrangedSubview:container atIndex:_leadingContainerCount];
    [self setupContainerView:container withStackView:_collectionsStackView];
    [_containers insertObject:container atIndex:0];
    ++_leadingContainerCount;
}

- (void)removeRecentItemsContainer:(NSView<HeimdallLibraryHomeViewContainerView> *)container
{
    [self.collectionsStackView removeConstraints:container.constraintsWithSuperview];
    [self.collectionsStackView removeArrangedSubview:container];
    [_containers removeObject:container];
    container = nil; // Important to detect whether the view is presented or not
    --_leadingContainerCount;
}

- (void)recentsChanged:(NSNotification *)notification
{
    const BOOL shouldShowRecentsContainer = [self recentMediaPresent];
    const BOOL recentsContainerPresent = self.recentsView != nil;

    if (recentsContainerPresent == shouldShowRecentsContainer) {
        return;
    } else if (shouldShowRecentsContainer) {
        _recentsView = [[HeimdallLibraryHomeViewVideoCarouselContainerView alloc] init];
        self.recentsView.videoGroup = HeimdallMediaLibraryParentGroupTypeRecentVideos;
        [self prependRecentItemsContainer:self.recentsView];
    } else {
        [self removeRecentItemsContainer:_recentsView];
    }
}

- (void)audioRecentsChanged:(NSNotification *)notification
{
    const BOOL shouldShowAudioRecentsContainer = [self recentAudioMediaPresent];
    const BOOL audioRecentsContainerPresent = self.audioRecentsView != nil;

    if (audioRecentsContainerPresent == shouldShowAudioRecentsContainer) {
        return;
    } else if (shouldShowAudioRecentsContainer) {
        _audioRecentsView = [[HeimdallLibraryHomeViewAudioCarouselContainerView alloc] init];
        self.audioRecentsView.audioLibrarySegment = HeimdallAudioLibraryRecentsSegment;
        [self prependRecentItemsContainer:self.audioRecentsView];
    } else {
        [self removeRecentItemsContainer:_audioRecentsView];
    }
}

- (void)generateGenericCollectionViewContainers
{
    NSUInteger i = HeimdallMediaLibraryParentGroupTypeRecentVideos + 1;

    for (; i <= HeimdallMediaLibraryParentGroupTypeVideoLibrary; ++i) {
        HeimdallLibraryHomeViewVideoGridContainerView * const containerView = [[HeimdallLibraryHomeViewVideoGridContainerView alloc] init];
        containerView.videoGroup = i;
        [_containers addObject:containerView];
    }
}

- (void)reloadData
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.heroView setOptimalRepresentedItem];

        for (NSView<HeimdallLibraryHomeViewContainerView> * const containerView in self->_containers) {
            if ([containerView isKindOfClass:HeimdallLibraryHomeViewBaseCarouselContainerView.class]) {
                HeimdallLibraryHomeViewBaseCarouselContainerView * const baseContainerView = (HeimdallLibraryHomeViewBaseCarouselContainerView *)containerView;
                [baseContainerView.dataSource reloadData];
            }
        }
    });
}

- (void)connectContainers
{
    for (NSView<HeimdallLibraryHomeViewContainerView> * const container in _containers) {
        [container connect];
    }
}

- (void)disconnectContainers
{
    for (NSView<HeimdallLibraryHomeViewContainerView> * const container in _containers) {
        [container disconnect];
    }
}

- (NSArray<NSLayoutConstraint*> *)setupViewConstraints:(NSView *)view
                                         withStackView:(NSStackView *)stackView
{
    if (view == nil || stackView == nil) {
        return @[];
    }

    view.translatesAutoresizingMaskIntoConstraints = NO;

    NSArray<NSLayoutConstraint*> * const constraintsWithSuperview = @[
        [NSLayoutConstraint constraintWithItem:view
                                     attribute:NSLayoutAttributeLeft
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:stackView
                                     attribute:NSLayoutAttributeLeft
                                    multiplier:1
                                      constant:0
        ],
        [NSLayoutConstraint constraintWithItem:view
                                     attribute:NSLayoutAttributeRight
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:stackView
                                     attribute:NSLayoutAttributeRight
                                    multiplier:1
                                      constant:0
        ],
    ];
    [stackView addConstraints:constraintsWithSuperview];

    return constraintsWithSuperview;
}

- (void)setupContainerView:(NSView<HeimdallLibraryHomeViewContainerView> *)containerView
              withStackView:(NSStackView *)stackView
{
    if (containerView == nil || stackView == nil) {
        return;
    }

    containerView.constraintsWithSuperview = [self setupViewConstraints:containerView
                                                          withStackView:stackView];
}

- (void)addView:(NSView *)view
    toStackView:(NSStackView *)stackView
{
    if (view == nil || stackView == nil) {
        return;
    }

    [stackView addArrangedSubview:view];
    [self setupViewConstraints:view withStackView:stackView];
}

- (void)addContainerView:(NSView<HeimdallLibraryHomeViewContainerView> *)containerView
             toStackView:(NSStackView *)stackView
{
    if (containerView == nil || stackView == nil) {
        return;
    }

    [stackView addArrangedSubview:containerView];
    [self setupContainerView:containerView withStackView:stackView];
}

- (void)setCollectionsStackView:(NSStackView *)collectionsStackView
{
    NSParameterAssert(collectionsStackView);

    if (_collectionsStackView) {
        for (NSView<HeimdallLibraryHomeViewContainerView> * const containerView in _containers) {
            if (containerView.constraintsWithSuperview.count > 0) {
                [_collectionsStackView removeConstraints:containerView.constraintsWithSuperview];
            }
        }
    }

    _collectionsStackView = collectionsStackView;
    _collectionsStackView.spacing = HeimdallUIUnits.largeSpacing;
    _collectionsStackView.orientation = NSUserInterfaceLayoutOrientationVertical;
    _collectionsStackView.alignment = NSLayoutAttributeLeading;
    _collectionsStackView.distribution = NSStackViewDistributionFill;
    [_collectionsStackView setHuggingPriority:NSLayoutPriorityDefaultHigh
                               forOrientation:NSLayoutConstraintOrientationVertical];

    for (NSView * const view in _collectionsStackView.arrangedSubviews.copy) {
        [view removeFromSuperview];
    }

    [self addCustomContainerViews];

    for (NSView<HeimdallLibraryHomeViewContainerView> * const containerView in _containers) {
        [self addContainerView:containerView toStackView:_collectionsStackView];
    }
}

- (void)setCollectionViewItemSize:(NSSize)collectionViewItemSize
{
    _collectionViewItemSize = collectionViewItemSize;

     for (NSView<HeimdallLibraryHomeViewContainerView> *containerView in _containers) {
        if ([containerView isKindOfClass:HeimdallLibraryHomeViewVideoGridContainerView.class]) {
            HeimdallLibraryHomeViewVideoGridContainerView * const collectionViewContainerView = (HeimdallLibraryHomeViewVideoGridContainerView *)containerView;
            collectionViewContainerView.collectionViewDelegate.staticItemSize = collectionViewItemSize;
        }
    }
}

- (void)setCollectionViewSectionInset:(NSEdgeInsets)collectionViewSectionInset
{
    _collectionViewSectionInset = collectionViewSectionInset;

     for (NSView<HeimdallLibraryHomeViewContainerView> *containerView in _containers) {
        if ([containerView isKindOfClass:HeimdallLibraryHomeViewVideoGridContainerView.class]) {
            HeimdallLibraryHomeViewVideoGridContainerView * const collectionViewContainerView = (HeimdallLibraryHomeViewVideoGridContainerView *)containerView;
            collectionViewContainerView.collectionViewLayout.sectionInset = collectionViewSectionInset;
        }
     }
}

- (void)setCollectionViewMinimumLineSpacing:(CGFloat)collectionViewMinimumLineSpacing
{
    _collectionViewMinimumLineSpacing = collectionViewMinimumLineSpacing;

     for (NSView<HeimdallLibraryHomeViewContainerView> *containerView in _containers) {
        if ([containerView isKindOfClass:HeimdallLibraryHomeViewVideoGridContainerView.class]) {
            HeimdallLibraryHomeViewVideoGridContainerView * const collectionViewContainerView = (HeimdallLibraryHomeViewVideoGridContainerView *)containerView;
            collectionViewContainerView.collectionViewLayout.minimumLineSpacing = collectionViewMinimumLineSpacing;
        }
    }
}

- (void)setCollectionViewMinimumInteritemSpacing:(CGFloat)collectionViewMinimumInteritemSpacing
{
    _collectionViewMinimumInteritemSpacing = collectionViewMinimumInteritemSpacing;

    for (NSView<HeimdallLibraryHomeViewContainerView> * const containerView in _containers) {
        if ([containerView isKindOfClass:HeimdallLibraryHomeViewVideoGridContainerView.class]) {
            HeimdallLibraryHomeViewVideoGridContainerView * const collectionViewContainerView = (HeimdallLibraryHomeViewVideoGridContainerView *)containerView;
            collectionViewContainerView.collectionViewLayout.minimumInteritemSpacing = collectionViewMinimumInteritemSpacing;
        }
    }
}


- (NSView<HeimdallLibraryHomeViewContainerView> *)containerViewForGroup:(HeimdallMediaLibraryParentGroupType)group
{
    const NSUInteger index = [_containers indexOfObjectPassingTest:^BOOL(NSView<HeimdallLibraryHomeViewContainerView> * const container, const NSUInteger __unused idx, BOOL * const __unused stop) {
        if ([container conformsToProtocol:@protocol(HeimdallLibraryHomeViewVideoContainerView)]) {
            NSView<HeimdallLibraryHomeViewVideoContainerView> * const videoContainer = (NSView<HeimdallLibraryHomeViewVideoContainerView> *)container;
            return videoContainer.videoGroup == group;
        }
        return NO;
    }];

    if (index == NSNotFound) {
        return nil;
    }

    return [_containers objectAtIndex:index];
}

- (void)presentLibraryItem:(id<HeimdallMediaLibraryItemProtocol>)libraryItem
{
    if (libraryItem == nil) {
        return;
    }

    // TODO: Make this work agnostically of video type
    NSView<HeimdallLibraryHomeViewVideoContainerView> * const containerView = (NSView<HeimdallLibraryHomeViewVideoContainerView> *)[self containerViewForGroup:HeimdallMediaLibraryParentGroupTypeVideoLibrary];
    if (containerView == nil) {
        return;
    }

    [containerView presentLibraryItem:libraryItem];
}

@end
