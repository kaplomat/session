#!/bin/bash

PROG='../session'
[ -f "$PROG" ] || {
  echo 'No `session` binary found in project directory.'
  echo 'Please `make all` before running tests!'
  exit 1
}

echo "> $PROG --id test-sess --set name --value haha"
$PROG --id test-sess --set name --value haha

echo "$PROG --id test-sess --get name"
val=$(> $PROG --id test-sess --get name)
echo "$val"

[ "$val" = "haha" ] || {
  echo 'FAILURE:'
  echo "Expected '$val' == 'haha'"
  exit 1
}
