#ifndef ERN8_REPO_H
#define ERN8_REPO_H

#include <iostream>

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

#include "version.h"
#include "config.h"

namespace bfs = boost::filesystem;
namespace logging = boost::log;

class Repo {
public:
    Repo(std::string name);
    bool isCloned(Config);
    void clone(Config);
    void fetch(Config);
    bfs::path getPathToRepo(Config);
    void showBuildable(Config);
private:
    string repo_name;
};


/*
class RebarRepo {
public:
    RebarRepo(std::string name);
    bool isCloned(Config);
    void clone(Config);
    void fetch(Config);
    bfs::path getPathToRepo(Config);
    void showBuildable(Config);
private:
    string repo_name;
};
*/

#endif

