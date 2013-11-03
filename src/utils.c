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
    printf("  Progress: %d%%", pcnt);
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

void download_configdir_file(char *url, char *subdir, char *filename) {
  gchar *configfilename = get_configdir_file_name(subdir, filename);
  printf("Downloading %s\n", filename);
  download_file(url, configfilename);
  g_free(configfilename);
}


/*void download_erlang(char *version) {
  // TODO: don't hardcode the latest version, duh :-)
  download_configdir_file("http://www.erlang.org/download/otp_src_R16B02.tar.gz", "sources", "otp_src_R16B02.tar.gz");
}*/





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



gboolean file_exists(char *filename) {
  return g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
}

