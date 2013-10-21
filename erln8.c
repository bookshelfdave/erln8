#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define G_LOG_DOMAIN    ((gchar*) 0)
static gboolean init_erln8 = FALSE;
static gboolean debug      = FALSE;
static const gchar* homedir;

static GOptionEntry entries[] =
{
  { "init", 'i', 0, G_OPTION_ARG_NONE, &init_erln8, "Initialize Erln8", NULL },
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Debug Erln8", NULL },
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

// TODO: move the download stuff to another module
size_t erln8_write_func(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

size_t erln8_read_func(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fread(ptr, size, nmemb, stream);
}

int erln8_progress_func(void *unused,
                     double t, /* dltotal */ 
                     double d, /* dlnow */ 
                     double ultotal,
                     double ulnow) {
  int it = (int)t;
  int id = (int)d;
  printf("%d of %d\n", id, it);
  return 0;
}


void download_erlang() {
  char *url =  "http://www.erlang.org/download/otp_src_R16B02.tar.gz";
  CURL *curl;
  CURLcode res;
  FILE *outfile;

  curl = curl_easy_init();
  if(curl)
  {
    outfile = fopen("otp16.tar.gz", "w");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, erln8_write_func);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, erln8_read_func);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, erln8_progress_func);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, NULL);

    res = curl_easy_perform(curl);

    fclose(outfile);
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }

  return NULL;
}

void check_path() {
  gchar *out;
  gchar *err;
  gint   status;
  GError *error;
  g_spawn_command_line_sync ("which erl", &out, &err, &status, &error);
  if(!status) {
    erln8_error_and_exit("Erlang already exists in the path\n");
  }
}


gboolean check_config() {
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

void initialize() {
  if(check_config()) {
    erln8_error_and_exit("Configuration directory ~/.erln8.d already exists");
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
    mk_config_subdir("sources"); // location of .tar.gz source files
    mk_config_subdir("opts");    // location of compiled otp source files
    mk_config_subdir("configs"); // specific configs to use for a build
  }
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

  download_erlang();

/*  if(init_erln8) {
    initialize();
  } else {
    if(!check_config()) {
      erln8_error_and_exit("Please initialize erln8 with -i or --init");
    }
  }
*/

  //check_path();

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
