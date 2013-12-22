ERLBINS=Install escript run_erl beam dyn_erl etop runcgi.sh beam.smp edoc_generate format_man_pages snmpc bench.sh emem getop start cdv epmd heart start_webtool child_setup erl inet_gethost to_erl codeline_preprocessing.escript erl.src makewhatis typer ct_run erl_call memsup xml_from_edoc.escript dialyzer erlc odbcserver diameterc erlexec printenv.sh

all:
	gcc -o erln8 erln8.c `pkg-config --cflags --libs glib-2.0 gio-2.0` -DGLIB_DISABLE_DEPRECATION_WARNINGS -Wall
	rm -f ./erl ./erlc ./escript ./dialyzer
	$(foreach b,$(ERLBINS),ln -s ./erln8 $(b);)

format:
	astyle --style=attach --indent=spaces=2 --indent-cases --delete-empty-lines --align-pointer=type --align-reference=type --mode=c ./erln8.c

clean:
	rm -f erln8
	$(foreach b,$(ERLBINS),rm -f $(b);)
