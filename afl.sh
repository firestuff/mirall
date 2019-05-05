#!/bin/bash -ex

cd $(dirname $0)

make afl
afl-fuzz -i afl/testcases -o afl/findings -- ./fastcgi_conn_afl
