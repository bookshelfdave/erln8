#import "link.h"


void LinkedErlang::link(Config &cfg) {
  if(!p.has_root_directory()) {
    cerr << "An absolute path must be specified to --link" << endl;
    exit(-1);
  }

  if(!exists(p)) {
    cerr << p << " doesn't exist" << endl;
    exit(-1);
  }

  auto found = cfg.erlangs.find(name);
  if(found != cfg.erlangs.end()) {
    cerr << "An installation of Erlang is already referenced by id " << name << endl;
    exit(-1);
  }

  bfs::path pathToBinErl = p / "bin" / "erl";
  if(!exists(pathToBinErl)) {
    cerr << "Can't link to an empty Erlang installation" << endl;
    exit(-1);
  }

  bfs::path distDir = cfg.getConfigDir() / "otps" / name;
  if(!create_directories(distDir)) {
    cerr << "Can't create " << distDir << endl;
    exit(-1);
  }
  cout << "Linking " << p << " as " << name << endl;
  cout << "Done" << endl;
}

void LinkedErlang::isLinked() {

}

void LinkedErlang::unlink() {

}

