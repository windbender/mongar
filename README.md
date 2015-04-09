# mongar
C/C++ program to run on beaglebone black to count GPIO transitions and report to graphite.

===================

This is a GLIB streams based counter which counts transitions on a GPIO line of the beaglebone black.

This code is setup to run on line 30. To enable the transition triggers of the input stream one needs to run the following two lines:

echo 30 >/sys/class/gpio/export
echo both >/sys/class/gpio/gpio30/edge

The first line will cause line 30 to be exported. The second line causes this line to trigger input on both rising and falling edges.

The code itself will count these transitions in a long int.  Once a minute it will report the number of transitions to a graphite instance.

On startup one must specify:
-p port of graphite instance. this is usually 2003
-h host of graphite instance.
-s string key to report the count into.
-i interval in seconds to report.  default is 60

As coded now, the once a minute reporting uses the SAME thread as the transition counter. If the process of connecting and reporting the value to graphite takes too long then the thread may be unable to count transitions.  This is a design deficiency.  I would like to change how this operates so that the reporting is done on a second thread.  And ideally there is some sort of mutex between the two threads.

