mongar: mongar.cpp Makefile
	g++ mongar.cpp -o mongar ` pkg-config glib-2.0  --cflags --libs`
