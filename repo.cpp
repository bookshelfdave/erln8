#include "repo.h"
#include <stdlib.h>

Repo::Repo(string name) : repo_name(name) {
}

bool Repo::isCloned(Config cfg) {
    return exists(getPathToRepo(cfg));
}

void Repo::clone(Config cfg) {
    if(isCloned(cfg)) {
        cerr << "Repo " << repo_name << " is already cloned" << endl;
        exit(-1);
    }

// TODO: check if repo exists!
    string cmd = "git clone " + cfg.repos[repo_name] + " " + getPathToRepo(cfg).c_str();
    BOOST_LOG_TRIVIAL(trace) << "Exec " << cmd;
    system(cmd.c_str());
}

void Repo::fetch(Config cfg) {
    if(!isCloned(cfg)) {
        cerr << "Repo " << repo_name << " has not yet been cloned." << endl;
        exit(-1);
    }
    // TODO: check if repo exists!
    string cmd = "cd " + string(getPathToRepo(cfg).c_str()) + " && git fetch --all --verbose";
    BOOST_LOG_TRIVIAL(trace) << "Exec " << cmd;
    system(cmd.c_str());
}


void Repo::showBuildable(Config cfg) {
  if(!isCloned(cfg)) {
    cerr << "Repo " << repo_name << " has not been cloned yet" << endl;
    return;
  }
  cout << "Repo: " << repo_name << endl;
  string cmd = string("cd ") + getPathToRepo(cfg).c_str() + " && git tag | sort";
  system(cmd.c_str());
}

bfs::path Repo::getPathToRepo(Config cfg) {
    bfs::path pathToRepo = cfg.getConfigDir() / "repos" / bfs::path(repo_name);
    return pathToRepo;
}


