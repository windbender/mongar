# mongar
C/C++ program to run on beaglebone black to count GPIO transitions and report to graphite.

===================

This is a GLIB streams based counter which counts transitions on a GPIO line of the beaglebone black.

modern BBB use debian as their install. So I have included to systemd service scripts and a script which sets up the GPIO line. These the two *.service scripts get copied into /etc/system/system  (do this as root ). You should modify BOTH of these to suit your particular setup, the server you are reporting, the GPIO line you are using, and where the setupGPIO.sh script is located.

I need to pull the exact GPIO line out as another CLI param, right now that is hard coded in the mongar.cpp



if you want to roll your own or manually start up, you need to do something like this to get the IO lines working:

echo 30 >/sys/class/gpio/export

echo both >/sys/class/gpio/gpio30/edge

The first line will cause line 30 to be exported. The second line causes this line to trigger input on both rising and falling edges.

The code itself will count these transitions in a long int.  Once a minute it will report the number of transitions to a graphite instance.

On startup one must specify:

-p port of graphite instance. this is usually 2003

-h host of graphite instance.

-s string key to report the count into.

-i interval in seconds to report.  default is 60

