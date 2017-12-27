#!/bin/bash

function _cmd()
{
	echo "> $*" > /dev/stderr
	$*

	return $?
}

PROGF='./session'
[ -f "$PROGF" ] || {
  echo 'No `session` binary found in project directory.'
  echo 'Please `make all` before running tests!'
  echo 'Make sure you are running tests from the root project dir.'
  exit 1
}

TEST_DB=$(mktemp 'session.XXXX.db' --tmpdir=/tmp)
PROG="$PROGF --db $TEST_DB"

_cmd "$PROG --id test-sess --set name --value haha"
val=$(_cmd "$PROG --id test-sess --get name")

[ "$val" = "haha" ] || {
  echo 'FAILURE:'
  echo "Expected '$val' == 'haha'"
  exit 1
}
