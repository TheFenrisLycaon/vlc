if ENABLE_MACOSX_UI

if ENABLE_MACOSX_XCTESTS

macosx_gui_xctest_frameworks_dir = $(shell $(XCRUN) --sdk macosx --show-sdk-platform-path)/Developer/Library/Frameworks
macosx_gui_xctest_bundles = HeimdallMacOSXTests.xctest

check_PROGRAMS += HeimdallMacOSXTests
TESTS += $(macosx_gui_xctest_bundles)

HeimdallMacOSXTests_SOURCES = \
	gui/macosx/tests/HeimdallHexNumberFormatterTest.m \
	gui/macosx/tests/NSStringHelpersTest.m \
	gui/macosx/tests/HeimdallLibraryDataTypesTest.m \
	gui/macosx/tests/HeimdallLibraryDataTypesTestSupport.m \
	gui/macosx/tests/HeimdallInputItemTest.m \
	gui/macosx/tests/HeimdallInputItemTestSupport.m \
	gui/macosx/tests/HeimdallInputNodeTest.m \
	gui/macosx/tests/HeimdallLibraryDataTypesIntegrationTest.m \
	gui/macosx/tests/HeimdallLibraryDataTypesIntegrationTestSupport.m \
	gui/macosx/tests/HeimdallLogMessageTest.m \
	gui/macosx/tests/HeimdallPositionFormatterTest.m \
	gui/macosx/tests/HeimdallTimeFormatterTest.m \
	gui/macosx/extensions/HeimdallHexNumberFormatter.m \
	gui/macosx/extensions/HeimdallPositionFormatter.m \
	gui/macosx/extensions/NSString+Helpers.m \
	gui/macosx/library/HeimdallLibraryDataTypes.m \
	gui/macosx/library/HeimdallInputItem.m \
	gui/macosx/library/HeimdallInputNode.m \
	gui/macosx/windows/logging/HeimdallLogMessage.m \
	gui/macosx/views/HeimdallTimeFormatter.m

noinst_HEADERS += \
	gui/macosx/tests/HeimdallInputItemTestSupport.h \
	gui/macosx/tests/HeimdallLibraryDataTypesTestSupport.h \
	gui/macosx/tests/HeimdallLibraryDataTypesIntegrationTestSupport.h

HeimdallMacOSXTests_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DHEIMDALL_MACOSX_TEST_SOURCE_DIR=\"$(abs_top_srcdir)\" \
	-F"$(macosx_gui_xctest_frameworks_dir)" \
	-I$(srcdir)/gui/macosx \
	-I$(top_srcdir)

HeimdallMacOSXTests_OBJCFLAGS = \
	$(AM_OBJCFLAGS) \
	-fblocks \
	-fobjc-arc \
	-fobjc-exceptions \
	-fmodules

HeimdallMacOSXTests_LDFLAGS = \
	-bundle \
	-F"$(macosx_gui_xctest_frameworks_dir)" \
	-Wl,-rpath,"$(macosx_gui_xctest_frameworks_dir)" \
	-Wl,-rpath,"$(abs_top_builddir)/src/.libs" \
	-Wl,-rpath,"$(abs_top_builddir)/lib/.libs" \
	-Wl,-framework,XCTest \
	-Wl,-framework,Foundation \
	-Wl,-framework,Cocoa \
	-Wl,-framework,IOKit

HeimdallMacOSXTests_LDADD = \
	$(top_builddir)/lib/libheimdall.la \
	$(top_builddir)/src/libheimdallcore.la \
	-lsqlite3

HeimdallMacOSXTests_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
	$(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(OBJCLD) \
	$(HeimdallMacOSXTests_OBJCFLAGS) $(OBJCFLAGS) \
	$(HeimdallMacOSXTests_LDFLAGS) $(LDFLAGS) -o $@

HeimdallMacOSXTests.xctest: HeimdallMacOSXTests$(EXEEXT) $(srcdir)/gui/macosx/tests/HeimdallMacOSXTests-Info.plist
	$(AM_V_GEN)rm -rf $@
	$(AM_V_at)$(MKDIR_P) $@/Contents/MacOS
	$(AM_V_at)$(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=install \
		$(INSTALL_PROGRAM) HeimdallMacOSXTests$(EXEEXT) \
		$(abs_builddir)/$@/Contents/MacOS/HeimdallMacOSXTests
	$(AM_V_at)$(INSTALL_DATA) $(srcdir)/gui/macosx/tests/HeimdallMacOSXTests-Info.plist $@/Contents/Info.plist

EXTRA_DIST += gui/macosx/tests/HeimdallMacOSXTests-Info.plist

mostlyclean-local: mostlyclean-local-macosx-xctest
mostlyclean-local-macosx-xctest:
	rm -rf $(macosx_gui_xctest_bundles)

check-macosx: HeimdallMacOSXTests.xctest
	$(XCTEST_LOG_COMPILER) $<

else

check-macosx:
	@echo "XCTest-based macOS GUI tests require a native macOS build; skipping."

endif

.PHONY: check-macosx mostlyclean-local-macosx-xctest

endif
