NULL =

LIBHEIMDALL_SAMPLES = \
	libheimdall/gtk_player.c \
	libheimdall/QtPlayer/LICENSE \
	libheimdall/QtPlayer/main.cpp \
	libheimdall/QtPlayer/player.cpp \
	libheimdall/QtPlayer/player.h \
	libheimdall/QtPlayer/QtHeimdall.pro \
	libheimdall/QtGL/main.cpp \
	libheimdall/QtGL/qtheimdallwidget.cpp \
	libheimdall/QtGL/qtheimdallwidget.h \
	libheimdall/QtGL/QtGl.pro \
	libheimdall/thumbnailer.c \
	libheimdall/wx_player.cpp \
	libheimdall/d3d11_player.cpp \
	libheimdall/d3d9_player.c \
	libheimdall/win_player.c \
	libheimdall/CMakeLists.txt \
	libheimdall/appkit_player.m \
	libheimdall/downloader.c \
	libheimdall/parser.c \
	libheimdall/media_discoverer.c \
	libheimdall/renderer_discoverer.c \
	libheimdall/player.c \
	$(NULL)

# Check that samples that depends only on LibHeimdall can build
AUTOMAKE_OPTIONS = subdir-objects
AM_LDFLAGS = -no-install $(LDFLAGS_heimdall) -L../src/ -lheimdallcore -L../lib -lheimdall

samples_libheimdall_example_SOURCES = libheimdall/example.c
samples_libheimdall_thumbnailer_SOURCES = libheimdall/thumbnailer.c
samples_libheimdall_downloader_SOURCES = libheimdall/downloader.c
samples_libheimdall_parser_SOURCES = libheimdall/parser.c
samples_libheimdall_media_discoverer_SOURCES = libheimdall/media_discoverer.c
samples_libheimdall_renderer_discoverer_SOURCES = libheimdall/renderer_discoverer.c
samples_libheimdall_player_SOURCES = libheimdall/player.c

check_PROGRAMS = \
	samples_libheimdall_example \
	samples_libheimdall_thumbnailer \
	samples_libheimdall_downloader \
	samples_libheimdall_parser \
	samples_libheimdall_media_discoverer \
	samples_libheimdall_renderer_discoverer \
	samples_libheimdall_player \
	$(NULL)

nobase_doc_DATA = $(LIBHEIMDALL_SAMPLES)

doc_DATA = \
	fortunes.txt \
	$(NULL)

CHANGELOGS = \
	ChangeLog-1999 \
	ChangeLog-2000 \
	ChangeLog-2001 \
	ChangeLog-2002 \
	ChangeLog-2003 \
	ChangeLog-2004 \
	ChangeLog-2005 \
	ChangeLog-2006 \
	ChangeLog-2007 \
	ChangeLog-2008 \
	ChangeLog-2009 \
	ChangeLog-2010 \
	ChangeLog-2011 \
	ChangeLog-2012 \
	ChangeLog-2013 \
	ChangeLog-2014 \
	ChangeLog-2015 \
	ChangeLog-2016 \
	ChangeLog-2017 \
	ChangeLog-2018 \
	ChangeLog-2019 \
	ChangeLog-2020 \
	ChangeLog-2021 \
	ChangeLog-2022 \
	ChangeLog-2023 \
	ChangeLog-2024 \
	ChangeLog-2025 \
	ChangeLog-2026 \
	$(NULL)

man1_MANS = \
	   heimdall.1 \
	   heimdall-wrapper.1

EXTRA_DIST = \
	$(CHANGELOGS) \
	$(man1_MANS) \
	$(LIBHEIMDALL_SAMPLES) \
	fortunes.txt \
	release-howto.txt \
	Doxyfile.in \
	lirc/example.lircrc \
	skins/skins-howto.txt \
	skins/events-howto.txt \
	skins/curve_maker/Bezier.bas \
	skins/curve_maker/Bezier.frm \
	skins/curve_maker/Bezier.frx \
	skins/curve_maker/bezier.vbp \
	skins/curve_maker/Bezier.vbw \
	strings.txt \
	translations.txt \
	demo.sh \
	doc_helper.sh \
	ldoc-config.ld \
	$(NULL)

DISTCLEANFILES = Doxyfile
MAINTAINERCLEANFILES = $(CHANGELOGS)

Doxyfile: Doxyfile.in $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status --file=$(subdir)/$@

doc: Doxyfile
	doxygen

luadoc: ldoc-config.ld
	ldoc -c '$<' -d $(abs_builddir)/luadoc .

$(CHANGELOGS): Makefile.am
	if test -e "$(top_srcdir)/.git"; then \
		y="$@"; y="$${y##ChangeLog-}" ; \
		git --git-dir="$(top_srcdir)/.git" log \
			--since="$$y-01-01" \
			--until="$$y-12-31 23:00:00 -0100" \
			> "$@" ; \
	fi

# This one needs to be rebuilt all the time :)
.PHONY: ChangeLog-2024 changelogs doc luadoc

changelogs: $(CHANGELOGS)

#distclean-hook:
#	test "$(srcdir)" = "$(builddir)" || rm -f -- $(CHANGELOGS)
