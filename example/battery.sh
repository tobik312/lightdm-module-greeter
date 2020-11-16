#!/bin/sh

charged=$'\U0f0e7'
grep -q 0 /sys/class/power_supply/AC/online && charged=$'\U0f240'
echo "$charged $(cat /sys/class/power_supply/BAT0/capacity)%"
