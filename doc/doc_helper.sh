#! /bin/sh

# Add <heimdall_common.h> before most headers
# from old toolbox

# Clean env
LC_ALL=C
export LC_ALL
LANG=C
export LANG

if test "$1" = "" ; then
  exit 0
fi

##  Add includes to help doxygen
case "$1" in
  */heimdall_common.h|*/include/heimdall/*);;
  */include/*.h) echo "#include <heimdall_common.h>" ;;
esac
cat $1
exit 0

