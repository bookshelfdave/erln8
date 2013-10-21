#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include <curl/curl.h>

char* load_config() {
  printf("Parsing config file...\n");

  if(g_file_test("./erln8.conf", G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
    GKeyFile* kf = g_key_file_new();
    GError* err;
    gboolean b = g_key_file_load_from_file(kf, "./erln8.conf", G_KEY_FILE_NONE, &err);

    if(!g_key_file_has_group(kf, "Erlang")) {
      printf("erln8 Erlang group not defined in erln8.conf\n");
      return NULL;
    } else {
      if(g_key_file_has_key(kf, "Erlang", "version", &err)) {
        gchar* erlversion = g_key_file_get_string(kf, "Erlang", "version", &err);
        printf("Running erlang version %s\n", erlversion);
        // THIS VALUE MUST BE FREED
        return erlversion;
      } else {
        printf("Missing Erlang | version\n");
        return NULL;
      }
    }

  } else {
    printf("Config file does not exist\n");
    return NULL;
  }
}

void download_erlang() {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://www.erlang.org/download/otp_src_R16B02.tar.gz");
    /* example.com is redirected, so we tell libcurl to follow redirection */ 
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
          curl_easy_strerror(res));
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
}

void check_path() {
  gchar *out;
  gchar *err;
  gint   status;
  GError *error;
  g_spawn_command_line_sync ("which erl", &out, &err, &status, &error);
  if(!status) {
    printf("Erlang already exists in the path\n");
  }
}


int main(int argc, char* argv[]) {
  printf("erln8 v0.0\n");
  printf("%s\n",argv[0]);

  //check_path();

  char *erlversion = load_config();
  printf("Config loaded\n");
  char *s = g_strconcat("/Users/dparfitt/erlang-", erlversion, "/bin/", argv[0], (char*)0);
  printf("%s\n",s);
  g_free(erlversion);
  // can't free s
  execl(s,"erl",(char *)0);
}
