ERLBINS=Install escript run_erl beam dyn_erl etop runcgi.sh beam.smp edoc_generate format_man_pages snmpc bench.sh emem getop start cdv epmd heart start_webtool child_setup erl inet_gethost to_erl codeline_preprocessing.escript erl.src makewhatis typer ct_run erl_call memsup xml_from_edoc.escript dialyzer erlc odbcserver diameterc erlexec printenv.sh

PREFIX?=/usr/local
BINDIR=$(PREFIX)/bin
CC=gcc
CFLAGS=-O2 -Wall
PROG=erln8
VERSION=0.6.0

$(PROG): *.c
	$(CC) -o erln8 erln8.c `pkg-config --cflags --libs glib-2.0 gio-2.0` -DGLIB_DISABLE_DEPRECATION_WARNINGS $(CFLAGS)

format:
	astyle --style=attach --indent=spaces=2 --indent-cases --delete-empty-lines --align-pointer=type --align-reference=type --mode=c ./erln8.c

clean:
	rm -f erln8

install: $(PROG)  uninstall
	echo "Installing"
	mkdir -p $(BINDIR)
	cp ./erln8 $(BINDIR)/erln8
	$(foreach b,$(ERLBINS),ln -s $(BINDIR)/erln8 $(BINDIR)/$(b);)
	$(foreach b,$(ERLBINS),chmod 755 $(BINDIR)/$(b);)

uninstall:
	echo "Uninstalling"
	rm -f $(BINDIR)/$(PROG)
	$(foreach b,$(ERLBINS),rm -f $(BINDIR)/$(b);)

deb:
	fpm -s dir -t deb -n erln8 -v $(VERSION) -C /tmp/installdir \
  -p ./packages/erln8-VERSION-ARCH.deb \
  -d "gcc (> 0)" \
  -d "glibc-devel (> 0)" \
  -d "make (> 0)" \
  -d "ncurses-devel (> 0)" \
  -d "openssl-devel (> 0)" \
  -d "autoconf (> 0)" \
  -d "glib2-devel(> 0)" \
  usr/local/bin
