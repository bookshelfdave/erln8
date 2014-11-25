#include <boost/program_options.hpp>
#include <boost/range/adaptor/map.hpp>
#include <iostream>
#include <iterator>
#include <unistd.h>
#include "config.h"
#include "repo.h"
#include "builder.h"

namespace bpo = boost::program_options;

void initLogging(bool debug) {
    logging::add_console_log(cout, boost::log::keywords::format = "E8> %Message%");
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= (debug ? logging::trivial::trace : logging::trivial::error)
    );
}


Config processOptions(int argc, char* argv[]) {
    bpo::options_description desc("erln8 options");
    string opt_use;
    string opt_clone;
    string opt_fetch;
    string opt_tag;
    string opt_id;
    string opt_config = "default";
    string opt_repo = "default";

    desc.add_options()
    ("help", "Display help message")
    ("debug", "Print erln8 debugging info")
    ("init", "Initialize erln8")
    ("clone", bpo::value<string>(&opt_clone)->implicit_value("default"),
              "Clone an Erlang source repository")
    ("fetch", bpo::value<string>(&opt_fetch)->implicit_value("default"),
              "Update Erlang source repository")
    ("use", bpo::value<string>(&opt_use),
              "Setup the current directory to use a specific verion of Erlang maintained by erln8")
    ("force", "Use the force")
    ("buildable", "Show buildable versions of Erlang from all configured/cloned repos")
    ("show", "Show the configured version of Erlang in the current working directory")
    ("prompt", "Display the version of Erlang configured for this part of the directory tree")
    ("tag", bpo::value<string>(&opt_tag), "Specified repo branch/tag to build")
    ("id", bpo::value<string>(&opt_id), "A user assigned name for a version of Erlang")
    ("repo", bpo::value<string>(&opt_repo)->implicit_value("default"),
              "Specifies repo name to build from")
    ("config", bpo::value<string>(&opt_config)->implicit_value("default"),
              "Build configuration")
    //("build-rebar", "Build a version of Rebar")
    //("use-rebar", "Build a version of Rebar")
    ("list", "List available Erlang installations")
    ("build", "Build a version of Erlang");

    /*
     *
     * REBAR needs fetch/clone/buildable/show
     *
     */


    try {
        bpo::variables_map vm;
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);

        if(vm.count("debug")) {
          initLogging(true);
        } else {
          initLogging(false);
        }

        Config cfg;
        if (vm.count("help")) {
            cout << endl << "erln8 v" << Erln8_VERSION_MAJOR << "." << Erln8_VERSION_MINOR << 
              ": the sneaky Erlang version manager" << endl;
            cout << "(c) 2014 Dave Parfitt" << endl;
            cout << "Licensed under the Apache License, Version 2.0" << endl << endl;
            cout << desc << "\n";
            exit(0);
        }

        if(vm.count("init")) {
            cout << "Initializing erln8" << std::endl;
            cfg.initialize();
            cout << "All set!" << std::endl;
            exit(0);
        }

        /* LOAD CONFIG */
        cfg.load();

        if(vm.count("use")) {
          auto found = cfg.erlangs.find(opt_use);
          if(found == cfg.erlangs.end()) {
            cerr << opt_use << " isn't a version of Erlang managed by erln8" << endl;
            exit(-1);
          }

          bfs::path cwd(bfs::current_path());
          BOOST_LOG_TRIVIAL(trace) << "Checking path from cwd: " << cwd;

          DirConfig dc(cwd);
          dc.create(opt_use, vm.count("force"));
          return cfg;
        }

        if(vm.count("list")) {
            for(auto k : cfg.erlangs | boost::adaptors::map_keys) {
              cout << k << endl;
            }
        }

        if(vm.count("clone")) {
            string repoName = opt_clone;
            auto found = cfg.repos.find(opt_clone);
            if(found == cfg.repos.end()) {
              cerr << opt_clone << " isn't an Erlang repo managed by erln8" << endl;
              exit(-1);
            }

            BOOST_LOG_TRIVIAL(trace) << "Cloning repo " << repoName;
            Repo repo(repoName);
            repo.clone(cfg);
            return cfg;
        }

        if(vm.count("fetch")) {
            string repoName = opt_fetch;
            auto found = cfg.repos.find(opt_fetch);
            if(found == cfg.repos.end()) {
              cerr << opt_fetch << " isn't an Erlang repo managed by erln8" << endl;
              exit(-1);
            }
            cout << "Fetching from repo " << opt_fetch << endl;
            BOOST_LOG_TRIVIAL(trace) << "Fetch repo " << repoName;
            Repo repo(repoName);
            repo.fetch(cfg);
            BOOST_LOG_TRIVIAL(trace) << "Done fetching repo " << repoName;
            return cfg;
        }

        if(vm.count("buildable")) {
            cout << "Buildable Erlang tags per repo:" << std::endl;
            for(auto k : cfg.repos | boost::adaptors::map_keys) {
              Repo r(k);
              r.showBuildable(cfg);
            }
            return cfg;
        }
        if(vm.count("show") || vm.count("prompt")) {
          auto result = cfg.configCheckFromCwd();
          if(result) {
            DirConfig dc(result.get());
            dc.load();
            cout << dc.erlangTag;
            if(vm.count("show")) {
              cout << std::endl;
            }
          }
          return cfg;
        }


        if(vm.count("build")) {
            auto foundRepo = cfg.repos.find(opt_repo);
            if(foundRepo == cfg.repos.end()) {
              cerr << opt_repo << "X isn't an Erlang repo managed by erln8" << endl;
              exit(-1);
            }

            auto foundConfig = cfg.configs.find(opt_config);
            if(foundRepo == cfg.repos.end()) {
              cerr << opt_config << "X isn't an erln8 build config" << endl;
              exit(-1);
            }

            if(!vm.count("id")) {
              cerr << "Please specify --id" << endl;
              exit(-1);
            }

            if(!vm.count("tag")) {
              cerr << "Please specify a tag/branch to build with --tag" << endl;
              exit(-1);
            }

            // look for VALID config, otherwise, use system default
            // use default repo if not specified
            // check if ID already exists
            Builder b(opt_repo, opt_tag, opt_id, opt_config);
            b.build(cfg);
            return cfg;
        }



      return cfg;
    } catch (boost::program_options::invalid_command_line_syntax s) {
        cout << "Argument error" << std::endl;
        cout << desc << "\n";
        exit(-1);
    } catch(...) {
        cout << "Invalid arguments" << std::endl;
        cout << desc << "\n";
        exit(-1);
    }
}

int main(int argc, char* argv[]) {

    //Config cfg;
    // processOptions sets up logging too
    Config cfg = processOptions(argc, argv);

    bfs::path called_as(argv[0]);
    string basename = called_as.filename().string();
    BOOST_LOG_TRIVIAL(trace) << "Called as " << basename << endl;

    if(basename == "erln8") {
      //cout << "erln8 v" << Erln8_VERSION_MAJOR << "." << Erln8_VERSION_MINOR << std::endl;
      //cout << "Try erln8 --help for more info" << endl;
    } else {
        BOOST_LOG_TRIVIAL(trace) << "Erlang command";

        auto result = cfg.configCheckFromCwd();

        if(result) {
            DirConfig dc(result.get());
            dc.load();
            if(cfg.erlangs.find(dc.erlangTag) == cfg.erlangs.end()) {
                // TODO: bail
                cerr << "Unconfigured version of Erlang detected: " << dc.erlangTag << std::endl;
            } else {
                string erlangPath = cfg.erlangs[dc.erlangTag];
                BOOST_LOG_TRIVIAL(trace) << "Erlang path = " << erlangPath;
                bfs::path cmdPath = bfs::path(erlangPath) / bfs::path(basename);
                execv(cmdPath.c_str(), argv);
            }
        } else {
            cout << "FAIL" << std::endl;
        }
    }
}

