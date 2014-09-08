/*
 * ------------------------------------------------------------
 * erln8: a sneaky Erlang version manager
 *
 * Copyright (c) 2014 Dave Parfitt
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
  split into multiple .c files
  code cleanup
*/

// enjoy the mix of Java, Erlang, and C styles throughout the 
// code. Guess it's not good to code when you have the flu.

/*
  memory management note:
  Since this program either exits or calls exec, there may be some
  pointers that aren't freed before calling g_error. I guess I don't
  really care about these. If you feel strongly about this,
  please fix and submit a pull request on Github.
*/


/*
ERLN8_CONFIG_DIR/
    config
    logs/
    otps/
      foo/
        dist/
      bar/
        dist/
      baz/
        dist/ (symlinked)
    repos/
      x/
      y/
    logs/
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

#define ERLN8_CONFIG_FILE	"erln8.config"
#define ERLN8_CONFIG_DIR	".erln8.d"

#define G_LOG_DOMAIN    ((gchar*) 0)

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static gboolean  opt_init_erln8 = FALSE;
static gboolean  opt_debug      = FALSE;
static gchar*    opt_use        = NULL;
static gboolean  opt_list       = FALSE;
static gboolean  opt_buildable  = FALSE;
static gboolean  opt_fetch      = FALSE;
static gboolean  opt_build      = FALSE;
static gboolean  opt_show       = FALSE;
static gchar*    opt_clone      = NULL;
static gboolean  opt_color      = TRUE;
static gboolean  opt_banner     = FALSE;
static gchar*    opt_repo       = NULL;
static gchar*    opt_tag        = NULL;
static gchar*    opt_id         = NULL;
static gchar*    opt_config     = NULL;
static gboolean  opt_configs    = FALSE;
static gboolean  opt_repos      = FALSE;
static gchar*    opt_link       = NULL;
static gboolean  opt_unlink     = FALSE;
static gboolean  opt_force      = FALSE;
static gboolean  opt_prompt     = FALSE;
static gboolean  opt_quickstart = FALSE;
static gboolean  opt_dryrun     = FALSE;

static const gchar* homedir;

static GOptionEntry entries[] = {
  {
    "init", 0, 0, G_OPTION_ARG_NONE, &opt_init_erln8,
    "Initializes erln8", NULL
  },
  {
    "use", 0, 0, G_OPTION_ARG_STRING, &opt_use,
    "Setup the current directory to use a specific verion of Erlang", "id"
  },
  {
    "list", 0, 0, G_OPTION_ARG_NONE, &opt_list,
    "List available Erlang installations", NULL
  },
  {
    "clone", 0, 0, G_OPTION_ARG_STRING, &opt_clone,
    "Clone an Erlang source repository", "repo"
  },
  {
    "fetch", 0, 0, G_OPTION_ARG_NONE, &opt_fetch,
    "Update source repos", "repo"
  },
  {
    "build", 0, 0, G_OPTION_ARG_NONE, &opt_build,
    "Build a specific version of OTP from source", NULL
  },
  {
    "repo", 0, 0, G_OPTION_ARG_STRING, &opt_repo,
    "Specifies repo name to build from", "repo"
  },
  {
    "tag", 0, 0, G_OPTION_ARG_STRING, &opt_tag,
    "Specifies repo branch/tag to build from", "git_tag"
  },
  {
    "id", 0, 0, G_OPTION_ARG_STRING, &opt_id,
    "A user assigned name for a version of Erlang", "id"
  },
  {
    "config", 0, 0, G_OPTION_ARG_STRING, &opt_config,
    "Build configuration", "config"
  },
  {
    "show", 0, 0, G_OPTION_ARG_NONE, &opt_show,
    "Show the configured version of Erlang in the current working directory", NULL
  },
  {
    "prompt", 0, 0, G_OPTION_ARG_NONE, &opt_prompt,
    "Display the version of Erlang configured for this part of the directory tree", NULL
  },
  {
    "configs", 0, 0, G_OPTION_ARG_NONE, &opt_configs,
    "List build configs", NULL
  },
  {
    "repos", 0, 0, G_OPTION_ARG_NONE, &opt_repos,
    "List build repos", NULL
  },
  {
    "link", 0, 0, G_OPTION_ARG_STRING, &opt_link,
    "Link a non-erln8 build of Erlang to erln8", NULL
  },
  {
    "unlink", 0, 0, G_OPTION_ARG_NONE, &opt_unlink,
    "Unlink a non-erln8 build of Erlang from erln8", NULL
  },
  {
    "force", 0, 0, G_OPTION_ARG_NONE, &opt_force,
    "Use the force", NULL
  },
  {
    "no-color", 0, 0, G_OPTION_ARG_NONE, &opt_color,
    "Don't use color output", NULL
  },
  {
    "buildable", 0, 0, G_OPTION_ARG_NONE, &opt_buildable,
    "List tags to build from configured source repos", NULL
  },
  {
    "quickstart", 0, 0, G_OPTION_ARG_NONE, &opt_quickstart,
    "Initialize erln8 and build the latest version of Erlang", NULL
  },
  /*
  {
    "dryrun", 0, 0, G_OPTION_ARG_NONE, &opt_dryrun,
    "Show build commands but don't execute them", NULL
  },
  */
  {
    "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug,
    "Debug Erln8", NULL
  },
  { NULL }
};

static gchar* step[] = {
  "[0] copy source                    ",
  "[1] otp_build                      ",
  "[2] configure                      ",
  "[3] make                           ",
  "[4] make install                   ",
  "[5] make install-docs              ",
  (gchar*)0
};

static int step_count = 6;

gchar* red() {
  return opt_color == TRUE ? ANSI_COLOR_RED : "";
}

gchar* green() {
  return opt_color == TRUE ? ANSI_COLOR_GREEN : "";
}

gchar* yellow() {
  return opt_color == TRUE ? ANSI_COLOR_YELLOW : "";
}

gchar* blue() {
  return opt_color == TRUE ? ANSI_COLOR_BLUE : "";
}

gchar* color_reset() {
  return opt_color == TRUE ? ANSI_COLOR_RESET : "";
}


void erln8_log( const gchar* log_domain,
                GLogLevelFlags log_level,
                const gchar* message,
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
gchar* get_configdir_file_name(gchar* filename) {
  gchar* configfilename = g_strconcat(homedir,
                                      "/" ERLN8_CONFIG_DIR "/",
                                      filename,
                                      (gchar*)0);
  return configfilename;
}


// get a filename in a subdir of the config directory
gchar* get_config_subdir_file_name(gchar* subdir, gchar* filename) {
  gchar* configfilename = g_strconcat(homedir,
                                      "/" ERLN8_CONFIG_DIR "/",
                                      subdir,
                                      "/",
                                      filename,
                                      (gchar*)0);
  return configfilename;
}

GHashTable* group_hash(gchar* group, gboolean ignore_error) {
  GHashTable* h = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  GKeyFile* kf = g_key_file_new();
  GError* error = NULL;
  gchar* fn = get_configdir_file_name("config");
  if(g_key_file_load_from_file(kf, fn, G_KEY_FILE_NONE, &error)) {
    if (error != NULL) {
      g_error("Unable to read file: %s\n", error->message);
      //g_error_free(error); program exits, can't free
    }
    GError* keyerror = NULL;
    gchar** keys = g_key_file_get_keys(kf, group, NULL, &keyerror);
    if (keyerror != NULL) {
      if(!ignore_error) {
        g_error("Unable to read %s section from ~/" ERLN8_CONFIG_DIR "/config: %s\n", group, keyerror->message);
      } else {
        // just return NULL
        g_free(fn);
        g_key_file_free(kf);
        g_error_free(keyerror);
        return NULL;
      }
      //g_error_free(error);
    } else {
      gchar** it = keys;
      while(*it) {
        GError* valerror = NULL;
        gchar* v = g_key_file_get_string(kf, group, *it, &valerror);
        g_hash_table_insert(h, strdup(*it++), strdup(v));
        g_free(v);
      }
    }
    g_strfreev(keys);
    g_key_file_free(kf);
  } else {
    g_error("Cannot read from ~/" ERLN8_CONFIG_DIR "/config\n");
  }
  g_free(fn);
  return h;
}

GHashTable* get_erlangs() {
  return group_hash("Erlangs", FALSE);
}

GHashTable* get_repos() {
  return group_hash("Repos", FALSE);
}

GHashTable* get_configs() {
  return group_hash("Configs", FALSE);
}

GHashTable* get_erln8() {
  return group_hash("Erln8", FALSE);
}

GHashTable* get_system_roots() {
  return group_hash("SystemRoots", TRUE);
}

void git_allbuildable() {
  GHashTable* repos = get_repos();
  GHashTableIter iter;
  gpointer repo, value;
  g_hash_table_iter_init (&iter, repos);
  while (g_hash_table_iter_next(&iter, &repo, &value)) {
    gchar* source_path = get_config_subdir_file_name("repos", repo);
    if(!g_file_test(source_path, G_FILE_TEST_EXISTS |
                    G_FILE_TEST_IS_REGULAR)) {
      g_error("Missing repo for %s, which should be in %s\n", (gchar*)repo, source_path);
    }
    printf("Tags for repo %s:\n", (gchar*)repo);
    gchar* fetchcmd = g_strconcat("cd ",
                                 source_path,
                                 " && git tag | sort",
                                 NULL);
    int result = system(fetchcmd);
    if(result != 0) {
      g_error("Cannot get a list of git tags.\n");
    }
    g_free(fetchcmd);
    g_free(source_path);
  }
}

void e8_print(gpointer data, gpointer user_data) {
  printf("%s\n", (gchar*)data);
}

// check and see if the ERLN8_CONFIG_FILE directory exists
// probably poorly named
gboolean check_home() {
  gchar* configdir = g_strconcat(homedir, "/" ERLN8_CONFIG_DIR "", (gchar*)0);
  g_debug("Checking config dir %s\n", configdir);
  gboolean result = g_file_test(configdir,
                                G_FILE_TEST_EXISTS |
                                G_FILE_TEST_IS_REGULAR);
  g_free(configdir);
  return result;
}

// make a subdirectory in the ~/ERLN8_CONFIG_DIR directory
void mk_config_subdir(gchar* subdir) {
  gchar* dirname = g_strconcat(homedir,
                               "/" ERLN8_CONFIG_DIR "/",
                               subdir,
                               (gchar*)0);
  g_debug("Creating %s\n", dirname);
  if(g_mkdir(dirname, S_IRWXU)) {
    g_free(dirname);
    g_error("Can't create directory %s\n", dirname);
  } else {
    g_free(dirname);
  }
}

// generate the initial ~/ERLN8_CONFIG_DIR/config file with some
// default settings
// TODO: more settings, Linux, FreeBSD etc
void init_main_config() {
  GKeyFile* kf = g_key_file_new();
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
                         FALSE);
  g_key_file_set_comment(kf,
                         "Erln8",
                         "banner",
                         "Show the version of Erlang that erln8 is running",
                         NULL);
  g_key_file_set_string(kf,
                        "Erln8",
                        "default_config",
                        "default");
  g_key_file_set_comment(kf,
                         "Erln8",
                         "default_config",
                         "The default config to use for a build",
                         NULL);
  g_key_file_set_string(kf,
                        "Erln8",
                        "system_default",
                        "");
  g_key_file_set_comment(kf,
                         "Erln8",
                         "system_default",
                         "If an erln8.config file isn't found, use this one.",
                         NULL);
  g_key_file_set_string(kf,
                        "Repos",
                        "default",
                        "https://github.com/erlang/otp.git");
  g_key_file_set_string(kf,
                        "Erlangs",
                        "none",
                        "");
  g_key_file_set_string(kf,
                        "Configs",
                        "default",
                        "");
  g_key_file_set_string(kf,
                        "Configs",
                        "osx_llvm",
                        "--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit");
  g_key_file_set_string(kf,
                        "Configs",
                        "osx_llvm_dtrace",
                        "--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit --enable-vm-probes --with-dynamic-trace=dtrace");
  g_key_file_set_string(kf,
                        "Configs",
                        "osx_gcc",
                        "--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit");
  g_key_file_set_string(kf,
                        "Configs",
                        "osx_gcc_env",
                        "CC=gcc-4.2 CPPFLAGS=\'-DNDEBUG\' MAKEFLAGS=\'-j 3\'");
  GError* error = NULL;
  gchar* d = g_key_file_to_data (kf, NULL, &error);
  if(error != NULL) {
    g_error("Unable to create ~/" ERLN8_CONFIG_DIR "/config: %s\n", error->message);
    //g_error_free(error);
  }
  gchar* fn = get_configdir_file_name("config");
  printf("Creating " ERLN8_CONFIG_FILE " file: %s\n", fn);
  GError* contentserror = NULL;
  if(!g_file_set_contents(fn, d, -1, &contentserror)) {
    if(contentserror != NULL) {
      g_error("Unable to write contents to ~/" ERLN8_CONFIG_DIR "/config: %s\n", contentserror->message);
    } else {
      g_error("Unable to write contents to ~/" ERLN8_CONFIG_DIR "/config\n");
    }
  }
  g_free(fn);
  g_free(d);
  g_key_file_free(kf);
  if(!opt_quickstart) {
    printf("%sIf you only want to use the canonical OTP repo, run `erln8 --clone default` before continuing%s\n", yellow(), color_reset());
  }
}

// write an ERLN8_CONFIG_FILE file into the cwd
// won't override an existing file
// unless the user specifies --force
void init_here(gchar* erlang) {
  GHashTable* erlangs = get_erlangs();
  gboolean has_erlang = g_hash_table_contains(erlangs, erlang);
  g_hash_table_destroy(erlangs);
  if(!has_erlang) {
    g_error("%s is not a configured version of Erlang\n", erlang);
  }
  GKeyFile* kf = g_key_file_new();
  g_key_file_set_string(kf,
                        "Config",
                        "Erlang",
                        erlang);
  GError* error = NULL;
  gchar* d = g_key_file_to_data (kf, NULL, &error);
  gchar* fn = "./" ERLN8_CONFIG_FILE;
  gboolean result = FALSE;
  if(!opt_force) {
    result = g_file_test(fn, G_FILE_TEST_EXISTS |
                         G_FILE_TEST_IS_REGULAR);
  }
  if(result) {
    g_error("Config already exists in this directory. Override with --force.\n");
  } else {
    GError* error2 = NULL;
    if(!g_file_set_contents(fn, d, -1, &error2)) {
      if (error2 != NULL) {
        g_error("Unable to write file %s: %s\n", fn, error2->message);
      } else {
        g_error("Unable to write file %s\n", fn);
      }
    } else {
      fprintf(stderr, "%sSaved " ERLN8_CONFIG_FILE " to %s%s\n", green(), fn, color_reset());
    }
  }
  g_key_file_free(kf);
}

// list installed version of Erlang
void list_erlangs() {
  GKeyFile* kf = g_key_file_new();
  GError* error = NULL;
  gchar* fn = get_configdir_file_name("config");
  if(g_key_file_load_from_file(kf, fn, G_KEY_FILE_NONE, &error)) {
    if (error != NULL) {
      g_error("Unable to read file: %s\n", error->message);
      //g_error_free(error); program exits, can't free
    }
    GError* keyerror = NULL;
    gchar** keys = g_key_file_get_keys(kf, "Erlangs", NULL, &keyerror);
    if (keyerror != NULL) {
      g_error("Unable to read Erlangs section from ~/" ERLN8_CONFIG_DIR "/config: %s\n", keyerror->message);
      //g_error_free(error);
    } else {
      gchar** it = keys;
      while(*it) {
        GError* valerror = NULL;
        gchar* v = g_key_file_get_string(kf, "Erlangs", *it, &valerror);
        printf("%s -> %s\n",*it++, v);
        g_free(v);
      }
    }
    g_strfreev(keys);
    g_key_file_free(kf);
  } else {
    g_error("Cannot read from ~/" ERLN8_CONFIG_DIR "/config\n");
  }
  g_free(fn);
}


// create ~/ERLN8_CONFIG_DIR, ~/ERLN8_CONFIG_DIR/config + related subdirs
// not atomic, if something blows up in here, the user will get
// a partial ~/ERLN8_CONFIG_DIR tree
void initialize() {
  if(check_home()) {
    g_warning("Configuration directory ~/" ERLN8_CONFIG_DIR " already exists\n");
    return;
  } else {
    //if(erl_on_path()) {
    //  g_warning("Erlang already exists on the current PATH\n");
    //}
    // create the top level config directory, then create all subdirs
    gchar* dirname = g_strconcat(homedir, "/" ERLN8_CONFIG_DIR "",(gchar*)0);
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

// search up the directory tree for a ERLN8_CONFIG_FILE to use
gchar* configcheck(char* d) {
  gchar* retval = NULL;
  gchar* f = g_strconcat(d, "/" ERLN8_CONFIG_FILE, NULL);
  GFile* gf = g_file_new_for_path(f);
  GFile* gd = g_file_new_for_path(d);
  if(g_file_query_exists(gf, NULL)) {
    gchar* cf = g_file_get_path(gf);
    retval = cf;
  } else {
    if(retval == NULL && g_file_has_parent(gd, NULL)) {
      GFile* parent = g_file_get_parent(gd);
      gchar* pp = g_file_get_path(parent);
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

// search up the directory tree for a SystemRoot to use
gchar* system_root_check(gchar* d) {
  gchar* retval = NULL;
  GFile* gd = g_file_new_for_path(d);
  GHashTable *roots = get_system_roots();
  if(roots == NULL) {
    return NULL;
  } else {
    gchar *p = (gchar*)g_hash_table_lookup(roots, d);
    if(p != NULL) {
      retval = strdup(p);
    }
  }

  if(roots != NULL) {
    g_hash_table_destroy(roots);
  }

  if(retval == NULL && g_file_has_parent(gd, NULL)) {
    GFile* parent = g_file_get_parent(gd);
    gchar* pp = g_file_get_path(parent);
    retval = system_root_check(pp);
    g_object_unref(parent);
    g_free(pp);
  }
  g_object_unref(gd);
  return retval;
}

// search up the directory tree for an ERLN8_CONFIG_FILE to use,
// starting from cwd
gchar* configcheckfromcwd() {
  gchar* d = getcwd(NULL, MAXPATHLEN);
  gchar* retval = configcheck(d);
  g_free(d);
  return retval;
}

// search up the directory tree for an ERLN8_CONFIG_FILE to use,
// starting from cwd
gchar* systemrootcheck_from_cwd() {
  gchar* d = getcwd(NULL, MAXPATHLEN);
  gchar* retval = system_root_check(d);
  g_free(d);
  return retval;
}

gchar* get_system_default() {
  GHashTable *e = get_erln8();
  gchar* d = (gchar*)g_hash_table_lookup(e, "system_default");
  // d can be NULL if the key doesn't exist etc.
  if(d != NULL) {
    gchar* ret = strdup(d);
    return ret;
  } else {
    g_hash_table_destroy(e);
    return NULL;
  }
}

// which version of erlang is configured for this particular
// branch of the dir tree
gchar* which_erlang() {
  gchar* cfgfile = configcheckfromcwd();
  if(cfgfile == NULL) {
    // check for a system root. 
    // if one doesn't exist, return the system_default
    gchar *sysroot = systemrootcheck_from_cwd();
    if(sysroot != NULL) {
      return sysroot;
    } else {
      return get_system_default();
    }
  } else {
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
  }
  return NULL;
}

// set a ~/ERLN8_CONFIG_DIR/config group/key value
// overwrites existing k/v's
gchar* set_config_kv(gchar* group, gchar* key, gchar* val) {
  gchar* cfgfile = get_configdir_file_name("config");
  GKeyFile* kf = g_key_file_new();
  GError* err = NULL;
  if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
    if(err != NULL) {
      fprintf (stderr,
               "Unable to load keyfile ~/" ERLN8_CONFIG_DIR "/config: %s\n",
               err->message);
      g_error_free(err);
    } else {
      fprintf(stderr, "Unable to load keyfile ~/" ERLN8_CONFIG_DIR "/config\n");
      // TODO: exit etc
    }
  } else {
    g_key_file_set_string(kf, group, key, val);
    GError* error = NULL;
    gchar* d = g_key_file_to_data (kf, NULL, &error);
    gchar* fn = get_configdir_file_name("config");
    g_debug("Writing to %s\n", fn);
    GError* error2 = NULL;
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


// set a ~/ERLN8_CONFIG_DIR/config group/key value
// overwrites existing k/v's
void rm_config_kv(gchar* group, gchar* key) {
  gchar* cfgfile = get_configdir_file_name("config");
  GKeyFile* kf = g_key_file_new();
  GError* err = NULL;
  if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
    if(err != NULL) {
      fprintf (stderr,
               "Unable to load keyfile ~/" ERLN8_CONFIG_DIR "/config: %s\n",
               err->message);
      g_error_free(err);
    } else {
      g_error("Unable to load keyfile ~/" ERLN8_CONFIG_DIR "/config\n");
    }
  } else {
    GError* error = NULL;
    g_key_file_remove_key(kf, group, key, &error);
    if(error != NULL) {
      g_error("Can't remove %s:%s from ~/" ERLN8_CONFIG_DIR "/config: %s\n",
              group, key, err->message);
    } else {
      error = NULL;
    }
    gchar* d = g_key_file_to_data (kf, NULL, &error);
    gchar* fn = get_configdir_file_name("config");
    g_debug("Writing to %s\n", fn);
    GError* error2 = NULL;
    if(!g_file_set_contents(fn, d, -1, &error2)) {
      if(error2 != NULL) {
        g_error("Error removing [%s] %s from config file: %s\n",
                group, key, error2->message);
      } else {
        g_error("Error removing [%s] %s from config file\n",
                group, key);
      }
    }
    g_free(fn);
    g_key_file_free(kf);
  }
  g_free(cfgfile);
}


void git_fetch(gchar* repo) {
  GHashTable* repos = get_repos();
  gboolean has_repo = g_hash_table_contains(repos, repo);
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
  gchar* fetchcmd = g_strconcat("cd ",
                               source_path,
                               " && git fetch --all",
                               NULL);
  int result = system(fetchcmd);
  if(result != 0) {
    g_error("Error fetching from repo %s\n", repo);
  }
  g_free(source_path);
  g_free(fetchcmd);
}


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

void setup_binaries(gchar* otpid) {
  GHashTable* erlangs = get_erlangs();
  gboolean has_erlang = g_hash_table_contains(erlangs, otpid);
  gchar* path0 = (gchar*)g_hash_table_lookup(erlangs, otpid);
  // path0 is freed when the hashtable is freed
  gchar* path = strdup(path0);
  g_hash_table_destroy(erlangs);
  if(!has_erlang) {
    g_error("%s doesn't appear to be linked. Did something go wrong with the build?\n", otpid);
  }
  gchar* genbins = g_strconcat("cd ", path, " && for i in `find -L . -perm -111 -type f | grep -v \"\\.so\" | grep -v \"\\.o\" | grep -v \"lib/erlang/bin\" | grep -v Install`; do  `ln -s -f $i $(basename $i)` ; done", NULL);
  g_debug("%s\n", genbins);
  if(!opt_dryrun) {
    int result = system(genbins);
    if(result != 0) {
      g_error("Error while generating symlinks using the following command: %s", genbins);
    }
  } else {
    printf("%s", genbins);
  }
  g_free(genbins);
  g_free(path);
}

// THIS FUNCTION NEEDS TO BE BROKEN UP INTO SMALLER PIECES!
void build_erlang(gchar* repo, gchar* tag, gchar* id, gchar* build_config) {
  // check to see if the ID has already been used
  GHashTable* otps = get_erlangs();
  gboolean has_otp = g_hash_table_contains(otps, id);
  g_hash_table_destroy(otps);
  if(has_otp) {
    g_error("A version of Erlang already exists with this id: %s\n", id);
  }
  // check to see if the repo exists
  GHashTable* repos   = get_repos();
  gboolean has_repo = g_hash_table_contains(repos, repo);
  g_hash_table_destroy(repos);
  if(!has_repo) {
    g_error("Unconfigured repo: %s\n", repo);
  }
  // check for a valid build config if one is specified
  GHashTable* configs = get_configs();
  if(build_config) {
    if(!g_hash_table_contains(configs, build_config)) {
      g_hash_table_destroy(configs);
      g_error("Unconfigured build config: %s\n", build_config);
    }
  } else {
    // check the default
    GHashTable* e8 = get_erln8();
    if(g_hash_table_contains(e8, "default_config")) {
      build_config = strdup((gchar*)g_hash_table_lookup(e8, "default_config"));
      printf("%sUsing default config %s%s\n", build_config, yellow(), color_reset());
    }
    g_hash_table_destroy(e8);
  }
  gchar pattern[] = "/tmp/erln8.buildXXXXXX";
  gchar* tmp = g_mkdtemp(pattern);
  g_debug("building in %s\n", tmp);
  gchar* output_root = get_config_subdir_file_name("otps",id);
  gchar* output_path = g_strconcat(output_root, "/dist", NULL);
  gchar* source_path = get_config_subdir_file_name("repos", repo);
  GTimeVal t;
  g_get_current_time(&t);
  gchar* ts = g_time_val_to_iso8601(&t);
  gchar* ld = g_strconcat("logs/build_", id, "_", ts, NULL);
  gchar* log_path    = get_configdir_file_name(ld);
  // check that the branch or tag exists in the specified repo
  gchar* check_obj = g_strconcat("cd ", source_path, "&& git show-ref ", tag, " > /dev/null", NULL);
  if(system(check_obj) != 0) {
    g_free(check_obj);
    g_error("branch or tag %s does not exist in %s Git repo\n",
            tag,
            repo);
  }
  g_free(check_obj);
  /// check that the repo has been cloned
  if(!g_file_test(source_path, G_FILE_TEST_EXISTS |
                  G_FILE_TEST_IS_REGULAR)) {
    g_error("Missing repo for %s, which should be in %s.\nDid you forget to `erln8 --clone <repo_name>?`\n", repo, source_path);
  }
  gchar* bc = NULL;
  gchar* env = NULL;
  if(build_config != NULL) {
    bc = (gchar*)g_hash_table_lookup(configs, build_config);
    // don't drop a NULL into the middle of the command string
    if(bc == NULL) {
      bc = "";
    } else {
      gchar* env_name = g_strconcat(build_config, "_env", NULL);
      env = (gchar*)g_hash_table_lookup(configs, env_name);
      if(env == NULL) {
        env = "";
      }
      g_free(env_name);
    }
  } else {
    if(env == NULL) {
      env = "";
    }
  }
  g_free(ld);
  g_debug("Output path = %s\n", output_path);
  g_debug("Source path = %s\n", source_path);
  g_debug("Log path = %s\n", log_path);
  printf("Building %s from tag/branch %s of repo %s\n", id, tag, repo);
  printf("Custom build config: %s\n", bc);
  printf("Custom build env: %s\n", env);
  printf("Build log: %s\n", log_path);
  gchar* buildcmd0 = g_strconcat(env,
                                " cd ",
                                source_path,
                                " && git archive ",
                                tag,
                                " | (cd ", tmp, "; tar -f - -x)", NULL);
  gchar* buildcmd1 = g_strconcat(env, " cd ", tmp,
                                " && ./otp_build autoconf > ", log_path, " 2>&1", NULL);
  gchar* buildcmd2 = g_strconcat(env, " cd ", tmp,
                                " && ./configure --prefix=", output_path," ",
                                bc == NULL ? "" : bc,
                                " >> ", log_path, " 2>&1",
                                NULL);
  gchar* buildcmd3 = g_strconcat(env, " cd ", tmp,
                                " && make >> ", log_path,  " 2>&1", NULL);

  gchar* buildcmd4 = g_strconcat(env, " cd ", tmp,
                                " && make install >> ", log_path,  " 2>&1", NULL);

  gchar* buildcmd5 = g_strconcat(env, " cd ", tmp,
                                " && make install-docs >> ", log_path, " 2>&1", NULL);
  gchar* build_cmds[] = {
    buildcmd0,
    buildcmd1,
    buildcmd2,
    buildcmd3,
    buildcmd4,
    buildcmd5,
    NULL
  };
  int result = 0;
  int i = 0;
  for(i = 0; i < step_count; i++) {
    show_build_progress(i, result);
    if(result != 0) {
      g_debug("STATUS = %d\n", result);
      printf("Here are the last 10 lines of the log file:\n");
      gchar* tail = g_strconcat("tail -10 ", log_path, NULL);
      int tailresult = system(tail);
      if(tailresult != 0) {
        g_error("Cannot run tail -10 on %s\n", log_path);
      }
      g_free(tail);
      printf("---------------------------------------------------------\n");
      g_error("Build error, please check the build logs for more details\n");
    }
    g_debug("running %s\n", build_cmds[i]);
    if(!opt_dryrun) {
      result = system(build_cmds[i]);
    } else {
      result = 0;
      printf("%s\n", build_cmds[i]);
    }
  }
  show_build_progress(step_count, result);
  printf("Registering Erlang installation\n");
  if(!opt_dryrun) {
    set_config_kv("Erlangs", id, output_root);
  }
  printf("Generating links\n");
  setup_binaries(id);
  printf("%sBuild complete%s\n", green(), color_reset() );
  g_free(buildcmd0);
  g_free(buildcmd1);
  g_free(buildcmd2);
  g_free(buildcmd3);
  g_free(buildcmd4);
  g_free(log_path);
  g_free(source_path);
  g_free(output_path);
  g_free(output_root);
  // destroy close to the end so the string isn't freed before it's used
  g_hash_table_destroy(configs);
}


gchar* get_bin(gchar* otpid, gchar* cmd) {
  GHashTable* erlangs = get_erlangs();
  gboolean has_erlang = g_hash_table_contains(erlangs, otpid);
  gchar* path0 = (gchar*)g_hash_table_lookup(erlangs, otpid);
  gchar* path = strdup(path0);
  g_hash_table_destroy(erlangs);
  if(!has_erlang) {
    g_error("%s doesn't appear to be linked. Did something go wrong with the build?\n", otpid);
  }
  gchar* cmdpath = g_strconcat(path, "/",  cmd, NULL);
  return cmdpath;
}

void doclone() {
  GHashTable* repos = get_repos();
  gchar* repo = g_hash_table_lookup(repos, opt_clone);
  if(repo == NULL) {
    g_hash_table_destroy(repos);
    g_error("Unknown repository %s\n", opt_clone);
  } else {
    gchar* path = get_config_subdir_file_name("repos",opt_clone);
    gchar* cmd = g_strconcat("git clone ", repo, " ", path, NULL);
    int result = system(cmd);
    if(result != 0) {
      g_error("Cannot clone %s\n", repo);
    }
    g_hash_table_destroy(repos);
    g_free(cmd);
    g_free(path);
  }
}

void dorepos() {
  GHashTable* repos = get_repos();
  GList* keys = g_hash_table_get_keys(repos);
  g_list_foreach(keys, e8_print, NULL);
  g_list_free(keys);
  g_hash_table_destroy(repos);
}

void doconfigs() {
  GHashTable* configs = get_configs();
  GList* keys = g_hash_table_get_keys(configs);
  g_list_foreach(keys, e8_print, NULL);
  g_list_free(keys);
  g_hash_table_destroy(configs);
}

void dobuild() {
  gchar* repo = NULL;
  if(opt_repo == NULL) {
    printf("%sRepo not specified, using default%s\n", yellow(), color_reset());
    repo = "default";
  } else {
    repo = opt_repo;
  }
  if(repo == NULL) {
    g_error("build repo not specified\n");
  }
  if(opt_tag == NULL) {
    g_error("build tag not specified\n");
  }
  if(opt_id == NULL) {
    g_error("build id not specified\n");
  }
  // opt_config is optional
  g_debug("repo:%s\n", repo);
  g_debug("tag :%s\n", opt_tag);
  g_debug("id  :%s\n", opt_id);
  g_debug("cfg :%s\n", opt_config);
  build_erlang(repo, opt_tag, opt_id, opt_config);
}

void dolink() {
  if(!opt_id) {
    g_error("Please specify --id when linking\n");
  }
  if(!opt_link) {
    g_error("An absolute path must be specified with --link\n");
  }
  if(opt_link[0] != '/') {
    g_error("An absolute path must be specified when linking\n");
  }
  GHashTable* erlangs = get_erlangs();
  gboolean result = g_hash_table_contains(erlangs, opt_id);
  g_hash_table_destroy(erlangs);
  if(result) {
    g_error("An installation of Erlang is already referenced by ID %s\n", opt_id);
  }
  gchar* erlpath = g_strconcat(opt_link, "/bin/erl", NULL);
  g_debug("checking for %s\n", erlpath);
  gboolean exists = g_file_test(erlpath,
                                G_FILE_TEST_EXISTS |
                                G_FILE_TEST_IS_REGULAR);
  g_free(erlpath);
  if(!exists) {
    g_error("Can't link to an empty Erlang installation\n");
  }
  gchar* output_root = get_config_subdir_file_name("otps", opt_id);
  gchar* output_path = g_strconcat(output_root, "/dist", NULL);
  if(g_mkdir_with_parents(output_root, 0755) == -1) {
    g_error("Cannot create OTP installation directory %s\n", output_path);
  }
  set_config_kv("Erlangs", opt_id, output_root);
  gchar* cmd = g_strconcat("ln -s -f ", opt_link, " ", output_path, NULL);
  result = system(cmd);
  g_free(cmd);
  if(result != 0) {
    g_error("Error linking to an existing Erlang installation\n");
  } else {
    setup_binaries(opt_id);
  }
}

void dounlink() {
  if(!opt_id) {
    g_error("Please specify --id when unlinking\n");
  }
  GHashTable* erlangs = get_erlangs();
  gboolean result = g_hash_table_contains(erlangs, opt_id);
  g_hash_table_destroy(erlangs);
  if(!result) {
    g_error("Can't remove %s, it doesn't exist\n", opt_id);
  }
  gchar* output_root = get_config_subdir_file_name("otps", opt_id);
  gchar* output_path = g_strconcat(output_root, "/dist", NULL);
  if(!g_file_test(output_path, G_FILE_TEST_IS_SYMLINK)) {
      g_error("%s isn't a linked version of Erlang\n", opt_id);
  }
  gchar* cmd = g_strconcat("rm ", output_path, " && rm -rf ", output_root, NULL);
  g_debug("%s\n", cmd);
  rm_config_kv("Erlangs", opt_id);
  result = system(cmd);
  if(result != 0) {
    g_error("Error unlinking an existing Erlang installation\n");
  }
}

void display_latest_quickstart() {
  gchar* repo = "default";
  GHashTable* repos = get_repos();
  gboolean has_repo = g_hash_table_contains(repos, repo);
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
  // too much color? I'd like this message to stand out
  printf("%sDetected latest Erlang/OTP tag: %s\n", blue(), green());
  gchar* fetchcmd = g_strconcat("cd ",
                               source_path,
                               " && git describe --abbrev=0 --tags",
                               NULL);
  int result = system(fetchcmd);
  if(result != 0) {
    g_error("Error fetching from repo %s\n", repo);
  }
  printf("%s\n", color_reset());
  g_free(source_path);
  g_free(fetchcmd);
}


// if not executing one of the erlang commands
// then process erln8 options etc
int erln8(int argc, gchar* argv[]) {
  // TODO: think about getting erlangs, configs, repos in one go
  //       instead of for each option. meh, maybe I don't care.
  GError* error = NULL;
  GOptionContext* context;
  context = g_option_context_new("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_error("erln8 option parsing failed: %s\n", error->message);
  }
  g_debug("argv[0] = [%s]\n",argv[0]);
  g_debug("opt_id = %s\n", opt_id);
  if(opt_quickstart) {
    initialize();
    opt_clone = "default";
    doclone();
    opt_repo = "default";
    opt_config = "default";
    // detect the latest git TAG from the OTP repo
    // I hope this works... ;-)
    opt_tag = "`git describe --abbrev=0 --tags`";
    opt_id = "quickstart_build";
    display_latest_quickstart();
    dobuild();
    return 0;
  }
  if(opt_init_erln8) {
    initialize();
    return 0;
  } else {
    if(!check_home()) {
      g_error("Please initialize erln8 with --init\n");
    }
  }
  GHashTable* runtime_options = get_erln8();
  gchar* use_color = (gchar*)g_hash_table_lookup(runtime_options, "color");
  if(g_strcmp0(use_color, "true") == 0) {
    opt_color = TRUE;
  } else {
    opt_color = FALSE;
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
    doclone();
    return 0;
  }
  if(opt_repos) {
    dorepos();
    return 0 ;
  }
  if(opt_configs) {
    doconfigs();
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
    dobuild();
    return 0;
  }
  if(opt_link) {
    dolink();
    return 0;
  }
  if(opt_unlink) {
    dounlink();
    return 0;
  }
  if(opt_buildable) {
    git_allbuildable();
    return 0;
  }
  if(opt_show) {
    gchar* erl = which_erlang();
    if(erl != NULL) {
      printf("%s\n", erl);
      g_free(erl);
      return 0;
    }
  }
  if(opt_prompt) {
    gchar* erl = which_erlang();
    if(erl != NULL) {
      printf("%s", erl);
      g_free(erl);
    } else {
      printf("erln8 error");
    }
    return 0;
  }
  printf("\nerln8: the sneaky Erlang version manager\n");
  printf("(c) 2013 Dave Parfitt\n");
  printf("Licensed under the Apache License, Version 2.0\n\n");
  printf("%s\n", g_option_context_get_help(context, TRUE, NULL));
  return 0;
}

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
  homedir = g_getenv("ERLN8_HOME");
  if(homedir == NULL) {
    homedir = g_get_home_dir();
  } else {
    // builds will fail if ERLN8_HOME is not an absolute path
    if(homedir[0] != '/') {
      g_error("ERLN8_HOME must be an absolute path\n");
    }
  }
  g_debug("home directory = %s\n", homedir);
  gchar* basename = g_path_get_basename(argv[0]);
  g_debug("basename = %s\n", basename);
  if((!strcmp(basename, "erln8")) || (!strcmp(basename, "./erln8"))) {
    erln8(argc, argv);
    g_free(basename);
  } else {
    gchar* erl = which_erlang();
    if(erl == NULL) {
      g_message("Can't find an " ERLN8_CONFIG_FILE " file to use\n");
      g_error("No " ERLN8_CONFIG_FILE " file\n");
    }
    GHashTable* erlangs = get_erlangs();
    GHashTable* runtime_options = get_erln8();
    gchar* path = g_hash_table_lookup(erlangs, erl);
    if(path == NULL) {
      g_hash_table_destroy(erlangs);
      g_hash_table_destroy(runtime_options);
      g_error("Version of Erlang (%s) isn't configured in erln8\n",
              erl);
    }
    gchar* use_color = (gchar*)g_hash_table_lookup(runtime_options, "color");
    if(g_strcmp0(use_color, "true") == 0) {
      opt_color = TRUE;
    } else {
      opt_color = FALSE;
    }
    gchar* use_banner = (gchar*)g_hash_table_lookup(runtime_options, "banner");
    if(g_strcmp0(use_banner, "true") == 0) {
      opt_banner = TRUE;
    } else {
      opt_banner = FALSE;
    }
    gchar* s = get_bin(erl, basename);
    g_debug("%s\n",s);
    gboolean result = g_file_test(s,
                                  G_FILE_TEST_EXISTS |
                                  G_FILE_TEST_IS_REGULAR);
    g_free(basename);
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

