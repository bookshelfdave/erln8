#ifndef ERN8_LINK_H
#define ERN8_LINK_H
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
#include "config.h"


namespace bfs = boost::filesystem;
namespace logging = boost::log;
using namespace std;

class LinkedErlang {
public:
    LinkedErlang(string name, bfs::path p) : name(name), p(p) {}
    void link(Config &cfg);
    void isLinked();
    void unlink();
private:
    string name;
    bfs::path p;
};

#endif
