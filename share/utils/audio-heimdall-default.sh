#!/bin/sh
# $Id$
#
# Copyright © 2007 VideoLAN team
# Copyright (C) 2026 TheFenrisLycaon
#
# Authors: http://forum.videolan.org/viewtopic.php?f=13&t=40560
#          Rafaël Carré <funman@lycariaorg>
#
# Sets Heimdall media player the default application for audio mime types
# on a freedesktop compliant desktop
#

MIME_FILE=~/.local/share/applications/defaults.list

if [ ! -f $MIME_FILE ]
    then echo "[Default Applications]" > $MIME_FILE
    else grep -v 'audio/' $MIME_FILE > /tmp/heimdall.defaults.list.tmp
    mv /tmp/heimdall.defaults.list.tmp $MIME_FILE
fi

ls /usr/share/mime/audio/* | sed -e 's@/usr/share/mime/@@' -e 's/\.xml/=heimdall.desktop/' >> $MIME_FILE
