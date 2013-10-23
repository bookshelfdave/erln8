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

