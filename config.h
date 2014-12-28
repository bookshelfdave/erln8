#ifndef ERN8_CONFIG_H
#define ERN8_CONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include "version.h"


#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <boost/optional.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>

#define ERLN8_CONFIG_FILE	"erln8.config"
#define ERLN8_CONFIG_DIR	".erln8.d"

namespace bfs = boost::filesystem;
namespace logging = boost::log;

using boost::property_tree::ptree;
namespace bpt = boost::property_tree;
typedef bpt::ptree::path_type path;

using namespace std;

class Config {
public:
    Config();
    ~Config();
    void load();
    void save();
    void initialize();
    string getVersion();
    string getHomeDir();

    bfs::path getConfigDir();
    bfs::path getConfigFilePath();

    boost::optional<bfs::path> configCheckFromCwd();
    boost::optional<bfs::path> configCheck(bfs::path d);

    boost::optional<string> systemRootCheckFromCwd();
    boost::optional<string> systemRootCheck(bfs::path d);

    void addErlang(string id, string path);
    /*
    void addRepo(string, string);
    void addConfig(string, string);
    */

    map<string, string> erlangs;
    map<string, string> configs;
    map<string, string> repos;
    map<string, string> systemRoots;
    string color;
    string banner;
    string default_config;
    string system_default;
    boost::property_tree::ptree pt;
    string homedir;
private:
    void detectHomeDir();

};

class DirConfig {
public:
    DirConfig(bfs::path);
    void load();
    void create(string erlangVersion, bool force);
    string erlangTag;
    string rebarTag;
private:
    bfs::path p;
    boost::property_tree::ptree pt;
};

#endif
