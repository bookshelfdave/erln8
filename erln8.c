#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/param.h>
#include <glib/gstdio.h>

// GIO stuff
#include <glib-object.h>
#include <gio/gio.h>
#include <sys/param.h>

#include <errno.h>
/*
 * TODO:
 *   error checking for all calls
 *   g_strfreev(keys);
 *   don't hardcode my paths in the default config :-)
 */


/* memory management note:
   Since this program either exits or calls exec, there may be some
   pointers that aren't freed before calling g_error. I guess I don't
   really care about these. If you feel strongly about this,
   please fix and submit a pull request on Github.
*/

#define G_LOG_DOMAIN    ((gchar*) 0)

#define BRIGHT 1
#define RED 31
#define BG_BLACK 40


static gboolean opt_init_erln8 = FALSE;
static gboolean opt_debug      = FALSE;
static gchar*   opt_use        = NULL;
static gboolean opt_list       = FALSE;
static gboolean opt_buildable  = FALSE;
static gboolean opt_fetch      = FALSE;
static gboolean opt_build      = FALSE;
static gboolean opt_show       = FALSE;
static gchar*   opt_clone      = NULL;
//static gboolean opt_color      = TRUE;

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

  //{ "no-color", 'N', 0, G_OPTION_ARG_NONE, &opt_color, "Don't use color output", NULL },
  //{ "buildable", 'o', 0, G_OPTION_ARG_NONE, &opt_buildable, "List tags to build from configured source repos", NULL },
  { "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug,
    "Debug Erln8", NULL },
  { NULL }
};


void erln8_log( const gchar *log_domain,
                GLogLevelFlags log_level,
                const gchar *message,
                gpointer user_data ) {
  switch(log_level & G_LOG_LEVEL_MASK) {
    case G_LOG_FLAG_RECURSION:
    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_LEVEL_ERROR:
        fprintf(stderr, "ERROR: %s",message);
        exit(-1);
        break;
    case G_LOG_LEVEL_WARNING:
        fprintf(stderr, "WARNING: %s",message);
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
        fprintf(stderr, "UNHANDLED: %s",message);
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


void git_init_config() {
  system("echo \"logs/\notps/\nrepos/\n\" > ~/.erln8.d/.gitignore ");
  system("git init ~/.erln8.d/");
  system("git add ~/.erln8.d/config");
  system("git commit -am \"erln8 init\"");
}


// generate the initial ~/.erln8.d/config file with some
// default settings
// TODO: more settings, Linux, FreeBSD etc
void init_main_config() {
  GKeyFile *kf = g_key_file_new();
  g_key_file_set_string(kf,
                        "Repos",
                        "default",
                        "https://github.com/erlang/otp.git");

/*
  g_key_file_set_string(kf,
                        "Repos",
                        "basho",
                        "https://github.com/basho/otp.git");
*/

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
        printf("  %s\n",*it++);
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
    if(erl_on_path()) {
      g_warning("Erlang already exists on the current PATH\n");
    }
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
    g_error("Config file does not exist\n");
    return NULL;
  }
}

// get a group/key value from ~/.erln8.d/config
char *get_config_kv(char *group, char *key) {
  gchar* cfgfile = get_configdir_file_name("config");
  GKeyFile* kf = g_key_file_new();
  GError* err = NULL;
  gchar* val = NULL;

  if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
      if(err != NULL) {
            g_error("Unable to load %s:%s from keyfile ~/.erln8.d/config: %s\n",
            group, key, err->message);
        //g_error_free(err);
      } else {
         g_error("Unable to load keyfile ~/.erln8.d/config\n");
      }
  } else {
    GError *kferr = NULL;
    if(g_key_file_has_key(kf, group, key, &kferr)) {
       val = g_key_file_get_string(kf, group, key, &err);
       if(err != NULL) {
            g_error("Unable to load %s:%s from keyfile ~/.erln8.d/config: %s\n",
            group, key,
            err->message);
          //g_error_free(err);
       }
    } else {
      if(kferr != NULL) {
        g_error("Unable to read group %s, key %s from ~/.erln8.d/config: %s\n",
            group,
            key,
            kferr->message);
        //g_error_free(kferr);
      } else {
        g_error("Unable to read group %s, key %s in ~/.erln8.d/config\n", group, key);
      }
    }
  }
  g_free(cfgfile);
  g_key_file_free(kf);
  return val;
}

// list all keys for a ~/.erln8.d/config group
char **get_config_keys(char *group) {
  gchar* cfgfile = get_configdir_file_name("config");
  GKeyFile* kf = g_key_file_new();
  GError* err = NULL;
  gchar** val = NULL;

  if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
      if(err != NULL) {
        g_error("Unable to load keyfile ~/.erln8.d/config: %s\n",
            err->message);
        //g_error_free(err);
      } else {
         g_error("Unable to load keyfile ~/.erln8.d/config\n");
      }
  } else {
    GError *kferr = NULL;
    val = g_key_file_get_keys(kf, group, NULL, &kferr);
    if(kferr != NULL) {
       g_error("Unable to get key list for %s:%s\n", group, kferr->message);
        //g_error_free(kferr);
    }
  }

  g_free(cfgfile);
  g_key_file_free(kf);
  return val;
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

void build_erlang(char *repo, char *tag, char *id, char *build_config) {
  // TODO:
  // make "tee" optional? -q: quiet build
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

  gchar* bc = NULL;
  if(build_config != NULL) {
    bc = get_config_kv("Configs", build_config);
  }

  g_free(ld);

  g_debug("Output path = %s\n", output_path);
  g_debug("Source path = %s\n", source_path);
  g_debug("Log path = %s\n", log_path);

  printf("Copying source...\n");
  char *copycmd = g_strconcat("cd ", source_path, " && git archive ", tag, " | (cd ", tmp, "; tar x)", NULL);
  g_debug("%s",copycmd);
  system(copycmd);
  g_free(copycmd);
  printf("Building source [%s]...\n", log_path);
  char *buildcmd = g_strconcat("(cd ", tmp,
      " && ./otp_build autoconf && ./configure --prefix=",
      output_path," ", bc == NULL ? "" : bc, " && make && make install) | tee ", log_path, NULL);
  g_debug("%s\n",buildcmd);
  system(buildcmd);
  set_config_kv("Erlangs", id, output_path);
  g_free(buildcmd);
  g_free(log_path);
  g_free(source_path);
  g_free(output_path);
  g_free(bc);
 }

// if not executing one of the erlang commands
// then process erln8 options etc
int erln8(int argc, char* argv[]) {
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_error("option parsing failed: %s\n", error->message);
  }

  g_debug("argv[0] = [%s]\n",argv[0]);

  if(opt_init_erln8) {
    initialize();
    return 0;
  } else {
    if(!check_home()) {
      g_error("Please initialize erln8 with --init");
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
    gchar* repo = get_config_kv("Repos", opt_clone);
    gchar* path = get_config_subdir_file_name("repos",opt_clone);
    if(repo == NULL || path == NULL) {
      g_error("Unknown repository %s\n", repo);
    } else {
      gchar* cmd = g_strconcat("git clone ", repo, " ", path, NULL);
      system(cmd);
      g_free(cmd);
    }
    return 0;
  }

  if(opt_repos) {
    gchar **repos = get_config_keys("Repos");
    gchar **it = repos;
    while(*it) {
      printf("%s\n", *it++);
    }
    g_strfreev(repos);
    return 0 ;
  }

  if(opt_configs) {
    gchar **configs = get_config_keys("Configs");
    gchar **it = configs;
    while(*it) {
      printf("%s\n", *it++);
    }
    g_strfreev(configs);
    return 0;
  }

  if(opt_fetch) {
    printf("Not implemented\n");
    return 0;
  }

  if(opt_build) {
    gchar *repo = NULL;
    if(opt_repo == NULL) {
      printf("Repo not specified, using default\n");
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
    printf("Not implemented\n");
    return 0;
  }

  if(opt_show) {
    char* erl = which_erlang();
    printf("%s\n", erl);
    g_free(erl);
    return 0;
  }

  printf("\nerln8: the sneaky Erlang version manager\n");
  printf("(c) 2013 Dave Parfitt\n");
  printf("Licensed under the Apache License, Version 2.0\n");
  printf("For more information, try erln8 --help\n\n");
  return 0;
}



int main(int argc, char* argv[]) {


  git_init_config();
   exit(0);
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

  if((!strcmp(argv[0], "erln8")) || (!strcmp(argv[0], "./erln8"))) {
    erln8(argc, argv);
  } else {
    char *erl = which_erlang();
    if(erl == NULL) {
     g_error("Can't find an erln8.config file to use\n");
    }
    char *path = get_config_kv("Erlangs", erl);
    g_debug("Using erlang %s\n", erl);
    //g_debug("  ->%s\n", path);

    char *s = g_strconcat(path, "/bin/", argv[0], (char*)0);
    g_debug("%s\n",s);
    // can't free s
    execv(s, argv);
  }
  return 0;
 }
