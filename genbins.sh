#!/bin/bash
ERLBINS=(escript run_erl beam dyn_erl etop runcgi.sh beam.smp edoc_generate format_man_pages snmpc bench.sh emem getop start cdv epmd heart start_webtool child_setup erl inet_gethost to_erl codeline_preprocessing.escript erl.src makewhatis typer ct_run erl_call memsup xml_from_edoc.escript dialyzer erlc odbcserver diameterc erlexec printenv.sh)

cd build
for b in "${ERLBINS[@]}"
do
  ln -s erln8 $b
  chmod 755 $b
done
