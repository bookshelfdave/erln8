all:
	gcc -o erln8 erln8.c `pkg-config --cflags --libs glib-2.0 gio-2.0` -DGLIB_DISABLE_DEPRECATION_WARNINGS -Wall
	rm -f ./erl ./erlc ./escript ./dialyzer
	ln -s ./erln8 ./erl
	ln -s ./erln8 ./erlc
	ln -s ./erln8 ./escript
	ln -s ./erln8 ./dialyzer

format:
	astyle --style=attach --indent=spaces=2 --indent-cases --delete-empty-lines --align-pointer=type --align-reference=type --mode=c ./erln8.c

clean:
	rm -f erln8
	rm -f erln8r
