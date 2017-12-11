#!/bin/bash

[ -f ../session ] || {
  echo 'No `session` binary found in project directory.'
  echo 'Please `make all` before running tests!'
  exit 1
}
