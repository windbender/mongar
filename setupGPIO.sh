#!/bin/bash
/bin/echo 30 >/sys/class/gpio/export
/bin/echo both >/sys/class/gpio/gpio30/edge
