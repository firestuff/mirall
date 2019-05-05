#!/bin/bash -ex

cd $(dirname $0)

make afl
afl-fuzz -i afl_state/testcases -o afl_state/findings -- ./fastcgi_conn_afl
