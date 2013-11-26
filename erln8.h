/*
 * ------------------------------------------------------------
 * erln8: a sneaky Erlang version manager
 *
 * Copyright (c) 2013 Dave Parfitt
 *
 * This file is provided to you under the Apache License,
 * Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain
 * a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * ------------------------------------------------------------
 */

#ifndef ERLN8_H
#define ERLN8_H


  void erln8_log(const gchar *log_domain,
                GLogLevelFlags log_level,
                const gchar *message,
                gpointer user_data );

  gboolean check_home();

  gboolean config_kv_exists(char *group, char *key);

  char* configcheck(char *d);

  char* configcheckfromcwd();

  char* which_erlang();

  char *get_config_kv(char *group, char *key);

  gboolean config_kv_exists(char *group, char *key);

  char **get_config_keys(char *group);

  char *set_config_kv(char *group, char *key, char *val);

  void git_fetch(char *repo);

  void git_buildable(char *repo);

  void build_erlang(char *repo, char *tag, char *id, char *build_config);

  int erln8(int argc, char* argv[]);


// WORK IN PROGRESS BELOW
/*
[0]:prime:~/.erln8.d/otps/r16b02$ ls -la ./lib/erlang/erts-5.10.3/bin
beam
beam.smp
child_setup
ct_run
dialyzer
dyn_erl
epmd
erl
erl.src
erlc
erlexec
escript
heart
inet_gethost
run_erl
start
start.src
start_erl.src
to_erl
typer

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/diameter-1.4.3/bin
diameterc

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/erl_docgen-0.3.4.1/priv/bin
codeline_preprocessing.escript xml_from_edoc.escript

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/erl_interface-3.7.14/bin
erl_call

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/inets-5.9.6/priv/bin
runcgi.sh

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/observer-1.3.1.1/priv/bin
cdv   etop  getop

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/odbc-2.10.17/priv/bin
odbcserver

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/os_mon-2.2.13/priv/bin
memsup

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/snmp-4.24.2/bin
snmpc

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/tools-2.6.12/bin
emem

[0]:prime:~/.erln8.d/otps/r16b02$ ls ./lib/erlang/lib/webtool-0.8.9.2/priv/bin
start_webtool

*/

/*
static gchar* erts[] = {
  "beam",
  "beam.smp",
  "child_setup",
  "ct_run",
  "dialyzer",
  "dyn_erl",
  "epmd",
  "erl",
  "erl.src",
  "erlc",
  "erlexec",
  "escript",
  "heart",
  "inet_gethost",
  "run_erl",
  "start",
  "start.src",
  "start_erl.src",
  "to_erl",
  "typer",
  0
};

static gchar* diameter[] = {
  "diameterc",
  0
};

static gchar* erl_docgen[] = {
  "codeline_preprocessing.escript",
  "xml_from_edoc.escript",
  0
};
*/

#endif
