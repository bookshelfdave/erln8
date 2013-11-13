all:
	gcc -o erln8 erln8.c `pkg-config --cflags --libs glib-2.0 gio-2.0` -DGLIB_DISABLE_DEPRECATION_WARNINGS -Wall
	rm -f ./erl ./erlc ./escript
	ln -s ./erln8 ./erl
	ln -s ./erln8 ./erlc
	ln -s ./erln8 ./escript


erln8r: erln8r.c
	gcc -o erln8r erln8r.c -lmenu -lncurses

