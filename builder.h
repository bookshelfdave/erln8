#ifndef ERN8_BUILDER_H
#define ERN8_BUILDER_H
#include <iostream>
#include "config.h"

using namespace std;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


class Builder {
  public:
    Builder(string repo, string tag, string id, string build_config);
    void build(Config&);
  private:
    string repo;
    string tag;
    string id;
    string build_config;
};


class RebarBuilder {
  public:
    RebarBuilder(string repo, string tag, string id, string build_config);
    void build(Config&);
  private:
    string repo;
    string tag;
    string id;
    string build_config;
};

class BuildTask {
  public:
    BuildTask(string cmd, string description, string failMsg);
    void run(Config &cfg);
  private:
    string cmd;
    string description;
    string failMsg;
};

class Color {
  public:
  static string blue(Config& cfg) { return returnIfColorEnabled(cfg, ANSI_COLOR_BLUE); }
  static string red(Config& cfg) { return returnIfColorEnabled(cfg, ANSI_COLOR_RED); }
  static string yellow(Config& cfg) { return returnIfColorEnabled(cfg, ANSI_COLOR_YELLOW); }
  static string green(Config& cfg) { return returnIfColorEnabled(cfg, ANSI_COLOR_GREEN); }
  static string reset(Config& cfg) { return returnIfColorEnabled(cfg, ANSI_COLOR_RESET); }
  static string returnIfColorEnabled(Config &cfg, string value) {
      if (cfg.color == "true") {
        return value;
      }
      else {
        return "";
      }
    }
  };
#endif
