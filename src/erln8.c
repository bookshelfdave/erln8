#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define G_LOG_DOMAIN    ((gchar*) 0)
#define SOURCES "sources"
#define MD5     "MD5"
static gboolean init_erln8 = FALSE;
static gboolean debug      = FALSE;
static const gchar* homedir;

static unsigned int last_pcnt = 0;

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
  int prefixlen = 14;
  if(t > 0 && d > 0) {
    int pcnt = (int)((100.0) * d / t);
    int len = 0;
    int i = 0;
    if(pcnt < 10) {
      len = 1 + prefixlen;
    } else if(pcnt < 100) {
      len = 2 + prefixlen;
    } else if(pcnt >= 100) {
      len = 3 + prefixlen;
    }

    for(i = 0; i < len + 1; i++) {
      printf("\b");
      fflush(stdout);
    }
    printf("Downloading: %d%%", pcnt);
    last_pcnt = pcnt;
  }
  return 0;
}


void download_file(char *url, char *filename) {
  CURL *curl;
  CURLcode res;
  FILE *outfile;

  curl = curl_easy_init();
  if(curl)
  {
    outfile = fopen(filename, "w");
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
    // print up a newline after the progress meter stops
    printf("\n");
  }
}

gchar* get_configdir_file_name(char *subdir, char* filename) {
  gchar *configfilename = g_strconcat(homedir, "/.erln8.d/", subdir, "/", filename, (char*)0);
  return configfilename;
}

void download_configdir_file(char *url, char *subdir, char *filename) {
  gchar *configfilename = get_configdir_file_name(subdir, filename);
  printf("Downloading %s\n", filename);
  download_file(url, configfilename);
  g_free(configfilename);
}

void download_current_erlang() {
  // TODO: don't hardcode the latest version, duh :-)
  download_configdir_file("http://www.erlang.org/download/otp_src_R16B02.tar.gz", "sources", "otp_src_R16B02.tar.gz");
}

void build_erlang() {
  char *fname = "otp_src_R16B02.tar.gz";
  char *fn = get_configdir_file_name("sources","otp_src_R16B02.tar.gz");
  // TODO: check MD5
  gchar *cmd = g_strconcat("tar xf ", fn, " --strip-components 1 -C ", homedir, "/.erln8.d/otps/default/", (char*)0);
  gchar *in;
  gchar *out;
  gint status;
  GError *err;
  printf("%s\n", cmd);
  gboolean result = g_spawn_command_line_sync(cmd, &in, &out, &status, &err);
  g_free(cmd);
  g_free(fn);
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

//gint compare_names(gconstpointer namea, gconstpointer nameb) {
//  return ((gint)strcmp((char*)namea, (char*)nameb));
//}

void parse_md5s() {
  gchar *configfilename = get_configdir_file_name(SOURCES, MD5);
  gboolean result = g_file_test(configfilename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
  gchar *contents;
  GError *err;
  gboolean result2 = g_file_get_contents(configfilename, &contents, NULL, &err);
  printf("Contents = \n%s\n", contents);
  gchar** lines = g_strsplit(contents,"\n", 1000);
  GSList *otplist = NULL;
  //TODO: FREE REGEX STUFF
  for(;*lines != NULL; *lines++) {
    GError *reerr;
    GRegex *gre = g_regex_new("MD5\\(otp_src_(R[0-9]+[A-Z](-)?([0-9]+)?)\\.tar\\.gz\\)=\\ ([a-zA-Z0-9]+)",
                              0,
                              0,
                              &reerr);
    GMatchInfo *match_info;
    g_regex_match (gre, *lines, 0, &match_info);
    if(g_match_info_matches(match_info)) {
      gchar* vsn = g_match_info_fetch(match_info, 1);
      printf("%s\n",vsn);
      //list = g_slist_append(otplist, vsn);

      //printf(">>> %s\n", *lines);
    }
    free(*lines);
  }
  //g_strfreev(lines);
  g_free(contents);
  g_free(configfilename);
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
    mk_config_subdir("sources"); // location of .tar.gz source files
    mk_config_subdir("sources/default"); // location of .tar.gz source files
    mk_config_subdir("otps");    // location of compiled otp source files
    mk_config_subdir("otps/default");    // location of compiled otp source files
    mk_config_subdir("configs"); // specific configs to use for a build
    mk_config_subdir("patches"); // specific patches to use for a build
    download_current_erlang();
  }
}

gboolean file_exists(char *filename) {
  return g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
}


void detect_platform() {
  if(file_exists("/etc/SuSE-release")) {
    printf("SUSE\n");
  } else if(file_exists("/etc/redhat-release")) {
    printf("Redhat\n");
  } else if(file_exists("/etc/fedora-release")) {
    printf("Fedora\n");
  } else if(file_exists("/etc/debian-version")) {
    printf("Debian\n");
  } else if(file_exists("/etc/slackware-version")) {
    printf("Slackware\n");
  } else if(file_exists("/mach_kernel")) {
    printf("OSX\n");
  }
  //if(file_exists("/proc/version")) {
  //}
  // lsb_release -rd
  // cat /proc/version
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

  //download_configdir_file("http://www.erlang.org/download/MD5","sources", "MD5");
  //parse_md5s();

  if(init_erln8) {
    initialize();
  } else {
    if(!check_config()) {
      erln8_error_and_exit("Please initialize erln8 with -i or --init");
    }
  }

  build_erlang();

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
