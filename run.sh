#!/bin/bash

make
ext=$?
if [[ $ext -ne 0 ]]; then
    exit $ext
fi
sudo setcap cap_net_admin=eip ./tc++p
./tc++p &
pid=$!

trap "kill $pid" INT TERM
wait $pid

