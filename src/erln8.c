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


#define G_LOG_DOMAIN    ((gchar*) 0)
#define SOURCES "sources"
#define MD5     "MD5"
static gboolean init_erln8 = FALSE;
static gboolean debug      = FALSE;
static const gchar* homedir;
static gchar** dl_otps = NULL;
static unsigned int last_pcnt = 0;

static GOptionEntry entries[] =
{
  { "init", 'i', 0, G_OPTION_ARG_NONE, &init_erln8, "Initialize Erln8", NULL },
  { "download", 'd', 0, G_OPTION_ARG_STRING_ARRAY, &dl_otps, "Initialize Erln8", NULL },
  { "debug", 'D', 0, G_OPTION_ARG_NONE, &debug, "Debug Erln8", NULL },
  { NULL }
};

void erln8_log( const gchar *log_domain,
                GLogLevelFlags log_level,
                const gchar *message,
                gpointer user_data ) {
  if(debug) {
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
  printf("ERROR: %s\n", msg);
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
  g_debug("Parsing config file...\n");

  if(g_file_test("./erln8.conf", G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
    GKeyFile* kf = g_key_file_new();
    GError* err;
    gboolean b = g_key_file_load_from_file(kf, "./erln8.conf", G_KEY_FILE_NONE, &err);
    if(!g_key_file_has_group(kf, "Erlang")) {
      erln8_error_and_exit("erln8 Erlang group not defined in erln8.conf\n");
      return NULL;
    } else {
      if(g_key_file_has_key(kf, "Erlang", "version", &err)) {
        gchar* erlversion = g_key_file_get_string(kf, "Erlang", "version", &err);
        printf("Running erlang version %s\n", erlversion);
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

gchar* get_configdir_file_name(char* filename) {
  gchar *configfilename = g_strconcat(homedir, "/.erln8.d/", filename, (char*)0);
  return configfilename;
}

gchar* get_config_subdir_file_name(char *subdir, char* filename) {
  gchar *configfilename = g_strconcat(homedir, "/.erln8.d/", subdir, "/", filename, (char*)0);
  return configfilename;
}

/*
void build_erlang() {
  char *fname = "otp_src_R16B02.tar.gz";
  char *fn = get_configdir_file_name("sources","otp_src_R16B02.tar.gz");
  // TODO: check MD5
  gchar *cmd = g_strconcat("tar xf ", fn, " --strip-components 1 -C ", homedir, "/.erln8.d/otps/default/", (char*)0);
  gchar *in;
  gchar *out;
  gint status;
  GError *err;
  printf("Uncompressing sources\n");
  gboolean result = g_spawn_command_line_sync(cmd, &in, &out, &status, &err);
  g_free(cmd);
  g_free(fn);
}
*/

gint git_command(char *command) {
  gchar *cmd = g_strconcat("git ", command, NULL);
  gchar *in;
  gchar *out;
  gint status;
  GError *err;
  //printf("cmd: %s\n", cmd);
  gboolean result = g_spawn_command_line_sync(cmd, &in, &out, &status, &err);
  g_free(cmd);
  if(err != NULL) {
    printf("ERROR: %s\n", err->message);
  }
  // TODO: free in/out?
  printf("Output: %s\n", in);
  return result;
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


void build_erlang(char *repo, char *tag, char *config) {
  // TODO: also check config_env
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

  GError *error;
  gchar* d = g_key_file_to_data (kf, NULL, &error);
  gchar* fn = get_configdir_file_name("config");
  printf("Writing to %s\n", fn);  
  GError *error2;
  if(!g_file_set_contents(fn, d, -1, &error2)) {
    printf("Error writing config file :-(\n");
  }
  printf("DATA = %s\n", d);
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
    mk_config_subdir("repos/github_otp"); // location of git repos
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

int main(int argc, char* argv[]) {
  printf("erln8 v0.0\n");
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_error("option parsing failed: %s\n", error->message);
  }

  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,  erln8_log, NULL);

  g_debug("argv[0] = [%s]\n",argv[0]);
  homedir = g_get_home_dir();
  g_debug("home directory = %s\n", homedir);

  // used for GIO
  g_type_init();



 if(init_erln8) {
    initialize();
 } else {
   if(!check_home()) {
      erln8_error_and_exit("Please initialize erln8 with -i or --init");
    }
  }

/*
./erln8 init
./erln8 clone
./erln8 fetch
./erln8 build default:R15B01 R15B01
./erln8 build basho:R15B01p  R15B01p
./erln8 genconfig basho_patched_R15B01
./erl
./erlc
./escript
*/


  char *cfg = configcheckfromcwd();
  if(cfg != NULL) {
    printf("Using config file [%s]\n", cfg);
  } else {
    printf("erln8 config not found\n");
  }

  /*
  char *erlversion = load_config();
  printf("Config loaded\n");
  char *s = g_strconcat("/Users/dparfitt/erlang-", erlversion, "/bin/", argv[0], (char*)0);
  printf("%s\n",s);
  g_free(erlversion);
  // can't free s
  execl(s,"erl",(char *)0);
  */
}
