actool_verbose = $(actool_verbose_$(V))
actool_verbose_ = $(actool_verbose__$(AM_DEFAULT_VERBOSITY))
actool_verbose_0 = @echo "  ACTOOL  " $@;
actool_verbose__0 = $(actool_verbose_0)

# All content of the asset catalog should be listed below
# so that it is properly rebuilt when its content changes.
ASSET_CATALOG_DEPS = \
	gui/macosx/Resources/Assets.xcassets/Contents.json \
	gui/macosx/Resources/Assets.xcassets/HeimdallAccentColor.colorset/Contents.json \
	gui/macosx/Resources/Assets.xcassets/dropzone.imageset/Contents.json \
	gui/macosx/Resources/Assets.xcassets/dropzone.imageset/dropzone-dark.png \
	gui/macosx/Resources/Assets.xcassets/dropzone.imageset/dropzone-dark@2x.png \
	gui/macosx/Resources/Assets.xcassets/dropzone.imageset/dropzone.png \
	gui/macosx/Resources/Assets.xcassets/dropzone.imageset/dropzone@2x.png

gui/macosx/Resources/Assets.car: gui/macosx/Resources/Assets.xcassets $(ASSET_CATALOG_DEPS)
	$(AM_V_at)$(MKDIR_P) "gui/macosx/Resources"
	$(actool_verbose)$(ACTOOL) --errors --warnings --notices \
	  --platform macosx --target-device mac --skip-app-store-deployment \
	  --minimum-deployment-target 10.13 --output-format human-readable-text \
	  --compile $(@D) $<

if ENABLE_MACOSX_UI
BUILT_SOURCES += gui/macosx/Resources/Assets.car
endif

EXTRA_DIST += $(ASSET_CATALOG_DEPS)

SUFFIXES += .xib .nib

xib_verbose = $(xib_verbose_$(V))
xib_verbose_ = $(xib_verbose__$(AM_DEFAULT_VERBOSITY))
xib_verbose_0 = @echo "  XIB     " $@;
xib_verbose__0 = $(xib_verbose_0)

.xib.nib:
	$(AM_V_at)$(MKDIR_P) "gui/macosx/UI"
	$(xib_verbose)$(XIB) --errors --warnings --notices \
	  --module Heimdall --auto-activate-custom-fonts --target-device mac \
	  --minimum-deployment-target 10.13 --output-format human-readable-text \
	  --compile $@ $<

if ENABLE_MACOSX_UI_SHADERS
SUFFIXES += .air .metal

metal_verbose = $(metal_verbose_$(V))
metal_verbose_ = $(metal_verbose__$(AM_DEFAULT_VERBOSITY))
metal_verbose_0 = @echo "  METAL   " $@;
metal_verbose__0 = $(metal_verbose_0)

# Rule to compile .metal to .air
.metal.air:
	$(AM_V_at)$(MKDIR_P) "gui/macosx/shaders"
	$(metal_verbose)$(METAL) -c $< -o $@

metallib_verbose = $(metallib_verbose_$(V))
metallib_verbose_ = $(metallib_verbose__$(AM_DEFAULT_VERBOSITY))
metallib_verbose_0 = @echo " METALLIB " $@;
metallib_verbose__0 = $(metallib_verbose_0)

libmacosx_plugin_la_METAL_srcs = gui/macosx/shaders/HeimdallSnowShader.metal
libmacosx_plugin_la_METAL_AIR_FILES = $(libmacosx_plugin_la_METAL_srcs:.metal=.air)
libmacosx_plugin_la_METAL_LIBRARY = gui/macosx/Resources/Shaders.metallib

$(libmacosx_plugin_la_METAL_LIBRARY): $(libmacosx_plugin_la_METAL_AIR_FILES)
	$(AM_V_at)$(MKDIR_P) "gui/macosx/Resources"
	$(metallib_verbose)$(METALLIB) $(libmacosx_plugin_la_METAL_AIR_FILES) -o $@

EXTRA_DIST += $(libmacosx_plugin_la_METAL_srcs)
BUILT_SOURCES += $(libmacosx_plugin_la_METAL_LIBRARY)
CLEANFILES += $(libmacosx_plugin_la_METAL_AIR_FILES) $(libmacosx_plugin_la_METAL_LIBRARY)

endif

libmacosx_plugin_la_OBJCFLAGS = $(AM_OBJCFLAGS) -fobjc-exceptions -fobjc-arc -I$(srcdir)/gui/macosx
libmacosx_plugin_la_LDFLAGS = $(AM_LDFLAGS) $(gui_RPATH) \
	-Wl,-framework,AVKit \
	-Wl,-framework,AVFoundation \
	-Wl,-framework,Cocoa \
	-Wl,-framework,CoreAudio \
	-Wl,-framework,CoreGraphics \
	-Wl,-framework,CoreVideo \
	-Wl,-framework,IOKit \
	-Wl,-framework,QuartzCore \
	-Wl,-framework,ScriptingBridge \
	-Wl,-framework,WebKit \
	-Wl,-framework,QuickLook \
	-Wl,-framework,QuickLookThumbnailing \
	-Wl,-weak_framework,MediaPlayer

if ENABLE_MACOSX_UI_SHADERS
libmacosx_plugin_la_LDFLAGS += -Wl,-framework,Metal \
	-Wl,-framework,MetalKit
endif

if HAVE_SPARKLE
libmacosx_plugin_la_LDFLAGS += -Wl,-framework,Sparkle
endif

libmacosx_plugin_la_SOURCES = \
	gui/macosx/coreinteraction/HeimdallHotkeysController.h \
	gui/macosx/coreinteraction/HeimdallHotkeysController.m \
	gui/macosx/coreinteraction/HeimdallVideoFilterHelper.h \
	gui/macosx/coreinteraction/HeimdallVideoFilterHelper.m \
	gui/macosx/extensions/NSAppearance+HeimdallAdditions.h \
	gui/macosx/extensions/NSAppearance+HeimdallAdditions.m \
	gui/macosx/extensions/NSAnimationContext+HeimdallAdditions.h \
	gui/macosx/extensions/NSAnimationContext+HeimdallAdditions.m \
	gui/macosx/extensions/NSArray+HeimdallAdditions.h \
	gui/macosx/extensions/NSArray+HeimdallAdditions.m \
	gui/macosx/extensions/NSColor+HeimdallAdditions.h \
	gui/macosx/extensions/NSColor+HeimdallAdditions.m \
	gui/macosx/extensions/NSFont+HeimdallAdditions.h \
	gui/macosx/extensions/NSFont+HeimdallAdditions.m \
	gui/macosx/extensions/NSImage+HeimdallAdditions.h \
	gui/macosx/extensions/NSImage+HeimdallAdditions.m \
	gui/macosx/extensions/NSIndexSet+HeimdallAdditions.h \
	gui/macosx/extensions/NSIndexSet+HeimdallAdditions.m \
	gui/macosx/extensions/NSMenu+HeimdallAdditions.h \
	gui/macosx/extensions/NSMenu+HeimdallAdditions.m \
	gui/macosx/extensions/NSMenuItem+HeimdallAdditions.h \
	gui/macosx/extensions/NSMenuItem+HeimdallAdditions.m \
	gui/macosx/extensions/NSScreen+HeimdallAdditions.h \
	gui/macosx/extensions/NSScreen+HeimdallAdditions.m \
	gui/macosx/extensions/NSScrollView+HeimdallAdditions.h \
	gui/macosx/extensions/NSScrollView+HeimdallAdditions.m \
	gui/macosx/extensions/NSString+Helpers.h \
	gui/macosx/extensions/NSString+Helpers.m \
	gui/macosx/extensions/NSView+HeimdallAdditions.h \
	gui/macosx/extensions/NSView+HeimdallAdditions.m \
	gui/macosx/extensions/NSWindow+HeimdallAdditions.h \
	gui/macosx/extensions/NSWindow+HeimdallAdditions.m \
	gui/macosx/extensions/NSPasteboardItem+HeimdallAdditions.h \
	gui/macosx/extensions/NSPasteboardItem+HeimdallAdditions.m \
	gui/macosx/extensions/NSTableCellView+HeimdallAdditions.h \
	gui/macosx/extensions/NSTableCellView+HeimdallAdditions.m \
	gui/macosx/extensions/NSTextField+HeimdallAdditions.h \
	gui/macosx/extensions/NSTextField+HeimdallAdditions.m \
	gui/macosx/extensions/HeimdallHexNumberFormatter.h \
	gui/macosx/extensions/HeimdallHexNumberFormatter.m \
	gui/macosx/extensions/HeimdallPositionFormatter.h \
	gui/macosx/extensions/HeimdallPositionFormatter.m \
	gui/macosx/imported/AppleRemote/AppleRemote.h \
	gui/macosx/imported/AppleRemote/AppleRemote.m \
	gui/macosx/imported/SPMediaKeyTap/SPMediaKeyTap.h \
	gui/macosx/imported/SPMediaKeyTap/SPMediaKeyTap.m \
	gui/macosx/library/HeimdallInputItem.h \
	gui/macosx/library/HeimdallInputItem.m \
	gui/macosx/library/HeimdallInputNode.h \
	gui/macosx/library/HeimdallInputNode.m \
	gui/macosx/library/HeimdallInputNodePathControl.h \
	gui/macosx/library/HeimdallInputNodePathControl.m \
	gui/macosx/library/HeimdallInputNodePathControlItem.h \
	gui/macosx/library/HeimdallInputNodePathControlItem.m \
	gui/macosx/library/HeimdallLibraryAbstractGroupingDataSource.h \
	gui/macosx/library/HeimdallLibraryAbstractGroupingDataSource.m \
	gui/macosx/library/HeimdallLibraryAbstractMediaLibrarySegmentViewController.h \
	gui/macosx/library/HeimdallLibraryAbstractMediaLibrarySegmentViewController.m \
	gui/macosx/library/HeimdallLibraryAbstractSegmentViewController.h \
	gui/macosx/library/HeimdallLibraryAbstractSegmentViewController.m \
	gui/macosx/library/HeimdallLibraryAddToPlaylistMenuController.h \
	gui/macosx/library/HeimdallLibraryAddToPlaylistMenuController.m \
	gui/macosx/library/HeimdallLibraryCarouselViewItemView.h \
	gui/macosx/library/HeimdallLibraryCarouselViewItemView.m \
	gui/macosx/library/HeimdallLibraryCollectionView.h \
	gui/macosx/library/HeimdallLibraryCollectionView.m \
	gui/macosx/library/HeimdallLibraryCollectionViewDataSource.h \
	gui/macosx/library/HeimdallLibraryCollectionViewDelegate.h \
	gui/macosx/library/HeimdallLibraryCollectionViewDelegate.m \
	gui/macosx/library/HeimdallLibraryCollectionViewFlowLayout.h \
	gui/macosx/library/HeimdallLibraryCollectionViewFlowLayout.m \
	gui/macosx/library/HeimdallLibraryCollectionViewItem.h \
	gui/macosx/library/HeimdallLibraryCollectionViewItem.m \
	gui/macosx/library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.h \
	gui/macosx/library/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.m \
	gui/macosx/library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.h \
	gui/macosx/library/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.m \
	gui/macosx/library/HeimdallLibraryCollectionViewSupplementaryDetailView.h \
	gui/macosx/library/HeimdallLibraryCollectionViewSupplementaryDetailView.m \
	gui/macosx/library/HeimdallLibraryCollectionViewSupplementaryElementView.h \
	gui/macosx/library/HeimdallLibraryCollectionViewSupplementaryElementView.m \
	gui/macosx/library/HeimdallLibraryController.h \
	gui/macosx/library/HeimdallLibraryController.m \
	gui/macosx/library/HeimdallLibraryConnectableDataSource.h \
	gui/macosx/library/HeimdallLibraryDataSource.h \
	gui/macosx/library/HeimdallLibraryDataTypes.h \
	gui/macosx/library/HeimdallLibraryDataTypes.m \
	gui/macosx/library/HeimdallLibraryDynamicToolbarFlagsCapable.h \
	gui/macosx/library/HeimdallLibraryDynamicViewModeCapable.h \
	gui/macosx/library/HeimdallLibraryNameCache.h \
	gui/macosx/library/HeimdallLibraryNameCache.m \
	gui/macosx/library/HeimdallLibraryGroupHeaderDelegate.h \
	gui/macosx/library/HeimdallLibraryHeroView.h \
	gui/macosx/library/HeimdallLibraryHeroView.m \
	gui/macosx/library/HeimdallLibraryImageCache.h \
	gui/macosx/library/HeimdallLibraryImageCache.m \
	gui/macosx/library/HeimdallThumbnailRequest.h \
	gui/macosx/library/HeimdallThumbnailRequest.m \
	gui/macosx/library/HeimdallLibraryItemInternalMediaItemsDataSource.h \
	gui/macosx/library/HeimdallLibraryItemInternalMediaItemsDataSource.m \
	gui/macosx/library/HeimdallLibraryItemPresentingCapable.h \
	gui/macosx/library/HeimdallLibraryMasterDetailViewTableViewDataSource.h \
	gui/macosx/library/HeimdallLibraryMasterDetailViewTableViewDelegate.h \
	gui/macosx/library/HeimdallLibraryMasterDetailViewTableViewDelegate.m \
	gui/macosx/library/HeimdallLibraryMenuController.h \
	gui/macosx/library/HeimdallLibraryMenuController.m \
	gui/macosx/library/HeimdallLibraryModel.h \
	gui/macosx/library/HeimdallLibraryModel.m \
	gui/macosx/library/HeimdallLibraryModelChangeDelegate.h \
	gui/macosx/library/HeimdallLibraryModelChangeDelegate.m \
	gui/macosx/library/HeimdallLibraryRepresentedItem.h \
	gui/macosx/library/HeimdallLibraryRepresentedItem.m \
	gui/macosx/library/HeimdallLibrarySegment.h \
	gui/macosx/library/HeimdallLibrarySegment.m \
	gui/macosx/library/HeimdallLibrarySegmentBookmarkedLocation.h \
	gui/macosx/library/HeimdallLibrarySegmentBookmarkedLocation.m \
	gui/macosx/library/HeimdallLibrarySortingMenuController.h \
	gui/macosx/library/HeimdallLibrarySortingMenuController.m \
	gui/macosx/library/HeimdallLibraryTableView.h \
	gui/macosx/library/HeimdallLibraryTableView.m \
	gui/macosx/library/HeimdallLibraryTableViewDataSource.h \
	gui/macosx/library/HeimdallLibrarySectionedTableViewDataSource.h \
	gui/macosx/library/HeimdallLibraryTableViewDelegate.h \
	gui/macosx/library/HeimdallLibraryTableViewDelegate.m \
	gui/macosx/library/HeimdallLibraryTableCellView.h \
	gui/macosx/library/HeimdallLibraryTableCellView.m \
	gui/macosx/library/HeimdallLibraryTableCellViewProtocol.h \
	gui/macosx/library/HeimdallLibraryTwoPaneSplitViewDelegate.h \
	gui/macosx/library/HeimdallLibraryTwoPaneSplitViewDelegate.m \
	gui/macosx/library/HeimdallLibraryWindow.h \
	gui/macosx/library/HeimdallLibraryWindow.m \
	gui/macosx/library/HeimdallLibraryWindowController.h \
	gui/macosx/library/HeimdallLibraryWindowController.m \
	gui/macosx/library/HeimdallLibraryWindowAbstractSidebarViewController.h \
	gui/macosx/library/HeimdallLibraryWindowAbstractSidebarViewController.m \
	gui/macosx/library/HeimdallLibraryWindowChaptersSidebarViewController.h \
	gui/macosx/library/HeimdallLibraryWindowChaptersSidebarViewController.m \
	gui/macosx/library/HeimdallLibraryWindowNavigationSidebarOutlineView.h \
	gui/macosx/library/HeimdallLibraryWindowNavigationSidebarOutlineView.m \
	gui/macosx/library/HeimdallLibraryWindowNavigationSidebarViewController.h \
	gui/macosx/library/HeimdallLibraryWindowNavigationSidebarViewController.m \
	gui/macosx/library/HeimdallLibraryWindowPersistentPreferences.h \
	gui/macosx/library/HeimdallLibraryWindowPersistentPreferences.m \
	gui/macosx/library/HeimdallLibraryWindowPlayQueueSidebarViewController.h \
	gui/macosx/library/HeimdallLibraryWindowPlayQueueSidebarViewController.m \
	gui/macosx/library/HeimdallLibraryWindowSidebarChildViewController.h \
	gui/macosx/library/HeimdallLibraryWindowSidebarRootViewController.h \
	gui/macosx/library/HeimdallLibraryWindowSidebarRootViewController.m \
	gui/macosx/library/HeimdallLibraryWindowSplitViewController.h \
	gui/macosx/library/HeimdallLibraryWindowSplitViewController.m \
	gui/macosx/library/HeimdallLibraryWindowTitlesSidebarViewController.h \
	gui/macosx/library/HeimdallLibraryWindowTitlesSidebarViewController.m \
	gui/macosx/library/HeimdallLibraryWindowToolbarDelegate.h \
	gui/macosx/library/HeimdallLibraryWindowToolbarDelegate.m \
	gui/macosx/library/HeimdallMediaLibraryFolderObserver.h \
	gui/macosx/library/HeimdallMediaLibraryFolderObserver.m \
	gui/macosx/library/groups-library/HeimdallLibraryGroupsDataSource.h \
	gui/macosx/library/groups-library/HeimdallLibraryGroupsDataSource.m \
	gui/macosx/library/groups-library/HeimdallLibraryGroupsViewController.h \
	gui/macosx/library/groups-library/HeimdallLibraryGroupsViewController.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewActionButtonCell.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewActionButtonCell.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewActionsView.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewActionsView.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewAudioCarouselContainerView.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewAudioCarouselContainerView.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewBaseCarouselContainerView.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewBaseCarouselContainerView.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewStackViewController.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewStackViewController.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewContainerView.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewController.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewController.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewVideoContainerView.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewVideoCarouselContainerView.h \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewVideoCarouselContainerView.m \
    gui/macosx/library/home-library/HeimdallLibraryHomeViewVideoContainerViewDataSource.h \
    gui/macosx/library/home-library/HeimdallLibraryHomeViewVideoContainerViewDataSource.m \
	gui/macosx/library/home-library/HeimdallLibraryHomeViewVideoGridContainerView.h \
    gui/macosx/library/home-library/HeimdallLibraryHomeViewVideoGridContainerView.m \
	gui/macosx/library/video-library/HeimdallLibraryMoviesDataSource.h \
	gui/macosx/library/video-library/HeimdallLibraryMoviesDataSource.m \
	gui/macosx/library/video-library/HeimdallLibraryShowsDataSource.h \
	gui/macosx/library/video-library/HeimdallLibraryShowsDataSource.m \
	gui/macosx/library/video-library/HeimdallLibraryVideoViewController.h \
	gui/macosx/library/video-library/HeimdallLibraryVideoViewController.m \
	gui/macosx/library/video-library/HeimdallLibraryVideoGroupDescriptor.h \
	gui/macosx/library/video-library/HeimdallLibraryVideoGroupDescriptor.m \
	gui/macosx/library/video-library/HeimdallLibraryVideoDataSource.h \
	gui/macosx/library/video-library/HeimdallLibraryVideoDataSource.m \
	gui/macosx/library/video-library/HeimdallLibraryVideoTableViewDelegate.h \
	gui/macosx/library/video-library/HeimdallLibraryVideoTableViewDelegate.m \
	gui/macosx/library/audio-library/HeimdallLibraryAlbumTableCellView.h \
	gui/macosx/library/audio-library/HeimdallLibraryAlbumTableCellView.m \
	gui/macosx/library/audio-library/HeimdallLibraryAlbumTracksTableViewDelegate.h \
	gui/macosx/library/audio-library/HeimdallLibraryAlbumTracksTableViewDelegate.m \
	gui/macosx/library/audio-library/HeimdallLibraryAllAudioGroupsMediaLibraryItem.h \
	gui/macosx/library/audio-library/HeimdallLibraryAllAudioGroupsMediaLibraryItem.m \
	gui/macosx/library/audio-library/HeimdallLibraryAudioDataSource.h \
	gui/macosx/library/audio-library/HeimdallLibraryAudioDataSource.m \
	gui/macosx/library/audio-library/HeimdallLibraryAudioGroupDataSource.h \
	gui/macosx/library/audio-library/HeimdallLibraryAudioGroupDataSource.m \
	gui/macosx/library/HeimdallLibraryHeaderCell.h \
	gui/macosx/library/HeimdallLibraryHeaderCell.m \
	gui/macosx/library/HeimdallLibraryHeaderView.h \
	gui/macosx/library/HeimdallLibraryHeaderView.m \
	gui/macosx/library/audio-library/HeimdallLibraryAudioGroupTableViewDelegate.h \
	gui/macosx/library/audio-library/HeimdallLibraryAudioGroupTableViewDelegate.m \
	gui/macosx/library/audio-library/HeimdallLibraryAudioTableViewDelegate.h \
	gui/macosx/library/audio-library/HeimdallLibraryAudioTableViewDelegate.m \
	gui/macosx/library/audio-library/HeimdallLibraryAudioViewController.h \
	gui/macosx/library/audio-library/HeimdallLibraryAudioViewController.m \
	gui/macosx/library/audio-library/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.h \
	gui/macosx/library/audio-library/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.m \
	gui/macosx/library/audio-library/HeimdallLibrarySongTableCellView.h \
	gui/macosx/library/audio-library/HeimdallLibrarySongTableCellView.m \
	gui/macosx/library/audio-library/HeimdallLibrarySongsTableViewSongPlayingTableCellView.h \
	gui/macosx/library/audio-library/HeimdallLibrarySongsTableViewSongPlayingTableCellView.m \
	gui/macosx/library/favorites-library/HeimdallLibraryFavoritesDataSource.h \
	gui/macosx/library/favorites-library/HeimdallLibraryFavoritesDataSource.m \
	gui/macosx/library/favorites-library/HeimdallLibraryFavoritesTableViewDelegate.h \
	gui/macosx/library/favorites-library/HeimdallLibraryFavoritesTableViewDelegate.m \
	gui/macosx/library/favorites-library/HeimdallLibraryFavoritesViewController.h \
	gui/macosx/library/favorites-library/HeimdallLibraryFavoritesViewController.m \
	gui/macosx/library/media-source/HeimdallLibraryMediaSourceViewController.h \
	gui/macosx/library/media-source/HeimdallLibraryMediaSourceViewController.m \
	gui/macosx/library/media-source/HeimdallMediaSource.h \
	gui/macosx/library/media-source/HeimdallMediaSource.m \
	gui/macosx/library/media-source/HeimdallLocalMediaSource.h \
	gui/macosx/library/media-source/HeimdallLocalMediaSource.m \
	gui/macosx/library/media-source/HeimdallLocalMediaSourceNodeObservation.h \
	gui/macosx/library/media-source/HeimdallLocalMediaSourceNodeObservation.m \
	gui/macosx/library/media-source/HeimdallMediaSourceNodeObservation.h \
	gui/macosx/library/media-source/HeimdallMediaSourceBaseDataSource.h \
	gui/macosx/library/media-source/HeimdallMediaSourceBaseDataSource.m \
	gui/macosx/library/media-source/HeimdallMediaSourceCollectionViewItem.h \
	gui/macosx/library/media-source/HeimdallMediaSourceCollectionViewItem.m \
	gui/macosx/library/media-source/HeimdallMediaSourceDataSource.h \
	gui/macosx/library/media-source/HeimdallMediaSourceDataSource.m \
	gui/macosx/library/media-source/HeimdallMediaSourceDeviceCollectionViewItem.h \
	gui/macosx/library/media-source/HeimdallMediaSourceDeviceCollectionViewItem.m \
	gui/macosx/library/media-source/HeimdallLibraryMediaSourceViewNavigationStack.h \
	gui/macosx/library/media-source/HeimdallLibraryMediaSourceViewNavigationStack.m \
	gui/macosx/library/media-source/HeimdallLibraryMediaSourceViewNavigationState.h \
	gui/macosx/library/media-source/HeimdallLibraryMediaSourceViewNavigationState.m \
	gui/macosx/library/media-source/HeimdallMediaSourceProvider.h \
	gui/macosx/library/media-source/HeimdallMediaSourceProvider.m \
	gui/macosx/library/playlist-library/HeimdallLibraryPlaylistDataSource.h \
	gui/macosx/library/playlist-library/HeimdallLibraryPlaylistDataSource.m \
	gui/macosx/library/playlist-library/HeimdallLibraryPlaylistViewController.h \
	gui/macosx/library/playlist-library/HeimdallLibraryPlaylistViewController.m \
	gui/macosx/library/HeimdallLibrarySectionedTableViewDelegate.h \
	gui/macosx/library/HeimdallLibrarySectionedTableViewDelegate.m \
	gui/macosx/library/search-library/HeimdallLibrarySearchDataSource.h \
	gui/macosx/library/search-library/HeimdallLibrarySearchDataSource.m \
	gui/macosx/library/search-library/HeimdallLibrarySearchProvider.h \
	gui/macosx/library/search-library/HeimdallLibrarySearchProvider.m \
	gui/macosx/library/search-library/HeimdallLibrarySearchViewController.h \
	gui/macosx/library/search-library/HeimdallLibrarySearchViewController.m \
	gui/macosx/main/CompatibilityFixes.h \
	gui/macosx/main/CompatibilityFixes.m \
	gui/macosx/main/HeimdallApplication.h \
	gui/macosx/main/HeimdallApplication.m \
	gui/macosx/main/HeimdallMain+Sparkle.h \
	gui/macosx/main/HeimdallMain+Sparkle.m \
	gui/macosx/main/HeimdallMain+OldPrefs.h \
	gui/macosx/main/HeimdallMain+OldPrefs.m \
	gui/macosx/main/HeimdallMain.h \
	gui/macosx/main/HeimdallMain.m \
	gui/macosx/main/macosx.m \
	gui/macosx/menus/HeimdallMainMenu.h \
	gui/macosx/menus/HeimdallMainMenu.m \
	gui/macosx/menus/HeimdallRecentStreamsMenuController.h \
	gui/macosx/menus/HeimdallRecentStreamsMenuController.m \
	gui/macosx/menus/HeimdallStatusBarIcon.h \
	gui/macosx/menus/HeimdallStatusBarIcon.m \
	gui/macosx/menus/renderers/HeimdallRendererDiscovery.h \
	gui/macosx/menus/renderers/HeimdallRendererDiscovery.m \
	gui/macosx/menus/renderers/HeimdallRendererItem.h \
	gui/macosx/menus/renderers/HeimdallRendererItem.m \
	gui/macosx/menus/renderers/HeimdallRendererMenuController.h \
	gui/macosx/menus/renderers/HeimdallRendererMenuController.m \
	gui/macosx/os-integration/Spotify.h \
	gui/macosx/os-integration/HeimdallClickerManager.h \
	gui/macosx/os-integration/HeimdallClickerManager.m \
	gui/macosx/os-integration/HeimdallSystemVolume.h \
	gui/macosx/os-integration/HeimdallSystemVolume.m \
	gui/macosx/os-integration/HeimdallDocumentController.h \
	gui/macosx/os-integration/HeimdallDocumentController.m \
	gui/macosx/os-integration/HeimdallKeyboardBacklightControl.h \
	gui/macosx/os-integration/HeimdallKeyboardBacklightControl.m \
	gui/macosx/os-integration/HeimdallRemoteControlService.h \
	gui/macosx/os-integration/HeimdallRemoteControlService.m \
	gui/macosx/os-integration/applescript.h \
	gui/macosx/os-integration/applescript.m \
	gui/macosx/os-integration/iTunes.h \
	gui/macosx/panels/HeimdallAudioEffectsWindowController.h \
	gui/macosx/panels/HeimdallAudioEffectsWindowController.m \
	gui/macosx/panels/HeimdallBookmarksWindowController.h \
	gui/macosx/panels/HeimdallBookmarksWindowController.m \
	gui/macosx/panels/HeimdallInformationWindowController.h \
	gui/macosx/panels/HeimdallInformationWindowController.m \
	gui/macosx/panels/HeimdallTimeSelectionPanelController.h \
	gui/macosx/panels/HeimdallTimeSelectionPanelController.m \
	gui/macosx/panels/HeimdallTrackSynchronizationWindowController.h \
	gui/macosx/panels/HeimdallTrackSynchronizationWindowController.m \
	gui/macosx/panels/HeimdallVideoEffectsWindowController.h \
	gui/macosx/panels/HeimdallVideoEffectsWindowController.m \
	gui/macosx/panels/bookmarks/HeimdallBookmark.h \
	gui/macosx/panels/bookmarks/HeimdallBookmark.m \
	gui/macosx/panels/bookmarks/HeimdallBookmarksTableViewDataSource.h \
	gui/macosx/panels/bookmarks/HeimdallBookmarksTableViewDataSource.m \
	gui/macosx/panels/bookmarks/HeimdallBookmarksTableViewDelegate.h \
	gui/macosx/panels/bookmarks/HeimdallBookmarksTableViewDelegate.m \
	gui/macosx/panels/dialogs/HeimdallCoreDialogProvider.h \
	gui/macosx/panels/dialogs/HeimdallCoreDialogProvider.m \
	gui/macosx/panels/dialogs/HeimdallCustomCropArWindowController.h \
	gui/macosx/panels/dialogs/HeimdallCustomCropArWindowController.m \
	gui/macosx/panels/dialogs/HeimdallPopupPanelController.h \
	gui/macosx/panels/dialogs/HeimdallPopupPanelController.m \
	gui/macosx/panels/dialogs/HeimdallResumeDialogController.h \
	gui/macosx/panels/dialogs/HeimdallResumeDialogController.m \
	gui/macosx/panels/dialogs/HeimdallTextfieldPanelController.h \
	gui/macosx/panels/dialogs/HeimdallTextfieldPanelController.m \
	gui/macosx/playqueue/HeimdallPlaybackContinuityController.h \
	gui/macosx/playqueue/HeimdallPlaybackContinuityController.m \
	gui/macosx/playqueue/HeimdallPlayerChapter.h \
	gui/macosx/playqueue/HeimdallPlayerChapter.m \
	gui/macosx/playqueue/HeimdallPlayerController.h \
	gui/macosx/playqueue/HeimdallPlayerController.m \
	gui/macosx/playqueue/HeimdallPlayerTitle.h \
	gui/macosx/playqueue/HeimdallPlayerTitle.m \
	gui/macosx/playqueue/HeimdallPlayQueueController.h \
	gui/macosx/playqueue/HeimdallPlayQueueController.m \
	gui/macosx/playqueue/HeimdallPlayQueueDataSource.h \
	gui/macosx/playqueue/HeimdallPlayQueueDataSource.m \
	gui/macosx/playqueue/HeimdallPlayQueueItem.h \
	gui/macosx/playqueue/HeimdallPlayQueueItem.m \
	gui/macosx/playqueue/HeimdallPlayQueueMenuController.h \
	gui/macosx/playqueue/HeimdallPlayQueueMenuController.m \
	gui/macosx/playqueue/HeimdallPlayQueueModel.h \
	gui/macosx/playqueue/HeimdallPlayQueueModel.m \
	gui/macosx/playqueue/HeimdallPlayQueueSortingMenuController.h \
	gui/macosx/playqueue/HeimdallPlayQueueSortingMenuController.m \
	gui/macosx/playqueue/HeimdallPlayQueueTableCellView.h \
	gui/macosx/playqueue/HeimdallPlayQueueTableCellView.m \
	gui/macosx/playqueue/HeimdallPlayQueueTableView.h \
	gui/macosx/playqueue/HeimdallPlayQueueTableView.m \
	gui/macosx/preferences/HeimdallHotkeyChangeWindow.h \
	gui/macosx/preferences/HeimdallHotkeyChangeWindow.m \
	gui/macosx/preferences/HeimdallSimplePrefsController.h \
	gui/macosx/preferences/HeimdallSimplePrefsController.m \
	gui/macosx/preferences/HeimdallSimplePrefsWindow.h \
	gui/macosx/preferences/HeimdallSimplePrefsWindow.m \
	gui/macosx/preferences/prefs.h \
	gui/macosx/preferences/prefs.m \
	gui/macosx/preferences/prefs_widgets.h \
	gui/macosx/preferences/prefs_widgets.m \
	gui/macosx/shaders/HeimdallShaderTypes.h \
	gui/macosx/views/iCarousel/iCarousel.h \
	gui/macosx/views/iCarousel/iCarousel.m \
	gui/macosx/views/HeimdallBasicView.h \
	gui/macosx/views/HeimdallBasicView.m \
	gui/macosx/views/HeimdallBottomBarView.h \
	gui/macosx/views/HeimdallBottomBarView.m \
	gui/macosx/views/HeimdallDefaultValueSlider.h \
	gui/macosx/views/HeimdallDefaultValueSlider.m \
	gui/macosx/views/HeimdallDefaultValueSliderCell.h \
	gui/macosx/views/HeimdallDefaultValueSliderCell.m \
	gui/macosx/views/HeimdallDragDropView.h \
	gui/macosx/views/HeimdallDragDropView.m \
	gui/macosx/views/HeimdallFileDragRecognisingView.h \
	gui/macosx/views/HeimdallFileDragRecognisingView.m \
	gui/macosx/views/HeimdallImageButton.h \
	gui/macosx/views/HeimdallImageButton.m \
	gui/macosx/views/HeimdallImageView.h \
	gui/macosx/views/HeimdallImageView.m \
	gui/macosx/views/HeimdallLibraryRatingIndicator.h \
	gui/macosx/views/HeimdallLibraryRatingIndicator.m \
	gui/macosx/views/HeimdallLinearProgressIndicator.h \
	gui/macosx/views/HeimdallLinearProgressIndicator.m \
	gui/macosx/views/HeimdallLoadingOverlayView.h \
	gui/macosx/views/HeimdallLoadingOverlayView.m \
	gui/macosx/views/HeimdallMediaItemCollectionViewItem.h \
	gui/macosx/views/HeimdallMediaItemCollectionViewItem.m \
	gui/macosx/views/HeimdallNoResultsLabel.h \
	gui/macosx/views/HeimdallNoResultsLabel.m \
	gui/macosx/views/HeimdallPlaybackEndViewController.h \
	gui/macosx/views/HeimdallPlaybackEndViewController.m \
	gui/macosx/views/HeimdallPlaybackProgressSlider.h \
	gui/macosx/views/HeimdallPlaybackProgressSlider.m \
	gui/macosx/views/HeimdallPlaybackProgressSliderCell.h \
	gui/macosx/views/HeimdallPlaybackProgressSliderCell.m \
	gui/macosx/views/HeimdallRoundedCornerTextField.h \
	gui/macosx/views/HeimdallRoundedCornerTextField.m \
	gui/macosx/views/HeimdallScrollingClipView.h \
	gui/macosx/views/HeimdallScrollingClipView.m \
	gui/macosx/views/HeimdallSettingTextField.h \
	gui/macosx/views/HeimdallSettingTextField.m \
	gui/macosx/views/HeimdallSpacingConstraint.h \
	gui/macosx/views/HeimdallSpacingConstraint.m \
	gui/macosx/views/HeimdallStackView.h \
	gui/macosx/views/HeimdallStackView.m \
	gui/macosx/views/HeimdallStatusNotifierView.h \
	gui/macosx/views/HeimdallStatusNotifierView.m \
	gui/macosx/views/HeimdallSubScrollView.h \
	gui/macosx/views/HeimdallSubScrollView.m \
	gui/macosx/views/HeimdallTimeField.h \
	gui/macosx/views/HeimdallTimeField.m \
	gui/macosx/views/HeimdallTimeFormatter.h \
	gui/macosx/views/HeimdallTimeFormatter.m \
	gui/macosx/views/HeimdallTrackingView.h \
	gui/macosx/views/HeimdallTrackingView.m \
	gui/macosx/views/HeimdallUIUnits.h \
	gui/macosx/views/HeimdallUIUnits.m \
	gui/macosx/views/HeimdallVolumeSlider.h \
	gui/macosx/views/HeimdallVolumeSlider.m \
	gui/macosx/views/HeimdallVolumeSliderCell.h \
	gui/macosx/views/HeimdallVolumeSliderCell.m \
	gui/macosx/windows/HeimdallAboutWindowController.h \
	gui/macosx/windows/HeimdallAboutWindowController.m \
	gui/macosx/windows/HeimdallConnectToServerDialog.h \
	gui/macosx/windows/HeimdallConnectToServerDialog.m \
	gui/macosx/windows/HeimdallDetachedAudioWindow.h \
	gui/macosx/windows/HeimdallDetachedAudioWindow.m \
	gui/macosx/windows/HeimdallErrorWindowController.h \
	gui/macosx/windows/HeimdallErrorWindowController.m \
	gui/macosx/windows/HeimdallHelpWindowController.h \
	gui/macosx/windows/HeimdallHelpWindowController.m \
	gui/macosx/windows/HeimdallOpenInputMetadata.h \
	gui/macosx/windows/HeimdallOpenInputMetadata.m \
	gui/macosx/windows/HeimdallOpenWindowController.h \
	gui/macosx/windows/HeimdallOpenWindowController.m \
	gui/macosx/windows/addons/HeimdallAddonListItem.h \
	gui/macosx/windows/addons/HeimdallAddonListItem.m \
	gui/macosx/windows/addons/HeimdallAddonsWindowController.h \
	gui/macosx/windows/addons/HeimdallAddonsWindowController.m \
	gui/macosx/windows/convertandsave/HeimdallConvertAndSaveWindowController.h \
	gui/macosx/windows/convertandsave/HeimdallConvertAndSaveWindowController.m \
	gui/macosx/windows/convertandsave/HeimdallOutput.h \
	gui/macosx/windows/convertandsave/HeimdallOutput.m \
	gui/macosx/windows/extensions/HeimdallExtensionsDialogProvider.h \
	gui/macosx/windows/extensions/HeimdallExtensionsDialogProvider.m \
	gui/macosx/windows/extensions/HeimdallExtensionsManager.h \
	gui/macosx/windows/extensions/HeimdallExtensionsManager.m \
	gui/macosx/windows/extensions/HeimdallUIWidgets.h \
	gui/macosx/windows/extensions/HeimdallUIWidgets.m \
	gui/macosx/windows/logging/HeimdallLogMessage.h \
	gui/macosx/windows/logging/HeimdallLogMessage.m \
	gui/macosx/windows/logging/HeimdallLogWindowController.h \
	gui/macosx/windows/logging/HeimdallLogWindowController.m \
	gui/macosx/windows/controlsbar/HeimdallControlsBarCommon.h \
	gui/macosx/windows/controlsbar/HeimdallControlsBarCommon.m \
	gui/macosx/windows/controlsbar/HeimdallMainVideoViewControlsBar.h \
	gui/macosx/windows/controlsbar/HeimdallMainVideoViewControlsBar.m \
	gui/macosx/windows/controlsbar/HeimdallMainWindowControlsBar.h \
	gui/macosx/windows/controlsbar/HeimdallMainWindowControlsBar.m \
	gui/macosx/windows/video/HeimdallAspectRatioRetainingVideoWindow.h \
	gui/macosx/windows/video/HeimdallAspectRatioRetainingVideoWindow.m \
	gui/macosx/windows/video/HeimdallFullVideoViewWindow.h \
	gui/macosx/windows/video/HeimdallFullVideoViewWindow.m \
	gui/macosx/windows/video/HeimdallMainVideoViewAudioMediaDecorativeView.h \
	gui/macosx/windows/video/HeimdallMainVideoViewAudioMediaDecorativeView.m \
	gui/macosx/windows/video/HeimdallMainVideoViewController.h \
	gui/macosx/windows/video/HeimdallMainVideoViewController.m \
	gui/macosx/windows/video/HeimdallMainVideoViewOverlayView.h \
	gui/macosx/windows/video/HeimdallMainVideoViewOverlayView.m \
	gui/macosx/windows/video/HeimdallVideoOutputProvider.h \
	gui/macosx/windows/video/HeimdallVideoOutputProvider.m \
	gui/macosx/windows/video/HeimdallVideoWindowCommon.h \
	gui/macosx/windows/video/HeimdallVideoWindowCommon.m \
	gui/macosx/windows/video/HeimdallVoutView.h \
	gui/macosx/windows/video/HeimdallVoutView.m \
	gui/macosx/windows/video/HeimdallWindow.h \
	gui/macosx/windows/video/HeimdallWindow.m

if ENABLE_MACOSX_UI_SHADERS
libmacosx_plugin_la_SOURCES += \
	gui/macosx/views/HeimdallSnowEffectView.h \
	gui/macosx/views/HeimdallSnowEffectView.m
endif

# User interface compilation

libmacosx_plugin_la_XIB_SOURCES = \
	gui/macosx/UI/About.xib \
	gui/macosx/UI/AddonManager.xib \
	gui/macosx/UI/AudioEffects.xib \
	gui/macosx/UI/Bookmarks.xib \
	gui/macosx/UI/ConvertAndSave.xib \
	gui/macosx/UI/CoreDialogs.xib \
	gui/macosx/UI/ErrorPanel.xib \
	gui/macosx/UI/Help.xib \
	gui/macosx/UI/LogMessageWindow.xib \
	gui/macosx/UI/MainMenu.xib \
	gui/macosx/UI/HeimdallCustomCropARPanel.xib \
	gui/macosx/UI/HeimdallDetachedAudioWindow.xib \
	gui/macosx/UI/HeimdallLibraryAlbumTableCellView.xib \
	gui/macosx/UI/HeimdallLibraryWindow.xib \
	gui/macosx/UI/HeimdallLibraryWindowChaptersView.xib \
	gui/macosx/UI/HeimdallLibraryWindowNavigationSidebarView.xib \
	gui/macosx/UI/HeimdallLibraryWindowPlayQueueView.xib \
	gui/macosx/UI/HeimdallLibraryWindowSidebarRootView.xib \
	gui/macosx/UI/HeimdallLibraryWindowTitlesView.xib \
	gui/macosx/UI/HeimdallLibraryTableCellView.xib \
	gui/macosx/UI/HeimdallLibraryVideoTableCellView.xib \
	gui/macosx/UI/HeimdallPlayQueueTableCellView.xib \
	gui/macosx/UI/HeimdallLibraryCarouselViewItemView.xib \
	gui/macosx/UI/HeimdallLibraryCollectionViewMediaItemListSupplementaryDetailView.xib \
	gui/macosx/UI/HeimdallLibraryCollectionViewAudioGroupSupplementaryDetailView.xib \
	gui/macosx/UI/HeimdallLibraryCollectionViewMediaItemSupplementaryDetailView.xib \
	gui/macosx/UI/HeimdallLibraryHeroView.xib \
	gui/macosx/UI/HeimdallLibraryHomeViewActionsView.xib \
	gui/macosx/UI/HeimdallLibrarySongTableCellView.xib \
	gui/macosx/UI/HeimdallClassicMainVideoView.xib \
	gui/macosx/UI/HeimdallMainVideoView.xib \
	gui/macosx/UI/HeimdallMainVideoViewAudioMediaDecorativeView.xib \
	gui/macosx/UI/HeimdallMediaItemCollectionViewItem.xib \
	gui/macosx/UI/HeimdallMediaSourceDeviceCollectionViewItem.xib \
	gui/macosx/UI/HeimdallInformationWindow.xib \
	gui/macosx/UI/HeimdallPlaybackEndView.xib \
	gui/macosx/UI/Open.xib \
	gui/macosx/UI/PlaylistAccessoryView.xib \
	gui/macosx/UI/PopupPanel.xib \
	gui/macosx/UI/Preferences.xib \
	gui/macosx/UI/ResumeDialog.xib \
	gui/macosx/UI/SimplePreferences.xib \
	gui/macosx/UI/StreamOutput.xib \
	gui/macosx/UI/SyncTracks.xib \
	gui/macosx/UI/TextfieldPanel.xib \
	gui/macosx/UI/TimeSelectionPanel.xib \
	gui/macosx/UI/VideoEffects.xib \
	gui/macosx/UI/HeimdallFullScreenPanel.xib \
	gui/macosx/UI/HeimdallStatusBarIconMainMenu.xib

libmacosx_plugin_la_SOURCES += $(libmacosx_plugin_la_XIB_SOURCES)

if ENABLE_MACOSX_UI
gui_PLUGINS += libmacosx_plugin.la
BUILT_SOURCES += $(libmacosx_plugin_la_XIB_SOURCES:.xib=.nib)
endif

libmacosx_plugin_la_RES = \
	gui/macosx/Resources/App-Icons/Heimdall-Xmas.icns \
	gui/macosx/Resources/App-Icons/Heimdall.icns \
	gui/macosx/Resources/File-Icons/aiff.icns \
	gui/macosx/Resources/File-Icons/audio.icns \
	gui/macosx/Resources/File-Icons/avi.icns \
	gui/macosx/Resources/File-Icons/flv.icns \
	gui/macosx/Resources/File-Icons/generic.icns \
	gui/macosx/Resources/File-Icons/m4a.icns \
	gui/macosx/Resources/File-Icons/m4v.icns \
	gui/macosx/Resources/File-Icons/mkv.icns \
	gui/macosx/Resources/File-Icons/mov.icns \
	gui/macosx/Resources/File-Icons/movie.icns \
	gui/macosx/Resources/File-Icons/mp3.icns \
	gui/macosx/Resources/File-Icons/mpeg.icns \
	gui/macosx/Resources/File-Icons/ogg.icns \
	gui/macosx/Resources/File-Icons/playlist.icns \
	gui/macosx/Resources/File-Icons/rm.icns \
	gui/macosx/Resources/File-Icons/subtitle.icns \
	gui/macosx/Resources/File-Icons/vob.icns \
	gui/macosx/Resources/File-Icons/wav.icns \
	gui/macosx/Resources/File-Icons/wma.icns \
	gui/macosx/Resources/File-Icons/wmv.icns \
	gui/macosx/Resources/browsewindow/bw-Music.png \
	gui/macosx/Resources/browsewindow/bw-Music@2x.png \
	gui/macosx/Resources/browsewindow/bw-Music@3x.png \
	gui/macosx/Resources/browsewindow/bw-Podcast.png \
	gui/macosx/Resources/browsewindow/bw-Podcast@1,5x.svg \
	gui/macosx/Resources/browsewindow/bw-Podcast@2x.png \
	gui/macosx/Resources/browsewindow/bw-Podcast@3x.png \
	gui/macosx/Resources/browsewindow/bw-Server1.png \
	gui/macosx/Resources/browsewindow/bw-Server1@2x.png \
	gui/macosx/Resources/browsewindow/bw-Server1@3x.png \
	gui/macosx/Resources/browsewindow/bw-blu-ray.png \
	gui/macosx/Resources/browsewindow/bw-blu-ray@2x.png \
	gui/macosx/Resources/browsewindow/bw-blu-ray@3x.png \
	gui/macosx/Resources/browsewindow/bw-home.png \
	gui/macosx/Resources/browsewindow/bw-home@2x.png \
	gui/macosx/Resources/browsewindow/bw-home@3x.png \
	gui/macosx/Resources/browsewindow/bw-phone.png \
	gui/macosx/Resources/browsewindow/bw-phone@2x.png \
	gui/macosx/Resources/browsewindow/bw-phone@3x.png \
	gui/macosx/Resources/browsewindow/bw-server2.png \
	gui/macosx/Resources/browsewindow/bw-server2@2x.png \
	gui/macosx/Resources/browsewindow/bw-server2@3x.png \
	gui/macosx/Resources/browsewindow/bw-usb1.png \
	gui/macosx/Resources/browsewindow/bw-usb1@2x.png \
	gui/macosx/Resources/browsewindow/bw-usb1@3x.png \
	gui/macosx/Resources/browsewindow/bw-usb2.png \
	gui/macosx/Resources/browsewindow/bw-usb2@2x.png \
	gui/macosx/Resources/browsewindow/bw-usb2@3x.png \
	gui/macosx/Resources/browsewindow/bw-video.png \
	gui/macosx/Resources/browsewindow/bw-video@2x.png \
	gui/macosx/Resources/browsewindow/bw-video@3x.png \
	gui/macosx/Resources/librarywindow/ellipsis.png \
	gui/macosx/Resources/librarywindow/ellipsis@2x.png \
	gui/macosx/Resources/librarywindow/note.png \
	gui/macosx/Resources/librarywindow/note@2x.png \
	gui/macosx/Resources/librarywindow/libraryPlay.png \
	gui/macosx/Resources/librarywindow/libraryPlay@2x.png \
	gui/macosx/Resources/librarywindow/repeatAll.png \
	gui/macosx/Resources/librarywindow/repeatAll@2x.png \
	gui/macosx/Resources/librarywindow/repeatOff.png \
	gui/macosx/Resources/librarywindow/repeatOff@2x.png \
	gui/macosx/Resources/librarywindow/repeatOne.png \
	gui/macosx/Resources/librarywindow/repeatOne@2x.png \
	gui/macosx/Resources/librarywindow/shuffleOff.png \
	gui/macosx/Resources/librarywindow/shuffleOff@2x.png \
	gui/macosx/Resources/librarywindow/shuffleOn.png \
	gui/macosx/Resources/librarywindow/shuffleOn@2x.png \
	gui/macosx/Resources/mainwindow/album.png \
	gui/macosx/Resources/mainwindow/album@2x.png \
	gui/macosx/Resources/mainwindow/artist.png \
	gui/macosx/Resources/mainwindow/artist@2x.png \
	gui/macosx/Resources/mainwindow/backward-3btns-pressed.png \
	gui/macosx/Resources/mainwindow/backward-3btns-pressed@2x.png \
	gui/macosx/Resources/mainwindow/backward-3btns.png \
	gui/macosx/Resources/mainwindow/backward-3btns@2x.png \
	gui/macosx/Resources/mainwindow/backward-6btns-pressed.png \
	gui/macosx/Resources/mainwindow/backward-6btns-pressed@2x.png \
	gui/macosx/Resources/mainwindow/backward-6btns.png \
	gui/macosx/Resources/mainwindow/backward-6btns@2x.png \
	gui/macosx/Resources/mainwindow/effects-double-buttons-pressed.png \
	gui/macosx/Resources/mainwindow/effects-double-buttons-pressed@2x.png \
	gui/macosx/Resources/mainwindow/effects-double-buttons.png \
	gui/macosx/Resources/mainwindow/effects-double-buttons@2x.png \
	gui/macosx/Resources/mainwindow/effects-one-button-pressed.png \
	gui/macosx/Resources/mainwindow/effects-one-button-pressed@2x.png \
	gui/macosx/Resources/mainwindow/effects-one-button.png \
	gui/macosx/Resources/mainwindow/effects-one-button@2x.png \
	gui/macosx/Resources/mainwindow/forward-3btns-pressed.png \
	gui/macosx/Resources/mainwindow/forward-3btns-pressed@2x.png \
	gui/macosx/Resources/mainwindow/forward-3btns.png \
	gui/macosx/Resources/mainwindow/forward-3btns@2x.png \
	gui/macosx/Resources/mainwindow/forward-6btns-pressed.png \
	gui/macosx/Resources/mainwindow/forward-6btns-pressed@2x.png \
	gui/macosx/Resources/mainwindow/forward-6btns.png \
	gui/macosx/Resources/mainwindow/forward-6btns@2x.png \
	gui/macosx/Resources/mainwindow/fullscreen-double-buttons-pressed.png \
	gui/macosx/Resources/mainwindow/fullscreen-double-buttons-pressed@2x.png \
	gui/macosx/Resources/mainwindow/fullscreen-double-buttons.png \
	gui/macosx/Resources/mainwindow/fullscreen-double-buttons@2x.png \
	gui/macosx/Resources/mainwindow/fullscreen-one-button-pressed.png \
	gui/macosx/Resources/mainwindow/fullscreen-one-button-pressed@2x.png \
	gui/macosx/Resources/mainwindow/fullscreen-one-button.png \
	gui/macosx/Resources/mainwindow/fullscreen-one-button@2x.png \
	gui/macosx/Resources/mainwindow/next-6btns-pressed.png \
	gui/macosx/Resources/mainwindow/next-6btns-pressed@2x.png \
	gui/macosx/Resources/mainwindow/next-6btns.png \
	gui/macosx/Resources/mainwindow/next-6btns@2x.png \
	gui/macosx/Resources/mainwindow/pause-pressed.png \
	gui/macosx/Resources/mainwindow/pause-pressed@2x.png \
	gui/macosx/Resources/mainwindow/pause.png \
	gui/macosx/Resources/mainwindow/pause@2x.png \
	gui/macosx/Resources/mainwindow/placeholder-group2.pdf \
	gui/macosx/Resources/mainwindow/placeholder-group2.png \
	gui/macosx/Resources/mainwindow/placeholder-group2.svg \
	gui/macosx/Resources/mainwindow/placeholder-group2@2x.png \
	gui/macosx/Resources/mainwindow/placeholder-music.pdf \
	gui/macosx/Resources/mainwindow/placeholder-music.png \
	gui/macosx/Resources/mainwindow/placeholder-music.svg \
	gui/macosx/Resources/mainwindow/placeholder-music@2x.png \
	gui/macosx/Resources/mainwindow/placeholder-video.pdf \
	gui/macosx/Resources/mainwindow/placeholder-video.png \
	gui/macosx/Resources/mainwindow/placeholder-video.svg \
	gui/macosx/Resources/mainwindow/placeholder-video@2x.png \
	gui/macosx/Resources/mainwindow/play-pressed.png \
	gui/macosx/Resources/mainwindow/play-pressed@2x.png \
	gui/macosx/Resources/mainwindow/play.png \
	gui/macosx/Resources/mainwindow/play@2x.png \
	gui/macosx/Resources/mainwindow/playlist-1btn-pressed.png \
	gui/macosx/Resources/mainwindow/playlist-1btn-pressed@2x.png \
	gui/macosx/Resources/mainwindow/playlist-1btn.png \
	gui/macosx/Resources/mainwindow/playlist-1btn@2x.png \
	gui/macosx/Resources/mainwindow/playlist-btn-pressed.png \
	gui/macosx/Resources/mainwindow/playlist-btn-pressed@2x.png \
	gui/macosx/Resources/mainwindow/playlist-btn.png \
	gui/macosx/Resources/mainwindow/playlist-btn@2x.png \
	gui/macosx/Resources/mainwindow/previous-6btns-pressed.png \
	gui/macosx/Resources/mainwindow/previous-6btns-pressed@2x.png \
	gui/macosx/Resources/mainwindow/previous-6btns.png \
	gui/macosx/Resources/mainwindow/previous-6btns@2x.png \
	gui/macosx/Resources/mainwindow/repeat-all-pressed.png \
	gui/macosx/Resources/mainwindow/repeat-all-pressed@2x.png \
	gui/macosx/Resources/mainwindow/repeat-all.png \
	gui/macosx/Resources/mainwindow/repeat-all@2x.png \
	gui/macosx/Resources/mainwindow/repeat-one-pressed.png \
	gui/macosx/Resources/mainwindow/repeat-one-pressed@2x.png \
	gui/macosx/Resources/mainwindow/repeat-one.png \
	gui/macosx/Resources/mainwindow/repeat-one@2x.png \
	gui/macosx/Resources/mainwindow/repeat-pressed.png \
	gui/macosx/Resources/mainwindow/repeat-pressed@2x.png \
	gui/macosx/Resources/mainwindow/repeat.png \
	gui/macosx/Resources/mainwindow/repeat@2x.png \
	gui/macosx/Resources/mainwindow/shuffle-blue-pressed.png \
	gui/macosx/Resources/mainwindow/shuffle-blue-pressed@2x.png \
	gui/macosx/Resources/mainwindow/shuffle-blue.png \
	gui/macosx/Resources/mainwindow/shuffle-blue@2x.png \
	gui/macosx/Resources/mainwindow/shuffle-pressed.png \
	gui/macosx/Resources/mainwindow/shuffle-pressed@2x.png \
	gui/macosx/Resources/mainwindow/shuffle.png \
	gui/macosx/Resources/mainwindow/shuffle@2x.png \
	gui/macosx/Resources/mainwindow/stop-pressed.png \
	gui/macosx/Resources/mainwindow/stop-pressed@2x.png \
	gui/macosx/Resources/mainwindow/stop.png \
	gui/macosx/Resources/mainwindow/stop@2x.png \
	gui/macosx/Resources/mainwindow/topbar_background.png \
	gui/macosx/Resources/mainwindow/video.png \
	gui/macosx/Resources/mainwindow/video@2x.png \
	gui/macosx/Resources/mainwindow/volume-high.png \
	gui/macosx/Resources/mainwindow/volume-high@2x.png \
	gui/macosx/Resources/mainwindow/volume-low.png \
	gui/macosx/Resources/mainwindow/volume-low@2x.png \
	gui/macosx/Resources/noart.png \
	gui/macosx/Resources/Pref-Icons/HeimdallAudioCone.png \
	gui/macosx/Resources/Pref-Icons/HeimdallHotkeysCone.png \
	gui/macosx/Resources/Pref-Icons/HeimdallInputCone.png \
	gui/macosx/Resources/Pref-Icons/HeimdallInterfaceCone.png \
	gui/macosx/Resources/Pref-Icons/HeimdallSubtitleCone.png \
	gui/macosx/Resources/Pref-Icons/HeimdallVideoCone.png \
	gui/macosx/Resources/sidebar-icons/sidebar-movie.png \
	gui/macosx/Resources/sidebar-icons/sidebar-movie@2x.png \
	gui/macosx/Resources/sidebar-icons/sidebar-music.png \
	gui/macosx/Resources/sidebar-icons/sidebar-music@2x.png \
	gui/macosx/Resources/Button-Icons/HeimdallNextTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallPauseTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallPlayTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallPreviousTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallShuffleTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallStatusBarIcon.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallBackwardTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallForwardTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallFullscreenOnTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallFullscreenOffTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallVolumeOnTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallVolumeOffTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallClearLogTemplate.pdf \
	gui/macosx/Resources/Button-Icons/HeimdallSaveTemplate.pdf \
	gui/macosx/Resources/Button-Icons/window-close-graphite.png \
	gui/macosx/Resources/Button-Icons/window-close-graphite@2x.png \
	gui/macosx/Resources/Button-Icons/window-close-on-graphite.png \
	gui/macosx/Resources/Button-Icons/window-close-on-graphite@2x.png \
	gui/macosx/Resources/Button-Icons/window-close-on.png \
	gui/macosx/Resources/Button-Icons/window-close-on@2x.png \
	gui/macosx/Resources/Button-Icons/window-close-over-graphite.png \
	gui/macosx/Resources/Button-Icons/window-close-over-graphite@2x.png \
	gui/macosx/Resources/Button-Icons/window-close-over.png \
	gui/macosx/Resources/Button-Icons/window-close-over@2x.png \
	gui/macosx/Resources/Button-Icons/window-close.png \
	gui/macosx/Resources/Button-Icons/window-close@2x.png

libmacosx_plugin_la_RES += \
	gui/macosx/Resources/dsa_pub.pem \
	gui/macosx/Resources/README \
	gui/macosx/Resources/heimdall.scriptSuite \
	gui/macosx/Resources/heimdall.scriptTerminology

EXTRA_DIST += \
	gui/macosx/Resources/Base.lproj/InfoPlist.strings \
	gui/macosx/Resources/ar.lproj/InfoPlist.strings \
	gui/macosx/Resources/bg.lproj/InfoPlist.strings \
	gui/macosx/Resources/ca.lproj/InfoPlist.strings \
	gui/macosx/Resources/cs.lproj/InfoPlist.strings \
	gui/macosx/Resources/de.lproj/InfoPlist.strings \
	gui/macosx/Resources/el.lproj/InfoPlist.strings \
	gui/macosx/Resources/es.lproj/InfoPlist.strings \
	gui/macosx/Resources/fr.lproj/InfoPlist.strings \
	gui/macosx/Resources/gl.lproj/InfoPlist.strings \
	gui/macosx/Resources/he.lproj/InfoPlist.strings \
	gui/macosx/Resources/hu.lproj/InfoPlist.strings \
	gui/macosx/Resources/it.lproj/InfoPlist.strings \
	gui/macosx/Resources/ja.lproj/InfoPlist.strings \
	gui/macosx/Resources/lv.lproj/InfoPlist.strings \
	gui/macosx/Resources/nl.lproj/InfoPlist.strings \
	gui/macosx/Resources/pl.lproj/InfoPlist.strings \
	gui/macosx/Resources/pt-BR.lproj/InfoPlist.strings \
	gui/macosx/Resources/pt-PT.lproj/InfoPlist.strings \
	gui/macosx/Resources/sk.lproj/InfoPlist.strings \
	gui/macosx/Resources/sl.lproj/InfoPlist.strings \
	gui/macosx/Resources/sv.lproj/InfoPlist.strings \
	gui/macosx/Resources/tr.lproj/InfoPlist.strings \
	gui/macosx/Resources/uk.lproj/InfoPlist.strings \
	gui/macosx/Resources/zh-Hans.lproj/InfoPlist.strings

if ENABLE_MACOSX_UI
dist_libmacosx_plugin_la_DATA = $(libmacosx_plugin_la_RES)
endif

libmacosx_plugin_ladir = $(prefix)/share/macosx/Contents/Resources

include gui/macosx/tests/Makefile.am
