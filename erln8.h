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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/param.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gio/gio.h>


#define G_LOG_DOMAIN    ((gchar*) 0)

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static gboolean opt_init_erln8 = FALSE;
static gboolean opt_debug      = FALSE;
static gchar*   opt_use        = NULL;
static gboolean opt_list       = FALSE;
static gboolean opt_buildable  = FALSE;
static gboolean opt_fetch      = FALSE;
static gboolean opt_build      = FALSE;
static gboolean opt_show       = FALSE;
static gchar*   opt_clone      = NULL;
static gboolean opt_color      = TRUE;

static gchar*   opt_repo       = NULL;
static gchar*   opt_tag        = NULL;
static gchar*   opt_id         = NULL;
static gchar*   opt_config     = NULL;

static gboolean  opt_configs   = FALSE;
static gboolean  opt_repos     = FALSE;
static gchar*    opt_link      = NULL;
static gchar*    opt_unlink    = NULL;
static gboolean  opt_force     = FALSE;

static gchar*    opt_repoadd   = NULL;
static gchar*    opt_reporm    = NULL;
static gchar*    opt_configadd = NULL;
static gchar*    opt_configrm  = NULL;
static gboolean  opt_prompt    = FALSE;

static const gchar* homedir;

static GOptionEntry entries[] =
{
  { "init", 0, 0, G_OPTION_ARG_NONE, &opt_init_erln8,
    "Initializes erln8", NULL },
  { "use", 0, 0, G_OPTION_ARG_STRING, &opt_use,
    "Setup the current directory to use a specific verion of Erlang", "id"},
  { "list", 0, 0, G_OPTION_ARG_NONE, &opt_list,
    "List available Erlang installations", NULL },
  { "clone", 0, 0, G_OPTION_ARG_STRING, &opt_clone,
    "Clone an Erlang source repository", "repo"},
  { "fetch", 0, 0, G_OPTION_ARG_NONE, &opt_fetch,
    "Update source repos", "repo"},
  { "build", 0, 0, G_OPTION_ARG_NONE, &opt_build,
    "Build a specific version of OTP from source", NULL },
  { "repo", 0, 0, G_OPTION_ARG_STRING, &opt_repo,
    "Specifies repo name to build from", "repo"},
  { "tag", 0, 0, G_OPTION_ARG_STRING, &opt_tag,
    "Specifies repo branch/tag to build from", "git_tag"},
  { "id", 0, 0, G_OPTION_ARG_STRING, &opt_id,
    "A user assigned name for a version of Erlang", "id"},
  { "config", 0, 0, G_OPTION_ARG_STRING, &opt_config,
    "Build configuration", "config"},
  { "show", 0, 0, G_OPTION_ARG_NONE, &opt_show,
    "Show the configured version of Erlang in the current working directory", NULL },
  { "prompt", 0, 0, G_OPTION_ARG_NONE, &opt_prompt,
    "Display the version of Erlang configured for this part of the directory tree", NULL },
  { "configs", 0, 0, G_OPTION_ARG_NONE, &opt_configs,
    "List build configs", NULL },
  { "repos", 0, 0, G_OPTION_ARG_NONE, &opt_repos,
    "List build repos", NULL },
  { "link", 0, 0, G_OPTION_ARG_STRING, &opt_link,
    "Link a non-erln8 build of Erlang to erln8", NULL },
  { "unlink", 0, 0, G_OPTION_ARG_STRING, &opt_unlink,
    "Unlink a non-erln8 build of Erlang from erln8", NULL },
  { "force", 0, 0, G_OPTION_ARG_NONE, &opt_force,
    "Use the force", NULL },
  { "repo-add-url", 0, 0, G_OPTION_ARG_STRING, &opt_repoadd,
    "Add a repo", "repo-id"},
  { "repo-rm-url", 0, 0, G_OPTION_ARG_STRING, &opt_reporm,
    "Remote a repo", "repo-id"},
  { "config-add", 0, 0, G_OPTION_ARG_STRING, &opt_configadd,
    "Add an Erlang build config", "config-id"},
  { "config-rm", 0, 0, G_OPTION_ARG_STRING, &opt_configrm,
    "Remove an Erlang build config", "config-id"},
  { "no-color", 0, 0, G_OPTION_ARG_NONE, &opt_color,
    "Don't use color output", NULL },
  { "buildable", 0, 0, G_OPTION_ARG_NONE, &opt_buildable,
    "List tags to build from configured source repos", NULL },
  { "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug,
    "Debug Erln8", NULL },
  { NULL }
};

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
