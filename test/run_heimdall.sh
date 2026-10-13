#! /bin/sh

set -e

Heimdall="./heimdall --ignore-config --rc-fake-tty"

$Heimdall -H
$Heimdall -Idummy heimdall://quit
$Heimdall -vv -Irc,oldrc heimdall://quit
$Heimdall -vv -Irc,oldrc --play-and-exit heimdall://nop

ASAN_OPTIONS="$ASAN_OPTIONS,detect_leaks=0"
export ASAN_OPTIONS

$Heimdall --play-and-exit heimdall://nop
