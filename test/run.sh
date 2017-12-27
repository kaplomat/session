#!/bin/bash

function _log()
{
    printf '%s\n' "$*" > /dev/stderr
}

function test_log()
{
    _log "*** $*"
}

function _cmd()
{
    _log ">" "$*"
	$*

	return $?
}

function test_cmd()
{
    _cmd $*
    [ ! $? ] && {
        test_log "Command failed, exiting"
    }
}

function setup()
{
    test_log 'SETUP'
    TEST_DB=$(mktemp 'session.XXXX.db' --tmpdir=/tmp)
    PROGF='./session'
    [ -f "$PROGF" ] || {
      echo 'No `session` binary found in project directory.'
      echo 'Please `make all` before running tests!'
      echo 'Make sure you are running tests from the root project dir.'
      test_finish 1
    }
    PROG="$PROGF --db $TEST_DB"
}

function teardown()
{
    test_log 'TEARDOWN'
    _cmd "rm $TEST_DB"
}

function test_start()
{
    setup
    test_log "TEST - $1"
}

function test_finish()
{
    teardown
}

test_start 'basic setting and getting a value'

test_cmd "$PROG --id test-sess --set name --value haha"
val=$(test_cmd "$PROG --id test-sess --get name")

[ "$val" = "haha" ] || {
  test_log 'FAILURE:'
  test_log "Expected '$val' == 'haha'"
}

test_finish

test_start 'empty values'

val=$(test_cmd "$PROG --id session1 --get name")
[ "$val" = "" ] || {
    test_log 'FAILURE:'
    test_log "Expected '$val' == ''"
}

test_finish

test_start 'session isolation'

test_cmd "$PROG --id session1 --set user.name --value johndoe23"
val=$(test_cmd "$PROG --id session1 --get user.name")

[ "$val" = "johndoe23" ] || {
    test_log 'FAILURE:'
    test_log "Expected '$val' == 'johndoe23'"
}

val=$(test_cmd "$PROG --id session2 --get user.name")
[ "$val" = "" ] || {
    test_log 'FAILURE:'
    test_log "Expected '$val' == ''"
}

test_finish
