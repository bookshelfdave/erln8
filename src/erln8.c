#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/param.h>

// GIO stuff
#include <glib-object.h>
#include <gio/gio.h>
#include <sys/param.h>

#include <errno.h>
/*
 * TODO:
 *   free all GErrors (and... everything else)
 *   error checking for all calls
 *   g_strfreev(keys);
 *   don't hardcode my paths in the default config :-)
 */

#define G_LOG_DOMAIN    ((gchar*) 0)
#define SOURCES "sources"
#define MD5     "MD5"
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

static const gchar* homedir;
//static unsigned int last_pcnt = 0;

static GOptionEntry entries[] =
{
  { "init", 0, 0, G_OPTION_ARG_NONE, &opt_init_erln8, "Initialize Erln8", NULL },
  { "use", 0, 0, G_OPTION_ARG_STRING, &opt_use, "Setup Erlang version in cwd", NULL },
  { "list", 0, 0, G_OPTION_ARG_NONE, &opt_list, "List available Erlang installations", NULL },
  { "clone", 'c', 0, G_OPTION_ARG_STRING, &opt_clone, "Clone source repos", NULL },
  //{ "fetch", 'f', 0, G_OPTION_ARG_NONE, &opt_fetch, "Update source repos", NULL },


  { "build", 0, 0, G_OPTION_ARG_NONE, &opt_build, "Build a specific version of OTP from source", NULL },
      { "repo", 0, 0, G_OPTION_ARG_STRING, &opt_repo, "Specifies repo name to build from", NULL },
      { "tag", 0, 0, G_OPTION_ARG_STRING, &opt_tag, "Specifies repo branch/tag to build from", NULL },
      { "id", 0, 0, G_OPTION_ARG_STRING, &opt_id, "A user assigned name for ", NULL },
      { "config", 0, 0, G_OPTION_ARG_STRING, &opt_config, "Build configuration", NULL },

  { "show", 0, 0, G_OPTION_ARG_NONE, &opt_show, "Show the configured version of Erlang", NULL },
  { "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug, "Debug Erln8", NULL },
  //{ "no-color", 'N', 0, G_OPTION_ARG_NONE, &opt_color, "Don't use color output", NULL },
  //{ "buildable-otps", 'o', 0, G_OPTION_ARG_NONE, &opt_buildable, "List tags to build from configured source repos", NULL },
  { NULL }
};

void erln8_log( const gchar *log_domain,
                GLogLevelFlags log_level,
                const gchar *message,
                gpointer user_data ) {
  if(opt_debug) {
    printf("%s",message);
  }
  return;
}

// TODO: move glib logging
// also, I always hated the critical output from glib
void erln8_error(char *msg) {
  printf("ERROR: %s\n", msg);
}

void erln8_error_and_exit(char *msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(-1);
}



gboolean erl_on_path() {
  gchar *out;
  gchar *err;
  gint   status;
  GError *error;
  g_spawn_command_line_sync ("which erl", &out, &err, &status, &error);
  if(!status) {
    return 1;
  } else {
    return 0;
  }
}

char* load_config() {
  }

gchar* get_configdir_file_name(char* filename) {
  gchar *configfilename = g_strconcat(homedir, "/.erln8.d/", filename, (char*)0);
  return configfilename;
}

gchar* get_config_subdir_file_name(char *subdir, char* filename) {
  gchar *configfilename = g_strconcat(homedir, "/.erln8.d/", subdir, "/", filename, (char*)0);
  return configfilename;
}

gboolean check_home() {
  gchar *configdir = g_strconcat(homedir, "/.erln8.d", (char*)0);
  g_debug("Checking config dir %s\n", configdir);
  //g_path_get_basename ()
  gboolean result = g_file_test(configdir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
  free(configdir);
  return result;
}

void mk_config_subdir(char *subdir) {
  gchar* dirname = g_strconcat(homedir, "/.erln8.d/", subdir, (char*)0);
  g_debug("Creating %s\n", dirname);
  if(g_mkdir(dirname, S_IRWXU)) {
    g_free(dirname);
    erln8_error_and_exit("Can't create directory");
    return;
  } else {
    g_free(dirname);
  }
}

void init_main_config() {
  GKeyFile *kf = g_key_file_new();
  g_key_file_set_string(kf,
                        "Repos",
                        "default",
                        "https://github.com/erlang/otp.git");

  g_key_file_set_string(kf,
                        "Repos",
                        "basho",
                        "https://github.com/basho/otp.git");


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

  g_key_file_set_string(kf,
                        "Erlangs",
                        "R15B01p",
                        "/Users/dparfitt/erlang_R15B01p");

  g_key_file_set_string(kf,
                        "Erlangs",
                        "R15B01",
                        "/Users/dparfitt/erlang_R15B01");

  g_key_file_set_string(kf,
                        "Erlangs",
                        "R16B02",
                        "/Users/dparfitt/erlang-R16B02");

  GError *error;
  gchar* d = g_key_file_to_data (kf, NULL, &error);
  gchar* fn = get_configdir_file_name("config");
  printf("Writing to %s\n", fn);  
  GError *error2;
  if(!g_file_set_contents(fn, d, -1, &error2)) {
    printf("Error writing config file :-(\n");
  }
  free(fn);
  g_key_file_free(kf);
}


void init_here(char* erlang) {
  GKeyFile *kf = g_key_file_new();
  g_key_file_set_string(kf,
                        "Config",
                        "Erlang",
                        erlang);

  GError *error = NULL;
  gchar* d = g_key_file_to_data (kf, NULL, &error);
  gchar* fn = "./erln8.config";
  printf("Writing to %s\n", fn);  

  GError *error2 = NULL;
  if(!g_file_set_contents(fn, d, -1, &error2)) {
    printf("Error writing config file :-(\n");
    if (error2 != NULL) {
      fprintf (stderr, "Unable to read file: %s\n", error2->message);
    }
  }
  // TODO
  //g_key_file_free(kf);
}


void list_erlangs() {
  GKeyFile *kf = g_key_file_new();
  GError *error;
  GError *error2;
  gsize keycount;
  gchar* fn = get_configdir_file_name("config");
  if(g_key_file_load_from_file(kf, fn, G_KEY_FILE_NONE, &error)) {
    printf("Available Erlang installations:\n");
    gchar** keys = g_key_file_get_keys(kf, "Erlangs", &keycount, &error2);
    int i = 0;
    for(i = 0; i < keycount; i++) {
      printf("  %s\n",*keys++);
    }
    // TODO
    //g_strfreev(keys);
  }
  // TODO: free error
  free(fn);
  g_key_file_free(kf);
}


void initialize() {
  if(check_home()) {
    erln8_error_and_exit("Configuration directory ~/.erln8.d already exists");
  //} //else if(erl_on_path()) {
    //erln8_error_and_exit("Erlang already exists on the current PATH");
  } else {
    // create the top level config directory, then create all subdirs
    gchar* dirname = g_strconcat(homedir, "/.erln8.d",(char*)0);
    g_debug("Creating %s\n", dirname);
    if(g_mkdir(dirname, S_IRWXU)) {
      erln8_error_and_exit("Can't create directory");
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

char* configcheck(char *d) {
  char *retval = NULL;
  char *f = g_strconcat(d, "/erln8.config", NULL);
  GFile *gf = g_file_new_for_path(f);
  GFile *gd = g_file_new_for_path(d);

  if(g_file_query_exists(gf, NULL)) {
      char *cf = g_file_get_path(gf);
      //g_free(cf);
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

char* configcheckfromcwd() {
  char *d = getcwd(NULL, MAXPATHLEN);
  char *retval = configcheck(d);
  free(d);
  return retval;
}


// TODO: this function leaks badly!
char* which_erlang() {
  char* cfgfile = configcheckfromcwd();
  // TODO: free
  if(g_file_test(cfgfile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
    GKeyFile* kf = g_key_file_new();
    GError* err;
    // TODO: free kf
    // TODO: free err
    gboolean b = g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err);
    if(!g_key_file_has_group(kf, "Config")) {
      erln8_error_and_exit("erln8 Config group not defined in erln8.config\n");
      return NULL;
    } else {
      if(g_key_file_has_key(kf, "Config", "Erlang", &err)) {
        gchar* erlversion = g_key_file_get_string(kf, "Config", "Erlang", &err);
        // THIS VALUE MUST BE FREED
        return erlversion;
      } else {
        erln8_error_and_exit("Missing Erlang | version\n");
        return NULL;
      }
    }
  } else {
    erln8_error_and_exit("Config file does not exist\n");
    return NULL;
  }
}

char *get_config_kv(char *group, char *key) {
  gchar* cfgfile = get_configdir_file_name("config");
  GKeyFile* kf = g_key_file_new();
  GError* err;
  gchar* val = NULL;

  // TODO: free kf
  // TODO: free err
  if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
      if(err != NULL) {
        fprintf (stderr,
            "Unable to load keyfile ~/.erln8.d/config: %s\n",
            group,
            key,
            err->message);
        g_error_free(err);
      } else {
         fprintf(stderr, "Unable to load keyfile ~/.erln8.d/config\n");
         // TODO: exit etc
      }
  } else {
    GError *kferr = NULL;
    if(g_key_file_has_key(kf, group, key, &kferr)) {
       val = g_key_file_get_string(kf, group, key, &err);
    } else {
      if(kferr != NULL) {
        fprintf (stderr,
            "Unable to read group %s, key %s from ~/.erln8.d/config: %s\n",
            group,
            key,
            kferr->message);
        g_error_free(kferr);
      } else {
        fprintf(stderr, "Unable to read group %s, key %s in ~/.erln8.d/config\n", group, key);
      }
    }
  }

  free(cfgfile);
  return val;
}


char *set_config_kv(char *group, char *key, char *val) {
  gchar* cfgfile = get_configdir_file_name("config");
  GKeyFile* kf = g_key_file_new();
  GError* err = NULL;

  // TODO: free kf
  // TODO: free err
  if(!g_key_file_load_from_file(kf, cfgfile, G_KEY_FILE_NONE, &err)) {
      if(err != NULL) {
        fprintf (stderr,
            "Unable to load keyfile ~/.erln8.d/config: %s\n",
            group,
            key,
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
    printf("Writing to %s\n", fn);
    GError *error2 = NULL;
    if(!g_file_set_contents(fn, d, -1, &error2)) {
      printf("Error writing config file :-(\n");
    }
    free(fn);
    g_key_file_free(kf);
  }

  free(cfgfile);
  return val;
}




// TODO: free strings!
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
  free(ld);

  printf("Output path = %s\n", output_path);
  printf("Source path = %s\n", source_path);
  printf("Log path = %s\n", log_path);

  printf("Copying source...\n");
  char *copycmd = g_strconcat("cd ", source_path, " && git archive ", tag, " | (cd ", tmp, "; tar x)", NULL);
  system(copycmd);
  free(copycmd);
  printf("Building source...\n");
  char *buildcmd = g_strconcat("(cd ", tmp,
      " && ./otp_build autoconf && ./configure --prefix=",
      output_path," && make && make install) | tee ",log_path, NULL);
  printf("%s\n",buildcmd);
  system(buildcmd);

  set_config_kv("Erlangs", id, output_path);
  free(buildcmd);
  free(log_path);
  free(source_path);
  free(output_path);

 }

int erln8(int argc, char* argv[]) {
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_error("option parsing failed: %s\n", error->message);
  }

  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,  erln8_log, NULL);

  g_debug("argv[0] = [%s]\n",argv[0]);

  if(opt_init_erln8) {
    initialize();
  } else {
    if(!check_home()) {
      erln8_error_and_exit("Please initialize erln8 with --init");
    }
  }

  if(opt_use) {
    init_here(opt_use);
  }

  if(opt_list) {
   list_erlangs();
  }

  if(opt_clone) {
    gchar* repo = get_config_kv("Repos", opt_clone);
    gchar* path = get_config_subdir_file_name("repos",opt_clone);
    if(repo == NULL || path == NULL) {
      erln8_error_and_exit("Repository not configured\n");
    } else {
      gchar* cmd = g_strconcat("git clone ", repo, " ", path, NULL);
      system(cmd);
      free(cmd);
    }
  }

  if(opt_fetch) {
    printf("Not implemented\n");
  }

  if(opt_build) {
    printf("Not implemented\n");
    gchar *repo = NULL;
    if(opt_repo == NULL) {
      printf("Repo not specified, using default\n");
      repo = "default";
    } else {
      repo = opt_repo;
    }
    if(repo == NULL) {
      erln8_error_and_exit("build repo not specified");
    }
    if(opt_tag == NULL) {
      erln8_error_and_exit("build tag not specified");
    }
    if(opt_id == NULL) {
      erln8_error_and_exit("build id not specified");
    }
    // opt_config is optional
    printf("repo:%s\n", repo);
    printf("tag :%s\n", opt_tag);
    printf("id  :%s\n", opt_id);
    printf("cfg :%s\n", opt_config);

    build_erlang(repo, opt_tag, opt_id, opt_config);
  }


  if(opt_buildable) {
    printf("Not implemented\n");
  }

  if(opt_show) {
    char* erl = which_erlang();
    printf("%s", erl);
    free(erl);
  }
  return 0;
}

#define BRIGHT 1
#define RED 31
#define BG_BLACK 40


int main(int argc, char* argv[]) {
  //printf("%c[%d;%d;%dmerln8", 0x1B, BRIGHT,RED,BG_BLACK);
  printf("erln8 v0.1\n");
  // compiler will whine about it being deprecated, but taking it out
  // blows things up
  // used for GIO
  g_type_init();

  homedir = g_get_home_dir();
  g_debug("home directory = %s\n", homedir);

  // erln8 --build --repo default --tag OTP_R16B02 --id R16B02



  if((!strcmp(argv[0], "erln8")) || (!strcmp(argv[0], "./erln8"))) {
    erln8(argc, argv);
  } else {
    char *erl = which_erlang();
    char *path = get_config_kv("Erlangs", erl);
    g_debug("Using erlang %s\n", erl);
    g_debug("  ->%s\n", path);

    char *s = g_strconcat(path, "/bin/", argv[0], (char*)0);
    g_debug("%s\n",s);
    // can't free s
    execv(s, argv);
  }

 }
