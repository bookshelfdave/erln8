all:
	gcc -o erln8 `pkg-config --cflags --libs glib-2.0 gio-2.0` erln8.c -DGLIB_DISABLE_DEPRECATION_WARNINGS
	rm -f ./erl ./erlc ./escript
	ln -s ./erln8 ./erl
	ln -s ./erln8 ./erlc
	ln -s ./erln8 ./escript
