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

/*
  TODO:
  build cleanup
  build env
  link/unlink
  add repo/rm repo
  add config/rm config
*/


/*
  memory management note:
  Since this program either exits or calls exec, there may be some
  pointers that aren't freed before calling g_error. I guess I don't
  really care about these. If you feel strongly about this,
  please fix and submit a pull request on Github.
*/


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
static gboolean opt_banner     = TRUE;

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
static gboolean  opt_gui       = FALSE;

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
  { "gui", 0, 0, G_OPTION_ARG_NONE, &opt_gui,
    "Show a gui if available", NULL },
  { "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug,
    "Debug Erln8", NULL },
  { NULL }
};


char* red() {
  return opt_color == TRUE ? ANSI_COLOR_RED : "";
}

char* green() {
  return opt_color == TRUE ? ANSI_COLOR_GREEN : "";
}

char* yellow() {
  return opt_color == TRUE ? ANSI_COLOR_YELLOW : "";
}

char* blue() {
  return opt_color == TRUE ? ANSI_COLOR_BLUE : "";
}

char* color_reset() {
  return opt_color == TRUE ? ANSI_COLOR_RESET : "";
}


void erln8_log( const gchar *log_domain,
    GLogLevelFlags log_level,
    const gchar *message,
    gpointer user_data ) {
  switch(log_level & G_LOG_LEVEL_MASK) {
    case G_LOG_FLAG_RECURSION:
    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_LEVEL_ERROR:
      fprintf(stderr, "%s", red());
      fprintf(stderr, "ERROR: %s",message);
      fprintf(stderr, "%s", color_reset());
      exit(-1);
      break;
    case G_LOG_LEVEL_WARNING:
      fprintf(stderr, "%s", yellow());
      fprintf(stderr, "WARNING: %s",message);
      fprintf(stderr, "%s", color_reset());
      break;
    case G_LOG_LEVEL_INFO:
    case G_LOG_LEVEL_MESSAGE:
      fprintf(stderr, "INFO: %s",message);
      break;
    case G_LOG_LEVEL_DEBUG:
      if(opt_debug) {
        fprintf(stderr, "DEBUG: %s",message);
      }
      break;
    default:
      fprintf(stderr, "%s", red());
      fprintf(stderr, "UNHANDLED: %s",message);
      fprintf(stderr, "%s", color_reset());
      break;
  }
  return;
}

// see if erlang i son the users path already
// if so, erln8 won't work correctly
gboolean erl_on_path() {
  int status = system("which erl > /dev/null");
  if(status == 0) {
    return 1;
  } else {
    return 0;
  }
}

// get a filename in the main config directory
gchar* get_configdir_file_name(char* filename) {
  gchar *configfilename = g_strconcat(homedir,
      "/.erln8.d/",
      filename,
      (char*)0);
  return configfilename;
}


// get a filename in a subdir of the config directory
gchar* get_config_subdir_file_name(char *subdir, char* filename) {
  gchar *configfilename = g_strconcat(homedir,
      "/.erln8.d/",
      subdir,
      "/",
      filename,
      (char*)0);
  return configfilename;
}


GHashTable* group_hash(char *group) {
  GHashTable *h = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  GKeyFile *kf = g_key_file_new();
  GError *error = NULL;
  gchar* fn = get_configdir_file_name("config");
  if(g_key_file_load_from_file(kf, fn, G_KEY_FILE_NONE, &error)) {
    if (error != NULL) {
      g_error("Unable to read file: %s\n", error->message);
      //g_error_free(error); program exits, can't free
    }
    GError *keyerror = NULL;
    gchar** keys = g_key_file_get_keys(kf, group, NULL, &keyerror);
    if (keyerror != NULL) {
      g_error("Unable to read %s section from ~/.erln8.d/config: %s\n", group, keyerror->message);
      //g_error_free(error);
    } else {
      gchar** it = keys;
      while(*it) {
        GError *valerror = NULL;
        gchar *v = g_key_file_get_string(kf, group, *it, &valerror);
        g_hash_table_insert(h, strdup(*it++), strdup(v));
        g_free(v);
      }
    }
    g_strfreev(keys);
    g_key_file_free(kf);
  } else {
    g_error("Cannot read from ~/.erln8.d/config\n");
  }
  g_free(fn);
  return h;
}

GHashTable* get_erlangs() {
  return group_hash("Erlangs");
}

GHashTable* get_repos() {
  return group_hash("Repos");
}

GHashTable* get_configs() {
  return group_hash("Configs");
}

GHashTable* get_erln8() {
  return group_hash("Erln8");
}

void e8_print(gpointer data, gpointer user_data) {
  printf("%s\n", (gchar*)data);
}

// check and see if the erln8 config directory exists
// probably poorly named
gboolean check_home() {
  gchar *configdir = g_strconcat(homedir, "/.erln8.d", (char*)0);
  g_debug("Checking config dir %s\n", configdir);
  gboolean result = g_file_test(configdir,
      G_FILE_TEST_EXISTS |
      G_FILE_TEST_IS_REGULAR);
  g_free(configdir);
  return result;
}

// make a subdirectory in the ~/.erln8.d directory
void mk_config_subdir(char *subdir) {
  gchar* dirname = g_strconcat(homedir,
      "/.erln8.d/",
      subdir,
      (char*)0);
  g_debug("Creating %s\n", dirname);
  if(g_mkdir(dirname, S_IRWXU)) {
    g_free(dirname);
    g_error("Can't create directory %s\n", dirname);
  } else {
    g_free(dirname);
  }
}

// generate the initial ~/.erln8.d/config file with some
// default settings
// TODO: more settings, Linux, FreeBSD etc
void init_main_config() {
  GKeyFile *kf = g_key_file_new();

  g_key_file_set_boolean(kf,
      "Erln8",
      "color",
      TRUE);

  g_key_file_set_comment(kf,
      "Erln8",
      "color",
      "Either true or false, case sensitive",
      NULL);

  g_key_file_set_boolean(kf,
      "Erln8",
      "banner",
      TRUE);

  g_key_file_set_comment(kf,
      "Erln8",
      "banner",
      "NOT IMPLEMENTED: show the version of Erlang that erln8 is running",
      NULL);

  g_key_file_set_string(kf,
      "Repos",
      "default",
      "https://github.com/erlang/otp.git");

  g_key_file_set_string(kf,
      "Configs",
      "osx_llvm",
      "--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit");

  g_key_file_set_string(kf,
      "Configs",
      "osx_gcc",
      "--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit");

  g_key_file_set_string(kf,
      "Configs",
      "osx_gcc_env",
      "CC=gcc-4.2 CPPFLAGS='-DNDEBUG' MAKEFLAGS='-j 3'");


  GError *error = NULL;
  gchar* d = g_key_file_to_data (kf, NULL, &error);
  if(error != NULL) {
    g_error("Unable to create ~/.erln8.d/config: %s\n", error->message);
    //g_error_free(error);
  }

  gchar* fn = get_configdir_file_name("config");
  printf("Creating erln8 config file: %s\n", fn);  
  GError *contentserror = NULL;
  if(!g_file_set_contents(fn, d, -1, &contentserror)) {
    if(contentserror != NULL) {
      g_error("Unable to write contents to ~/.erln8.d/config: %s\n", contentserror->message);
    } else {
      g_error("Unable to write contents to ~/.erln8.d/config\n");
    }
  }
  g_free(fn);
  g_free(d);
  g_key_file_free(kf);
}

// write an ./erln8.config file into the cwd
// won't override an existing file
// unless the user specifies --force
void init_here(char* erlang) {
  GHashTable *erlangs = get_erlangs();
  gboolean has_erlang = g_hash_table_contains(erlangs, erlang);
  g_hash_table_destroy(erlangs);
  if(!has_erlang) {
    g_error("%s is not a configured version of Erlang\n", erlang);
  }

  GKeyFile *kf = g_key_file_new();
  g_key_file_set_string(kf,
      "Config",
      "Erlang",
      erlang);

  GError *error = NULL;
  gchar* d = g_key_file_to_data (kf, NULL, &error);
  gchar* fn = "./erln8.config";
  gboolean result = FALSE;
  if(!opt_force) {
    result = g_file_test(fn, G_FILE_TEST_EXISTS |
        G_FILE_TEST_IS_REGULAR);
  }
  if(result) {
    g_error("Config already exists in this directory\n");
  } else {
    GError *error2 = NULL;
    if(!g_file_set_contents(fn, d, -1, &error2)) {
      if (error2 != NULL) {
        g_error("Unable to write file %s: %s\n", fn, error2->message);
      } else {
        g_error("Unable to write file %s\n", fn);
      }
    } else {
      fprintf(stderr, "Wrote to %s\n", fn);
    }
  }
  g_key_file_free(kf);
}

// list installed version of Erlang
void list_erlangs() {
  GKeyFile *kf = g_key_file_new();
  GError *error = NULL;
  gchar* fn = get_configdir_file_name("config");
  if(g_key_file_load_from_file(kf, fn, G_KEY_FILE_NONE, &error)) {
    if (error != NULL) {
      g_error("Unable to read file: %s\n", error->message);
      //g_error_free(error); program exits, can't free
    }
    printf("Available Erlang installations:\n");
    GError *keyerror = NULL;
    gchar** keys = g_key_file_get_keys(kf, "Erlangs", NULL, &keyerror);
    if (keyerror != NULL) {
      g_error("Unable to read Erlangs section from ~/.erln8.d/config: %s\n", keyerror->message);
      //g_error_free(error);
    } else {
      gchar** it = keys;
      while(*it) {
        GError *valerror = NULL;
        gchar *v = g_key_file_get_string(kf, "Erlangs", *it, &valerror);
        printf("%s -> %s\n",*it++, v);
        g_free(v);
      }
    }
    g_strfreev(keys);
    g_key_file_free(kf);
  } else {
    g_error("Cannot read from ~/.erln8.d/config\n");
  }
  g_free(fn);
}


// create ~/.erln8.d, ~/.erln8.d/config + related subdirs
// not atomic, if something blows up in here, the user will get
// a partial ~/.erln8.d tree
void initialize() {
  if(check_home()) {
    g_error("Configuration directory ~/.erln8.d already exists\n");
  } else {
    //if(erl_on_path()) {
    //  g_warning("Erlang already exists on the current PATH\n");
    //}
    // create the top level config directory, then create all subdirs
    gchar* dirname = g_strconcat(homedir, "/.erln8.d",(char*)0);
    g_debug("Creating %s\n", dirname);
    if(g_mkdir(dirname, S_IRWXU)) {
      g_error("Can't create directory %s\n", dirname);
      g_free(dirname);
      return;
    } else {
      g_free(dirname);
    }
    mk_config_subdir("otps");    // location of compiled otp source files
    mk_config_subdir("logs");    // logs!
    mk_config_subdir("repos"); // location of git repos
    init_main_config();
  }
}

// search up the directory tree for a erln8.config to use
char* configcheck(char *d) {
  char *retval = NULL;
  char *f = g_strconcat(d, "/erln8.config", NULL);
  GFile *gf = g_file_new_for_path(f);
  GFile *gd = g_file_new_for_path(d);

  if(g_file_query_exists(gf, NULL)) {
    char *cf = g_file_get_path(gf);
    retval = cf;
  } else {
    if(g_file_has_parent(gd, NULL)) {
      GFile *parent = g_file_get_parent(gd);
      char *pp = g_file_get_path(parent);
      retval = configcheck(pp);
      g_object_unref(parent);
      g_free(pp);
    }
  }

  g_free(f);
  g_object_unref(gf);
  g_object_unref(gd);
  return retval;
}

// search up the directory tree for an erln8.config to use,
// starting from cwd
char* configcheckfromcwd() {
  char *d = getcwd(NULL, MAXPATHLEN);
  char *retval = configcheck(d);
  g_free(d);
  return retval;
}


// which version of erlang is configured for this particular
// branch of the dir tree
char* which_erlang() {
  char* cfgfile = configcheckfromcwd();
  if(g_file_test(cfgfile, G_FILE_TEST_EXISTS |
        G_FILE_TEST_IS_REGULAR)) {
    GKeyFile* kf = g_key_file_new();
    GError* err = NULL;
    if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
      if(err != NULL) {
        g_error("Cannot load %s: %s\n", cfgfile, err->message);
      } else {
        g_error("Cannot load %s\n", cfgfile);
      }
    } else {
      if(!g_key_file_has_group(kf, "Config")) {
        g_error("Config group not defined in %s\n", cfgfile);
        return NULL;
      } else {
        err = NULL;
        if(g_key_file_has_key(kf, "Config", "Erlang", &err)) {
          gchar* erlversion = g_key_file_get_string(kf, "Config", "Erlang", &err);
          g_free(cfgfile);
          // THIS VALUE MUST BE FREED
          return erlversion;
        } else {
          if(err != NULL) {
            g_error("Missing Erlang | version: %s\n", err->message);
          } else {
            g_error("Missing Erlang | version\n");
          }
          return NULL;
        }
      }
    }
  } else {
    return NULL;
  }
  return NULL;
}

// set a ~/.erln8.d/config group/key value
// overwrites existing k/v's
char *set_config_kv(char *group, char *key, char *val) {
  gchar* cfgfile = get_configdir_file_name("config");
  GKeyFile* kf = g_key_file_new();
  GError* err = NULL;

  if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
    if(err != NULL) {
      fprintf (stderr,
          "Unable to load keyfile ~/.erln8.d/config: %s\n",
          err->message);
      g_error_free(err);
    } else {
      fprintf(stderr, "Unable to load keyfile ~/.erln8.d/config\n");
      // TODO: exit etc
    }
  } else {
    g_key_file_set_string(kf, group, key, val);
    GError *error = NULL;
    gchar* d = g_key_file_to_data (kf, NULL, &error);
    gchar* fn = get_configdir_file_name("config");
    g_debug("Writing to %s\n", fn);
    GError *error2 = NULL;
    if(!g_file_set_contents(fn, d, -1, &error2)) {
      if(error2 != NULL) {
        g_error("Error writing [%s] %s:%s to config file: %s\n",
            group, key, val, error2->message);
      } else {
        g_error("Error writing [%s] %s:%s to config file\n",
            group, key, val);
      }
    }
    g_free(fn);
    g_key_file_free(kf);
  }


  g_free(cfgfile);
  return val;
}


void git_fetch(char *repo) {
  GHashTable *repos = get_repos();
  gboolean has_repo = !g_hash_table_contains(repos, repo);
  g_hash_table_destroy(repos);
  if(!has_repo) {
    g_error("Unknown repo %s\n", repo);
  }
  gchar* source_path = get_config_subdir_file_name("repos", repo);
  if(!g_file_test(source_path, G_FILE_TEST_EXISTS |
        G_FILE_TEST_IS_REGULAR)) {
    g_error("Missing repo for %s, which should be in %s. Maybe you forgot to erln8 --clone repo_name\n",
            repo,
            source_path);
  }
  char *fetchcmd = g_strconcat("cd ",
                               source_path,
                               " && git fetch",
                               NULL);
  system(fetchcmd);
  g_free(source_path);
  g_free(fetchcmd);
}


void git_buildable(char *repo) {
  GHashTable *repos = get_repos();
  gboolean has_repo = g_hash_table_contains(repos, repo);
  g_hash_table_destroy(repos);
  if(!has_repo) {
    g_error("Unknown repo %s\n", repo);
  }
  gchar* source_path = get_config_subdir_file_name("repos", repo);
  if(!g_file_test(source_path, G_FILE_TEST_EXISTS |
        G_FILE_TEST_IS_REGULAR)) {
    g_error("Missing repo for %s, which should be in %s\n", repo, source_path);
  }

  char *fetchcmd = g_strconcat("cd ", source_path, " && git tag | sort", NULL);
  system(fetchcmd);
  g_free(source_path);
  g_free(fetchcmd);
}

static gchar *step[] = {
    "[0] copy source                    ",
    "[1] otp_build                      ",
    "[2] configure                      ",
    "[3] make                           ",
    "[4] make install                   ",
    (gchar*)0
};
static int step_count = 5;

void show_build_progress(int current_step, int exit_code) {
  if(exit_code != 0) {
    int len = strlen(step[current_step-1]);
    int i;
    for(i = 0; i < len; i++) {
      printf("\b");
    }
    printf("%s%s\n", yellow(), step[current_step-1]);
  } else if(current_step == step_count) {
    int len = strlen(step[current_step-1]);
    int i;
    for(i = 0; i < len; i++) {
      printf("\b");
    }
    printf("%s%s%s\n", green(), step[current_step-1], color_reset());
  } else if(current_step > 0) {
    int len = strlen(step[current_step-1]);
    int i;
    for(i = 0; i < len; i++) {
      printf("\b");
    }
    printf("%s%s\n", green(), step[current_step-1]),
    printf("%s%s", color_reset(), step[current_step]);
    fflush(stdout);
  } else {
    printf("%s%s", color_reset(), step[current_step]);
    fflush(stdout);
  }
}

void build_erlang(char *repo, char *tag, char *id, char *build_config) {
  // TODO:
  // make "tee" optional? -q: quiet build
  // check for valid repo
  // check for valid tag
  // check for compile flags
  // check for env
  // write to config "Erlangs"

  char pattern[] = "/tmp/erln8.buildXXXXXX";
  char* tmp = g_mkdtemp(pattern);
  g_debug("building in %s\n", tmp);
  gchar* output_path = get_config_subdir_file_name("otps",id);
  gchar* source_path = get_config_subdir_file_name("repos", repo);
  gchar* ld = g_strconcat("logs/build_", id, NULL);
  gchar* log_path    = get_configdir_file_name(ld);
  GHashTable *configs = get_configs();
  gchar* bc = NULL;

  if(build_config != NULL) {
    bc = (gchar*)g_hash_table_lookup(configs, build_config);
    // don't drop a NULL into the middle of the command string
    if(bc == NULL)
      bc = "";
  }
  g_free(ld);

  g_debug("Output path = %s\n", output_path);
  g_debug("Source path = %s\n", source_path);
  g_debug("Log path = %s\n", log_path);

  printf("Building %s from tag/branch %s of repo %s\n", id, tag, repo);
  printf("Custom build config: %s\n", bc);
  printf("Build log: %s\n", log_path);
  char *buildcmd0= g_strconcat("cd ", source_path, " && git archive ", tag, " | (cd ", tmp, "; tar x)", NULL);

  char *buildcmd1 = g_strconcat("cd ", tmp,
      " && ./otp_build autoconf >> ", log_path, " 2>&1", NULL);

  char *buildcmd2 = g_strconcat("cd ", tmp,
      "&& ./configure --prefix=", output_path," ",
      bc == NULL ? "" : bc,
      " >> ", log_path, " 2>&1",
      NULL);

  char *buildcmd3 = g_strconcat("cd ", tmp,
      " && make >> ", log_path,  " 2>&1", NULL);

  char *buildcmd4 = g_strconcat("cd ", tmp,
      " && make install >> ", log_path, " 2>&1", NULL);

  gchar* build_cmds[] = {
    buildcmd0,
    buildcmd1,
    buildcmd2,
    buildcmd3,
    buildcmd4,
    0
  };

  int result = 0;
  for(int i = 0; i <= step_count; i++) {
    show_build_progress(i, result);
    if(result != 0) {
      g_error("Build error, please check the build logs for more details\n");
    }
    result = system(build_cmds[i]);
  }

  set_config_kv("Erlangs", id, output_path);
  g_free(buildcmd0);
  g_free(buildcmd1);
  g_free(buildcmd2);
  g_free(buildcmd3);
  g_free(buildcmd4);

  g_free(log_path);
  g_free(source_path);
  g_free(output_path);
  g_free(bc);
  // destroy clost to the end so the string isn't freed before it's used
  g_hash_table_destroy(configs);
}

// if not executing one of the erlang commands
// then process erln8 options etc
int erln8(int argc, char* argv[]) {

  // TODO: think about getting erlangs, configs, repos in one go
  //       instead of for each option. meh, maybe I don't care.
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_error("erln8 option parsing failed: %s\n", error->message);
  }

  g_debug("argv[0] = [%s]\n",argv[0]);

  if(opt_init_erln8) {
    initialize();
    return 0;
  } else {
    if(!check_home()) {
      g_error("Please initialize erln8 with --init\n");
    }
  }

  if(opt_use) {
    init_here(opt_use);
    return 0;
  }

  if(opt_list) {
    list_erlangs();
    return 0;
  }

  if(opt_clone) {
    GHashTable *repos = get_repos();
    gchar *repo = g_hash_table_lookup(repos, opt_clone);
    if(repo == NULL) {
      g_hash_table_destroy(repos);
      g_error("Unknown repository %s\n", repo);
    } else {
      gchar* path = get_config_subdir_file_name("repos",opt_clone);
      gchar* cmd = g_strconcat("git clone ", repo, " ", path, NULL);
      system(cmd);
      g_hash_table_destroy(repos);
      g_free(cmd);
      g_free(path);
    }
    return 0;
  }

  if(opt_repos) {
    GHashTable *repos = get_repos();
    GList *keys = g_hash_table_get_keys(repos);
    g_list_foreach(keys, e8_print, NULL);
    g_list_free(keys);
    g_hash_table_destroy(repos);
    return 0 ;
  }

  if(opt_configs) {
    GHashTable *configs = get_configs();
    GList *keys = g_hash_table_get_keys(configs);
    g_list_foreach(keys, e8_print, NULL);
    g_list_free(keys);
    g_hash_table_destroy(configs);
    return 0;
  }

  if(opt_fetch) {
    if(opt_repo == NULL) {
      git_fetch("default");
    } else {
      git_fetch(opt_repo);
    }
    return 0;
  }

  if(opt_build) {
    gchar *repo = NULL;
    if(opt_repo == NULL) {
      printf("%sRepo not specified, using default%s\n", yellow(), color_reset());
      repo = "default";
    } else {
      repo = opt_repo;
    }
    if(repo == NULL) {
      g_error("build repo not specified");

    }
    if(opt_tag == NULL) {
      g_error("build tag not specified");
    }
    if(opt_id == NULL) {
      g_error("build id not specified");
    }

    // opt_config is optional
    g_debug("repo:%s\n", repo);
    g_debug("tag :%s\n", opt_tag);
    g_debug("id  :%s\n", opt_id);
    g_debug("cfg :%s\n", opt_config);

    build_erlang(repo, opt_tag, opt_id, opt_config);
    return 0;
  }

  if(opt_repoadd) {
    if(opt_repoadd == NULL || opt_repo != NULL) {
      g_message("Please use --repo-add-url along with --repo.\n");
      g_message("Example: erln8 --repo-add-url=https://github.com/foobar/otp.git --repo=foobar\n");
      g_error("Incomplete repo specification\n");
    } else {
      printf("RepoURL %s\n", opt_repoadd);
      printf("RepoID  %s\n", opt_repo);
      g_error("Not implemented\n");
      return 0;
    }
  }

  if(opt_reporm) {
    printf("Removing %s\n", opt_reporm);
    g_error("Not implemented\n");

    return 0;
  }

  if(opt_link) {
    printf("Not implemented\n");
    return 0;
  }

  if(opt_unlink) {
    printf("Not implemented\n");
    return 0;
  }

  if(opt_configadd) {
    g_error("Not implemented\n");
    return 0;
  }

  if(opt_configrm) {
    printf("Removing %s\n", opt_configrm);
    g_error("Not implemented\n");
    return 0;
  }

  if(opt_buildable) {
    if(opt_repo == NULL) {
      git_buildable("default");
    } else {
      git_buildable(opt_repo);
    }
    return 0;
  }

  if(opt_show) {
    char* erl = which_erlang();
    if(erl != NULL) {
      printf("%s\n", erl);
      g_free(erl);
      return 0;
    }
  }

  if(opt_prompt) {
    char* cfgfile = configcheckfromcwd();
    if(cfgfile != NULL) {
      g_free(cfgfile);
      char* erl = which_erlang();
      if(erl != NULL) {
        printf("%s", erl);
        g_free(erl);
      } else {
        printf("erln8 error");
      }
    } else {
      printf("none");
    }
    return 0;
  }

  printf("\nerln8: the sneaky Erlang version manager\n");
  printf("(c) 2013 Dave Parfitt\n");
  printf("Licensed under the Apache License, Version 2.0\n");
  printf("For more information, try erln8 --help\n\n");
  return 0;
}


/*
   void setup_binaries() {
   GHashTable *bins = g_hash_table_new(g_str_hash, g_str_equal);
   gchar** p = erts;
   while(*p != NULL) {
// gotta think about this for a bit...
//g_hash_table_insert(bins, *p++, "./lib/erlang/erts-star/bin");
}
gpointer* x = g_hash_table_lookup(bins, "erlc");
printf("%s\n", (gchar*)x);
}
*/




int main(int argc, char* argv[]) {
    // compiler will whine about it being deprecated, but taking it out
  // blows things up
  // used for GIO
  g_type_init();
  g_log_set_always_fatal(G_LOG_LEVEL_ERROR);
  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG |
      G_LOG_LEVEL_ERROR |
      G_LOG_LEVEL_WARNING |
      G_LOG_LEVEL_MESSAGE |
      G_LOG_FLAG_RECURSION |
      G_LOG_FLAG_FATAL,  erln8_log, NULL);
  homedir = g_get_home_dir();
  g_debug("home directory = %s\n", homedir);
  gchar* basename = g_path_get_basename(argv[0]);
  g_debug("basename = %s\n", basename);

  if((!strcmp(basename, "erln8")) || (!strcmp(basename, "./erln8"))) {
    erln8(argc, argv);
    g_free(basename);
  } else {
    g_free(basename);
    char *erl = which_erlang();
    if(erl == NULL) {
      g_message("Can't find an erln8.config file to use\n");
      list_erlangs();
      g_error("No erln8.config file\n");
    }
    GHashTable *erlangs = get_erlangs();
    GHashTable *runtime_options = get_erln8();
    char *path = g_hash_table_lookup(erlangs, erl);
    if(path == NULL) {
      g_hash_table_destroy(erlangs);
      g_hash_table_destroy(runtime_options);
      g_error("Version of Erlang (%s) isn't configured in erln8\n",
               erl);
    }
    gchar *use_color = (gchar*)g_hash_table_lookup(runtime_options, "color");
    if(g_strcmp0(use_color, "true") == 0) {
      opt_color = TRUE;
    } else {
      opt_color = FALSE;
    }

    gchar *use_banner = (gchar*)g_hash_table_lookup(runtime_options, "banner");
    if(g_strcmp0(use_banner, "true") == 0) {
      opt_banner = TRUE;
    } else {
      opt_banner = FALSE;
    }

    char *s = g_strconcat(path, "/bin/", argv[0], (char*)0);
    g_debug("%s\n",s);
    gboolean result = g_file_test(s,
      G_FILE_TEST_EXISTS |
      G_FILE_TEST_IS_REGULAR);
    if(!result) {
      g_hash_table_destroy(erlangs);
      g_hash_table_destroy(runtime_options);
      g_free(s);
      g_error("Can't run %s, check to see if the file exists\n", s);
    }

    if(opt_banner) {
      printf("%s", red());
      printf("erln8: %s", blue());
      printf("using Erlang %s", path);
      printf("%s\n", color_reset());
    }

    g_hash_table_destroy(erlangs);
    g_hash_table_destroy(runtime_options);
    // can't free s
    execv(s, argv);
  }
  return 0;
}
