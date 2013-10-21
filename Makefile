all:
	gcc -o erln8 -lcurl `pkg-config --cflags --libs glib-2.0`  erln8.c
