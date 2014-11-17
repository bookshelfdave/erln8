#include "config.h"
#include <stdlib.h>

Config::Config() {
    BOOST_LOG_TRIVIAL(trace) << "Config in : " ERLN8_CONFIG_DIR;
    detectHomeDir();

}

void Config::save() {
    bfs::path configFilePath = getConfigFilePath();

    if(!exists(configFilePath)) {
        cerr << "erln8 not initialized, please use erln8 --init." << endl;
        exit(-1);
    }

    write_ini(configFilePath.c_str(), pt);
}

void Config::load() {
    bfs::path configFilePath = getConfigFilePath();

    if(!exists(configFilePath)) {
        cerr << "erln8 not initialized, please use erln8 --init." << endl;
        exit(-1);
    }

    read_ini(configFilePath.c_str(), pt);

    try {
        color = pt.get<string>("Erln8.color");
        banner = pt.get<string>("Erln8.banner");
        default_config = pt.get<string>("Erln8.default_config");
        system_default = pt.get<string>("Erln8.system_default");

        ptree erlangsTree = pt.get_child("Erlangs");

        for(const auto& kv : erlangsTree) {
            BOOST_LOG_TRIVIAL(trace) << "Erlang: " << kv.first << "->" << kv.second.data();
            erlangs[kv.first] = kv.second.data();
        }

        /*ptree systemRootsTree = pt.get_child("SystemRoots");
        for(const auto& kv : systemRootsTree) {
            BOOST_LOG_TRIVIAL(trace) << "SystemRoot: " << kv.first << "->" << kv.second.data();
            systemRoots[kv.first] = kv.second.data();
        }*/

        ptree reposTree = pt.get_child("Repos");

        for(const auto& kv : reposTree) {
            BOOST_LOG_TRIVIAL(trace) << "Repo: " << kv.first << "->" << kv.second.data();
            repos[kv.first] = kv.second.data();
        }

        ptree configsTree = pt.get_child("Configs");

        for(const auto& kv : configsTree) {
            BOOST_LOG_TRIVIAL(trace) << "Config: " << kv.first << "->" << kv.second.data();
            configs[kv.first] = kv.second.data();
        }

    } catch (boost::property_tree::ptree_bad_path& p) {
        cout << "Erln8 config missing property:" <<  p.what() << endl;
        // TODO: bail
        exit(-1);
    }

}

void Config::addErlang(string id, string path) {
  pt.put("Erlangs." + id, path);
}


void Config::initialize() {
    bfs::path configDir = getConfigDir();

    if(exists(configDir)) {
        cerr << "erln8 has already been initialized" << endl;
        exit(-1);
    }

    BOOST_LOG_TRIVIAL(trace) << "Creating " << configDir;
    create_directory(configDir);
    BOOST_LOG_TRIVIAL(trace) << "Creating otps";
    create_directory(configDir / bfs::path("otps"));
    BOOST_LOG_TRIVIAL(trace) << "Creating rebars";
    create_directory(configDir / bfs::path("rebars"));
    BOOST_LOG_TRIVIAL(trace) << "Creating logs";
    create_directory(configDir / bfs::path("logs"));
    BOOST_LOG_TRIVIAL(trace) << "Creating repos";
    create_directory(configDir / bfs::path("repos"));
    BOOST_LOG_TRIVIAL(trace) << "Creating rebar repos";
    create_directory(configDir / bfs::path("rebar_repos"));
    BOOST_LOG_TRIVIAL(trace) << "Done creating directories";

    cout << "Creating config file " << getConfigFilePath() << endl;
    pt.put("Erln8.color", true);
    pt.put("Erln8.banner", false);
    pt.put("Erln8.default_config", "default");
    pt.put("Erln8.system_default", "");
    pt.put("Repos.default", "https://github.com/erlang/otp.git");
    pt.put("RebarRepos.default", "https://github.com/rebar/rebar.git");
    pt.put("Erlangs.none","");
    pt.put("Configs.default","");
    pt.put("Configs.osx_llvm","--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit");
    pt.put("Configs.osx_llvm_dtrace", "--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit --enable-vm-probes --with-dynamic-trace=dtrace");
    pt.put("Configs.osx_gcc", "--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit");
    pt.put("Configs.osx_gcc_env", "CC=gcc-4.2 CPPFLAGS=\'-DNDEBUG\' MAKEFLAGS=\'-j 3\'");
    pt.put("SystemRoots.none", "");
    write_ini(getConfigFilePath().c_str(), pt);
}


bfs::path Config::getConfigFilePath() {
    bfs::path configFile("config");
    bfs::path fullPath = getConfigDir() / configFile;
    return fullPath;
}

bfs::path Config::getConfigDir() {
    bfs::path homeDir(homedir);
    bfs::path configDir(ERLN8_CONFIG_DIR);
    return homeDir / configDir;
}

void Config::detectHomeDir() {
    int bufsize;

    if ((bufsize = sysconf(_SC_GETPW_R_SIZE_MAX)) == -1)
        abort();

    char buffer[bufsize];
    struct passwd pwd, *result = NULL;

    if (getpwuid_r(getuid(), &pwd, buffer, bufsize, &result) != 0 || !result) {
        cerr << "Can't find home directory" << endl;
        abort();
    }

    homedir = string(pwd.pw_dir);
    BOOST_LOG_TRIVIAL(trace) << "Home directory: " << homedir;
}

Config::~Config() {
}

string Config::getVersion() {
    ostringstream oss;
    oss << "erln8 v" << Erln8_VERSION_MAJOR << "." << Erln8_VERSION_MINOR;
    return oss.str();
}

string Config::getHomeDir() {
    return homedir;
}


boost::optional<bfs::path> Config::configCheckFromCwd() {
    bfs::path cwd(bfs::current_path());
    BOOST_LOG_TRIVIAL(trace) << "Checking path from cwd: " << cwd;
    return configCheck(cwd);
}

boost::optional<bfs::path> Config::configCheck(bfs::path d) {
    bfs::path configFileForPath = d / ERLN8_CONFIG_FILE;
    BOOST_LOG_TRIVIAL(trace) << "Checking path " << d << " for " << configFileForPath;

    if(exists(configFileForPath) && is_regular_file(configFileForPath)) {
        BOOST_LOG_TRIVIAL(trace) << "Found file at " << configFileForPath;
        return configFileForPath;
    } else if(d.has_parent_path()) {
        return configCheck(d.parent_path());
    } else {
        BOOST_LOG_TRIVIAL(trace) << "Reached root, no config file found";
        return boost::none;
    }
}

DirConfig::DirConfig(bfs::path p) {
    BOOST_LOG_TRIVIAL(trace) << "Reading dirconfig from " << p;

    try {
        read_ini(p.c_str(), pt);
    } catch (boost::property_tree::ini_parser::ini_parser_error& e) {
        cerr << p << " isn't a valid erln8 file" << endl;
        return;
    }

    boost::optional<string> erlang = pt.get_optional<string>("Config.Erlang");

    if(erlang) {
        erlangTag = string(erlang.get());
        BOOST_LOG_TRIVIAL(trace) << "Erlang tag to use: " << erlangTag;
    } else {
        BOOST_LOG_TRIVIAL(trace) << "DATA NOT FOUND";
        throw runtime_error("erln8 file corrupt");
        return;
    }
    boost::optional<string> rebar = pt.get_optional<string>("Config.Rebar");
    if(rebar) {
        rebarTag = string(rebar.get());
        BOOST_LOG_TRIVIAL(trace) << "Rebar tag to use: " << rebarTag;
    } else {
        cout << "DEFAULT REBAR" << endl;
        BOOST_LOG_TRIVIAL(trace) << "Using default version of Rebar";
    }
}




