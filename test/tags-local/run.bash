#!/usr/bin/env bash

base=$(dirname $0)

source $base/../common.bash

tags_test_file() {
  if [[ $mode == 'native' ]]
  then
    tree-sitter tags $2
  elif [[ $mode == 'wasm' ]]
  then
    message "Tags tests can't be run in wasm."
    exit 1
  else
    message "Invalid mode: $mode"
    exit 1
  fi
}

test_files 'tags' tags_test_file
exit
