/*****************************************************************************
 * HeimdallLibraryCollectionViewFlowLayout.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Claudio Cambra <claudio.cambra@gmail.com>
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

#import "HeimdallLibraryCollectionViewFlowLayout.h"

#import "extensions/NSAnimationContext+HeimdallAdditions.h"

#import "library/HeimdallLibraryCollectionViewDataSource.h"
#import "library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h"
#import "library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h"

#import "library/audio-library/HeimdallLibraryAudioDataSource.h"
#import "library/audio-library/HeimdallLibraryAudioGroupDataSource.h"
#import "library/audio-library/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.h"

#import "views/HeimdallUIUnits.h"

#pragma mark - Private data
static const NSUInteger kAnimationSteps = 32;
static const NSUInteger kWrapAroundValue = (NSUInteger)-1;

static const CGFloat kDetailViewCollapsedHeight = 0.;

typedef NS_ENUM(NSUInteger, HeimdallDetailViewAnimationType)
{
    HeimdallDetailViewAnimationTypeExpand,
    HeimdallDetailViewAnimationTypeCollapse,
};

typedef NS_ENUM(NSUInteger, HeimdallExpandAnimationType) {
    HeimdallExpandAnimationTypeVerticalMedium = 0,
    HeimdallExpandAnimationTypeVerticalLarge,
    HeimdallExpandAnimationTypeHorizontalMedium,
    HeimdallExpandAnimationTypeHorizontalLarge,
};

#pragma mark - HeimdallLibraryCollectionViewFlowLayout
@interface HeimdallLibraryCollectionViewFlowLayout ()
{
    CVDisplayLinkRef _displayLinkRef;

    NSArray *_mediumHeightAnimationSteps;
    NSArray *_largeHeightAnimationSteps;
    NSArray *_mediumWidthAnimationSteps;
    NSArray *_largeWidthAnimationSteps;
    
    HeimdallExpandAnimationType _animationType;
    CGFloat _prevProvidedAnimationStep;

    BOOL _invalidateAll;
    CGFloat _previousWidth;
    NSUInteger _displayLinkGeneration;
}

@property (nonatomic, readwrite) BOOL detailViewIsAnimating;
@property (nonatomic, readonly)  BOOL animationIsCollapse;
@property (nonatomic, readwrite) NSUInteger animationIndex;
@property (nonatomic, readwrite, strong) NSIndexPath *selectedIndexPath;

@end

@implementation HeimdallLibraryCollectionViewFlowLayout

- (void)dealloc
{
    [self releaseDisplayLink];
}

+ (instancetype)standardLayout
{
    const CGFloat collectionItemSpacing = HeimdallUIUnits.collectionViewItemSpacing;
    const NSEdgeInsets collectionViewSectionInset = HeimdallUIUnits.collectionViewSectionInsets;

    HeimdallLibraryCollectionViewFlowLayout * const collectionViewLayout = [[HeimdallLibraryCollectionViewFlowLayout alloc] init];
    collectionViewLayout.minimumLineSpacing = collectionItemSpacing;
    collectionViewLayout.minimumInteritemSpacing = collectionItemSpacing;
    collectionViewLayout.sectionInset = collectionViewSectionInset;

    return collectionViewLayout;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _mediumHeightAnimationSteps = [self generateAnimationStepsForExpandedViewDimension:HeimdallUIUnits.mediumDetailSupplementaryViewCollectionViewHeight];
        _largeHeightAnimationSteps = [self generateAnimationStepsForExpandedViewDimension:HeimdallUIUnits.largeDetailSupplementaryViewCollectionViewHeight];
        _mediumWidthAnimationSteps = [self generateAnimationStepsForExpandedViewDimension:HeimdallUIUnits.mediumDetailSupplementaryViewCollectionViewWidth];
        _largeWidthAnimationSteps = [self generateAnimationStepsForExpandedViewDimension:HeimdallUIUnits.largeDetailSupplementaryViewCollectionViewWidth];
        
        _animationType = HeimdallExpandAnimationTypeVerticalMedium;
        _prevProvidedAnimationStep = 0;

        _invalidateAll = NO;

        [self resetLayout];
    }

    return self;
}

- (NSArray *)generateAnimationStepsForExpandedViewDimension:(NSInteger)dimension
{
    NSMutableArray *generatedAnimationSteps = [NSMutableArray arrayWithCapacity:kAnimationSteps];

    // Easing out cubic
    for(NSUInteger i = 0; i < kAnimationSteps; ++i) {
        CGFloat progress = (CGFloat)i  / (CGFloat)kAnimationSteps;
        progress -= 1;
        generatedAnimationSteps[i] = @(dimension * (progress * progress * progress + 1) + kDetailViewCollapsedHeight);
    }

    return generatedAnimationSteps;
}

- (CGFloat)currentAnimationStep
{
    if (_animationIndex < 0 || _animationIndex >= kAnimationSteps) {
        return _prevProvidedAnimationStep; // Try to disguise problem
    }

    switch(_animationType) {
        case HeimdallExpandAnimationTypeHorizontalMedium:
            _prevProvidedAnimationStep = [_mediumWidthAnimationSteps[_animationIndex] floatValue];
            break;
        case HeimdallExpandAnimationTypeHorizontalLarge:
            _prevProvidedAnimationStep = [_largeWidthAnimationSteps[_animationIndex] floatValue];
            break;
        case HeimdallExpandAnimationTypeVerticalLarge:
            _prevProvidedAnimationStep = [_largeHeightAnimationSteps[_animationIndex] floatValue];
            break;
        case HeimdallExpandAnimationTypeVerticalMedium:
        default:
            _prevProvidedAnimationStep = [_mediumHeightAnimationSteps[_animationIndex] floatValue];
            break;
    }

    return _prevProvidedAnimationStep;
}

- (CGFloat)finalExpandedHeight
{
    switch(_animationType) {
        case HeimdallExpandAnimationTypeHorizontalMedium:
            return HeimdallUIUnits.mediumDetailSupplementaryViewCollectionViewWidth;
        case HeimdallExpandAnimationTypeHorizontalLarge:
            return HeimdallUIUnits.largeDetailSupplementaryViewCollectionViewWidth;
        case HeimdallExpandAnimationTypeVerticalLarge:
            return HeimdallUIUnits.largeDetailSupplementaryViewCollectionViewHeight;
        case HeimdallExpandAnimationTypeVerticalMedium:
        default:
            return HeimdallUIUnits.mediumDetailSupplementaryViewCollectionViewHeight;
    }
}

#pragma mark - Public methods
- (void)expandDetailSectionAtIndex:(NSIndexPath *)indexPath
{
    if([_selectedIndexPath isEqual:indexPath]) {
        return;
    }

    BOOL newItemOnSameRow = NO;
    if (_selectedIndexPath != nil) {
        NSCollectionViewItem * const oldSelectedItem = [self.collectionView itemAtIndexPath:_selectedIndexPath];
        NSCollectionViewItem * const newSelectedItem = [self.collectionView itemAtIndexPath:indexPath];

        newItemOnSameRow = oldSelectedItem.view.frame.origin.y == newSelectedItem.view.frame.origin.y;
    }

    _selectedIndexPath = indexPath;

    if (!newItemOnSameRow) {
        [self animateDetailViewWithAnimation:HeimdallDetailViewAnimationTypeExpand];

        NSRect frame = [self.collectionView layoutAttributesForItemAtIndexPath:indexPath].frame;
        frame.size.height += [self finalExpandedHeight] + HeimdallUIUnits.largeSpacing;

        [NSAnimationContext runAnimationRespectingPreferencesWithChanges:^(NSAnimationContext * const __unused context) {
            [self.collectionView.animator scrollRectToVisible:frame];
        } completionHandler:nil];
    } else {
        _animationIsCollapse = NO;
        
        if ([self.collectionView.dataSource conformsToProtocol:@protocol(HeimdallLibraryCollectionViewDataSource)]) {
            id<HeimdallLibraryCollectionViewDataSource> const libraryDataSource = 
                (id<HeimdallLibraryCollectionViewDataSource>)self.collectionView.dataSource;
            NSString * const supplementaryDetailViewKind = libraryDataSource.supplementaryDetailViewKind;
            if (supplementaryDetailViewKind != nil && supplementaryDetailViewKind.length > 0) {
                NSSet<NSIndexPath *> * const visibleSupplementaryIndexPaths = 
                    [self.collectionView indexPathsForVisibleSupplementaryElementsOfKind:supplementaryDetailViewKind];
                
                for (NSIndexPath * const supplementaryIndexPath in visibleSupplementaryIndexPaths) {
                    HeimdallLibraryCollectionViewSupplementaryDetailView * const supplementaryView = 
                        (HeimdallLibraryCollectionViewSupplementaryDetailView *)[self.collectionView supplementaryViewForElementKind:supplementaryDetailViewKind atIndexPath:supplementaryIndexPath];
                    if (supplementaryView != nil) {
                        supplementaryView.selectedItem = [self.collectionView itemAtIndexPath:indexPath];
                        supplementaryView.needsDisplay = YES;
                    }
                }
            }
        }
    }
}

- (void)collapseDetailSectionAtIndex:(NSIndexPath *)indexPath
{
    if(![_selectedIndexPath isEqual:indexPath]) {
        return;
    }

    [self animateDetailViewWithAnimation:HeimdallDetailViewAnimationTypeCollapse];
}

- (void)resetLayout
{
    [self releaseDisplayLink];

    _selectedIndexPath = nil;
    _detailViewIsAnimating = NO;
    _animationIndex = 0;

    [self invalidateLayout];
}

#pragma mark - Flow Layout methods

- (NSSize)collectionViewContentSize
{
    NSSize contentSize = [super collectionViewContentSize];

    if (!_selectedIndexPath) {
        return contentSize;
    }

    if (self.scrollDirection == NSCollectionViewScrollDirectionVertical) {
        contentSize.height += [self currentAnimationStep];
    } else if (self.scrollDirection == NSCollectionViewScrollDirectionHorizontal) {
        contentSize.width += [self currentAnimationStep];
    }
    return contentSize;
}

- (BOOL)shouldInvalidateLayoutForBoundsChange:(NSRect)newBounds
{
    const BOOL superResult = [super shouldInvalidateLayoutForBoundsChange:newBounds];
    const CGFloat newWidth = newBounds.size.width;

    if (newWidth != _previousWidth) {
        _invalidateAll = YES;
        _previousWidth = newWidth;
        return YES;
    }

    return superResult || _selectedIndexPath != nil;
}

- (void)invalidateLayoutWithContext:(NSCollectionViewLayoutInvalidationContext *)context
{
    NSCollectionViewFlowLayoutInvalidationContext *flowLayoutContext = (NSCollectionViewFlowLayoutInvalidationContext *)context;
    if (flowLayoutContext && _invalidateAll) {
        flowLayoutContext.invalidateFlowLayoutAttributes = YES;
        flowLayoutContext.invalidateFlowLayoutDelegateMetrics = YES;
        _invalidateAll = NO;
    }

    [super invalidateLayoutWithContext:context];
}

- (NSCollectionViewLayoutAttributes *)layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath
{
    if(_selectedIndexPath == nil || indexPath == _selectedIndexPath) {
        return [super layoutAttributesForItemAtIndexPath:indexPath];
    }

    NSCollectionViewLayoutAttributes * const attributes =
        [super layoutAttributesForItemAtIndexPath:indexPath].copy;
    [attributes setFrame:[self frameForDisplacedAttributes:attributes]];
    return attributes;
}

- (NSArray<__kindof NSCollectionViewLayoutAttributes *> *)layoutAttributesForElementsInRect:(NSRect)rect
{
    if (_selectedIndexPath == nil) {
        return [super layoutAttributesForElementsInRect:rect];
    }

    // Computed attributes from parent
    NSMutableArray<__kindof NSCollectionViewLayoutAttributes *> * const layoutAttributesArray =
        [super layoutAttributesForElementsInRect:rect].mutableCopy;
    for (NSUInteger i = 0; i < layoutAttributesArray.count; i++) {
        NSCollectionViewLayoutAttributes * const attributes = layoutAttributesArray[i].copy;
        NSString * const elementKind = attributes.representedElementKind;

        if (([elementKind isEqualToString:NSCollectionElementKindSectionHeader] && self.sectionHeadersPinToVisibleBounds) ||
            ([elementKind isEqualToString:NSCollectionElementKindSectionFooter] && self.sectionFootersPinToVisibleBounds)) {
            continue;
        }

        [attributes setFrame:[self frameForDisplacedAttributes:attributes]];
        layoutAttributesArray[i] = attributes;
    }

    if ([self.collectionView.dataSource conformsToProtocol:@protocol(HeimdallLibraryCollectionViewDataSource)]) {
        id<HeimdallLibraryCollectionViewDataSource> const libraryDataSource = 
            (id<HeimdallLibraryCollectionViewDataSource>)self.collectionView.dataSource;
        NSString * const supplementaryDetailViewKind = libraryDataSource.supplementaryDetailViewKind;
        if (supplementaryDetailViewKind != nil && supplementaryDetailViewKind.length > 0) {
            NSCollectionViewLayoutAttributes * const supplementaryDetailViewLayoutAttributes =
                [self layoutAttributesForSupplementaryViewOfKind:supplementaryDetailViewKind
                                                     atIndexPath:self.selectedIndexPath];
            [layoutAttributesArray addObject:supplementaryDetailViewLayoutAttributes];
        }
    }

    return layoutAttributesArray;
}

- (NSCollectionViewLayoutAttributes *)layoutAttributesForSupplementaryViewOfKind:(NSCollectionViewSupplementaryElementKind)elementKind
                                                                     atIndexPath:(NSIndexPath *)indexPath
{
    BOOL isLibrarySupplementaryView = NO;

    if ([elementKind isEqualToString:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind] ||
        [elementKind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind]) {

        isLibrarySupplementaryView = YES;
        _animationType = self.scrollDirection == NSCollectionViewScrollDirectionVertical ? HeimdallExpandAnimationTypeVerticalMedium : HeimdallExpandAnimationTypeHorizontalMedium;
    } else if ([elementKind isEqualToString:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind]) {

        isLibrarySupplementaryView = YES;
        _animationType = self.scrollDirection == NSCollectionViewScrollDirectionVertical ? HeimdallExpandAnimationTypeVerticalLarge : HeimdallExpandAnimationTypeHorizontalLarge;
    }

    if(isLibrarySupplementaryView) {
        NSCollectionViewLayoutAttributes *detailViewAttributes = [NSCollectionViewLayoutAttributes layoutAttributesForSupplementaryViewOfKind:elementKind
                                                                                                                                withIndexPath:indexPath];
        NSAssert1(detailViewAttributes != NULL,
                  @"Failed to create NSCollectionViewLayoutAttributes for view of kind %@.",
                  elementKind);

        const NSRect selectedItemFrame = [[self layoutAttributesForItemAtIndexPath:_selectedIndexPath] frame];

        if (self.scrollDirection == NSCollectionViewScrollDirectionVertical) {
            const float selectedItemFrameMaxY = _selectedIndexPath == nil ? 0 : NSMaxY(selectedItemFrame);
            detailViewAttributes.frame = NSMakeRect(NSMinX(self.collectionView.frame) + self.minimumInteritemSpacing,
                                                    selectedItemFrameMaxY + HeimdallUIUnits.mediumSpacing,
                                                    self.collectionViewContentSize.width - (self.minimumInteritemSpacing * 2),
                                                    [self currentAnimationStep]);

        } else if (self.scrollDirection == NSCollectionViewScrollDirectionHorizontal) {
            const float selectedItemFrameMinY = _selectedIndexPath == nil ? 0 : NSMinY(selectedItemFrame);
            const float selectedItemFrameMaxX = _selectedIndexPath == nil ? 0 : NSMaxX(selectedItemFrame);
            const float selectedItemFrameHeight = _selectedIndexPath == nil ? 0 : selectedItemFrame.size.height;
            detailViewAttributes.frame = NSMakeRect(selectedItemFrameMaxX + self.minimumInteritemSpacing,
                                                    selectedItemFrameMinY,
                                                    [self currentAnimationStep],
                                                    selectedItemFrameHeight);
        }

        return detailViewAttributes;
    }

    // Default attributes
    NSCollectionViewLayoutAttributes * const attributes =
        [super layoutAttributesForSupplementaryViewOfKind:elementKind atIndexPath:indexPath].copy;
    [attributes setFrame:[self frameForDisplacedAttributes:attributes]];
    return attributes;
}

- (NSSet<NSIndexPath *> *)indexPathsToDeleteForSupplementaryViewOfKind:(NSString *)elementKind
{
    if ([elementKind isEqualToString:HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailViewKind] ||
        [elementKind isEqualToString:HeimdallLibraryCollectionViewMediaItemSupplementaryDetailViewKind] ||
        [elementKind isEqualToString:HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailViewKind]) {

        return [self.collectionView indexPathsForVisibleSupplementaryElementsOfKind:elementKind];
    }
    return [NSSet set];
}

# pragma mark - Calculation of displaced frame attributes

- (NSRect)frameForDisplacedAttributes:(NSCollectionViewLayoutAttributes *)inAttributes
{
    if(inAttributes == nil) {
        return NSZeroRect;
    }

    NSRect attributesFrame = inAttributes.frame;

    if (self.selectedIndexPath) {
        NSCollectionViewLayoutAttributes *selectedItemLayoutAttributes = [self layoutAttributesForItemAtIndexPath:_selectedIndexPath];

        if(selectedItemLayoutAttributes == nil) {
            return attributesFrame;
        }

        NSRect selectedItemFrame = selectedItemLayoutAttributes.frame;

        if (self.scrollDirection == NSCollectionViewScrollDirectionVertical &&
            NSMinY(attributesFrame) > (NSMaxY(selectedItemFrame))) {

            attributesFrame.origin.y += [self currentAnimationStep] + HeimdallUIUnits.mediumSpacing;

        } else if (self.scrollDirection == NSCollectionViewScrollDirectionHorizontal &&
                   NSMinX(attributesFrame) > (NSMaxX(selectedItemFrame))) {

            attributesFrame.origin.y += [self currentAnimationStep] + HeimdallUIUnits.mediumSpacing;
            attributesFrame.origin.x += [self currentAnimationStep] + HeimdallUIUnits.mediumSpacing;
        }
    }

    return attributesFrame;
}

#pragma mark - Detail view animation
- (void)animateDetailViewWithAnimation:(HeimdallDetailViewAnimationType)type
{
    if (type == HeimdallDetailViewAnimationTypeExpand) {
        _animationIsCollapse = NO;
        _animationIndex = kWrapAroundValue;
    } else {
        _animationIsCollapse = YES;
        _animationIndex = kAnimationSteps;
    }

    _detailViewIsAnimating = YES;

    if (_displayLinkRef == NULL) {
        [self initDisplayLink];
    }
}

- (void)initDisplayLink
{
    const CVReturn createResult = CVDisplayLinkCreateWithActiveCGDisplays(&_displayLinkRef);

    if ((createResult != kCVReturnSuccess) || (_displayLinkRef == NULL)) {
        _detailViewIsAnimating = NO;
        return;
    }

    const NSUInteger displayLinkGeneration = ++_displayLinkGeneration;
    CVDisplayLinkRef const expectedDisplayLink = _displayLinkRef;
    __weak HeimdallLibraryCollectionViewFlowLayout * const weakSelf = self;
    const CVReturn handlerResult = CVDisplayLinkSetOutputHandler(_displayLinkRef, ^CVReturn(
        CVDisplayLinkRef displayLink __unused,
        const CVTimeStamp *inNow __unused,
        const CVTimeStamp *inOutputTime __unused,
        CVOptionFlags flagsIn __unused,
        CVOptionFlags *flagsOut __unused) {
        dispatch_async(dispatch_get_main_queue(), ^{
            HeimdallLibraryCollectionViewFlowLayout * const strongSelf = weakSelf;
            if (strongSelf == nil || strongSelf->_displayLinkGeneration != displayLinkGeneration ||
                strongSelf->_displayLinkRef != expectedDisplayLink) {
                return;
            }
            [strongSelf advanceDetailViewAnimation];
        });
        return kCVReturnSuccess;
    });

    if (handlerResult != kCVReturnSuccess || CVDisplayLinkStart(_displayLinkRef) != kCVReturnSuccess) {
        [self releaseDisplayLink];
        _detailViewIsAnimating = NO;
    }
}

- (void)advanceDetailViewAnimation
{
    BOOL animationFinished = NO;

    if (self.detailViewIsAnimating) {
        if (self.animationIsCollapse) {
            --self.animationIndex;
            animationFinished = (self.animationIndex == kWrapAroundValue);
        } else {
            ++self.animationIndex;
            animationFinished = (self.animationIndex == kAnimationSteps);
        }
    }

    if (self.detailViewIsAnimating == NO || animationFinished) {
        self.detailViewIsAnimating = NO;
        [self releaseDisplayLink];

        if (self.animationIsCollapse) {
            self.selectedIndexPath = nil;
            self.animationIndex = 0;
        } else {
            self.animationIndex = kAnimationSteps - 1;
        }
    }

    [self invalidateLayout];
}

- (void)releaseDisplayLink
{
    if (_displayLinkRef == NULL ) {
        return;
    }

    CVDisplayLinkRef const displayLink = _displayLinkRef;
    _displayLinkRef = NULL;
    ++_displayLinkGeneration;

    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);
}

@end
