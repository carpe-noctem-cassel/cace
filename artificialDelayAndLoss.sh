#!/bin/sh

tc qdisc add dev eth0 root netem delay 100ms loss 10%
#tc qdisc change dev eth0 root netem loss 0.1%
