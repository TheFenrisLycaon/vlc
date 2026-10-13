#!/bin/sh

TELNET_FAIL="telnet_fail"
PORT=`echo "$$+1024" | bc`

killer()
{
  sleep 2 && ps $1 > /dev/null && touch $TELNET_FAIL && kill -9 $1
}

wait_or_quit()
{
  wait $1
  if [ `ls $TELNET_FAIL 2> /dev/null | wc -l` = 1 ]
  then
    rm -f $TELNET_FAIL
    exit 1
  fi
}

# Remove the fail file if needed
rm -f $TELNET_FAIL

# Test that Heimdall handle options correctly
../heimdall -I luatelnet --telnet-port $PORT &
HEIMDALL1=$!
sleep 1
killer $HEIMDALL1 &
echo "admin\nshutdown\n" | nc localhost $PORT
wait_or_quit $HEIMDALL1

../heimdall -I luatelnet --telnet-port $PORT --telnet-password bla &
HEIMDALL2=$!
sleep 1
killer $HEIMDALL2 &
echo "bla\nshutdown\n" | nc localhost $PORT
wait_or_quit $HEIMDALL2

../heimdall -I luatelnet --telnet-port $PORT --telnet-password one_long_password &
HEIMDALL3=$!
sleep 1
killer $HEIMDALL3 &
echo "one_long_password\nshutdown\n" | nc localhost $PORT
wait_or_quit $HEIMDALL3

../heimdall -I luatelnet --telnet-port $PORT --telnet-password "" &
HEIMDALL4=$!
sleep 1
killer $HEIMDALL4 &
echo "\nshutdown\n" | nc localhost $PORT
wait_or_quit $HEIMDALL4

exit 0
