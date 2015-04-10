#!/bin/bash
echo 30 >/sys/class/gpio/export
echo both >/sys/class/gpio/gpio30/edge
