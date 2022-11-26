#!/bin/bash

make
ext=$?
if [[ $ext -ne 0 ]]; then
    exit $ext
fi
sudo setcap cap_net_admin=eip ./tc++p
./tc++p &
pid=$!

sleep 5
sudo ip link add name br0 type bridge
sudo ip link set dev br0 up
sudo ip link set tun0 master br0
sudo ip link set tun1 master br0

# sudo ip addr add 192.168.0.1/24 dev tun0
# sudo ip link set up dev tun0
trap "kill $pid" INT TERM
wait $pid

sudo ip link set dev br0 down
sudo ip link delete br0
