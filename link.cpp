#import "link.h"


void LinkedErlang::link(Config &cfg) {
//  if(!p.has_root_directory()) {
//    cerr << "An absolute path must be specified to --link" << endl;
//    exit(-1);
//  }
//
//  if(!exists(p)) {
//    cerr << p << " doesn't exist" << endl;
//    exit(-1);
//  }
//
//  auto found = cfg.erlangs.find(name);
//  if(found != cfg.erlangs.end()) {
//    cerr << "An installation of Erlang is already referenced by id " << name << endl;
//    exit(-1);
//  }

  bfs::path pathToBinErl = p / "bin" / "erl";
  if(!exists(pathToBinErl)) {
    cerr << "Can't link to an empty Erlang installation" << endl;
    exit(-1);
  }

  bfs::path topLevelOTPDir = cfg.getConfigDir() / "otps" / name;
//  try {
//    create_directories(topLevelOTPDir);
//  } catch(bfs::filesystem_error fse) {
//    cerr << "Can't create " << topLevelOTPDir << ":" << fse.what() << endl;
//    exit(-1);
//  }

  bfs::path distDir = topLevelOTPDir / "dist";
//  cout << "Linking " << p << " as " << name << endl;
//  cout << distDir << endl;
//  try {
//    create_symlink(p, distDir);
//  } catch(bfs::filesystem_error fse) {
//    cout << "Error creating symlink: " << fse.what() << std::endl;
//    exit(-1);
//  }
  auto exeperms = (bfs::owner_exe | bfs::group_exe | bfs::others_exe);
  bfs::path sosuffix (".so");
  bfs::path osuffix (".o");


  for(auto& entry : boost::make_iterator_range(bfs::recursive_directory_iterator(distDir), {})) {
    // TODO: can error
    bfs::file_status s = status(entry);
    if(s.type() == bfs::file_type::regular_file) {
      string fullpath(entry.path().c_str());
      auto found = fullpath.find("lib/erlang/bin");
      if (found == std::string::npos) {
        if ((entry.path().extension() != sosuffix) && (entry.path().extension() != osuffix)) {
          if ((s.permissions() & exeperms) == exeperms) {
            cout << ">>" << entry << " " << s.permissions() << " " << " " << endl;
          }
        }
      }
    }
  }
}

void LinkedErlang::isLinked() {

}

void LinkedErlang::unlink() {

}

void LinkedErlang::generateLinks(Config &cfg) {

}
