#include "builder.h"
#include <stdlib.h>

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

Builder::Builder(std::string repo, std::string tag, std::string id, std::string build_config)
  : repo(repo), tag(tag), id(id), build_config(build_config) {

}


void Builder::build(Config& cfg) {
  cout << "Building " << tag << endl;


  bfs::path tmp = bfs::temp_directory_path() / bfs::unique_path();
  cout << tmp.c_str() << endl;
  create_directories(tmp);



  bfs::path outputPath = cfg.getConfigDir() / "otps" / id;

  bfs::path logFile = cfg.getConfigDir() / "logs" / id;
  cout << "Log file: " << logFile << endl;

  bfs::path repoPath = cfg.getConfigDir() / "repos" / repo;
  cout << "Repo path: " << repoPath << endl;

  cout << outputPath.c_str() << endl;
  if(exists(outputPath)) {
    cerr << "Build " << id << " already exists" << endl;
    exit(-1);
  }


  vector<BuildTask> tasks;

  // TODO: ENV stuff
  // check tag
  string checkObj = string("cd ") + repoPath.c_str() + " && git show-ref " + tag + " > /dev/null";
  BuildTask checkObjTask(checkObj, "Ensure Git object exists",
      string("Git object") + tag + " does not exist");
  tasks.push_back(checkObjTask);

  string copySourceCmd = string(" cd ") + repoPath.c_str() + " && git archive " + tag +
              " | (cd " + tmp.c_str() + "; tar -f - -x)";
  BuildTask copySourceTask(copySourceCmd, "Copy Erlang source", "Source copy failed");
  tasks.push_back(copySourceTask);

  string cdToSourceDir = string("cd ") + tmp.c_str() + " && ";
  string appendToLog  = string(" >> ") + logFile.c_str() + " 2>&1";
  string otpBuildCmd = cdToSourceDir + "./otp_build autoconf" + appendToLog;
  BuildTask otpBuildTask(otpBuildCmd, "Run otp_build", "otp_build failed");
  tasks.push_back(otpBuildTask);

  string otpConfigureCmd = cdToSourceDir + "./configure --prefix=" + outputPath.c_str() + appendToLog;
  BuildTask otpConfigureTask(otpConfigureCmd, "Configure", "./configure failed");
  tasks.push_back(otpConfigureTask);

  string otpMakeCmd = cdToSourceDir + "make" + appendToLog;
  BuildTask otpMakeTask(otpMakeCmd, "Building Erlang", "Failed to build Erlang");
  tasks.push_back(otpMakeTask);

  string otpMakeInstallCmd = cdToSourceDir + "make install" + appendToLog;
  BuildTask otpMakeInstallTask(otpMakeInstallCmd, "Installing Erlang", "Failed to install Erlang");
  tasks.push_back(otpMakeInstallTask);

  string otpMakeInstallDocsCmd = cdToSourceDir + "make install-docs" + appendToLog;
  BuildTask otpMakeInstallDocsTask(otpMakeInstallCmd, "Installing Erlang Documentation", "Failed to install Erlang documentation");
  tasks.push_back(otpMakeInstallDocsTask);

  // create links
  string createLinksCmd = string("cd ") + outputPath.c_str() + " && for i in `find -L . -perm -111 -type f | grep -v \"\\.so\" | grep -v \"\\.o\" | grep -v \"lib/erlang/bin\" | grep -v Install`; do  `ln -s -f $i $(basename $i)` ; done";
  cout << createLinksCmd << endl;
  BuildTask createLinksTask(createLinksCmd, "Creating erln8 links", "Failed to create erln8 links");
  tasks.push_back(createLinksTask);

  int i = 0;
  for(BuildTask& task : tasks) {
    cout << Color::yellow() << "[" << i++ << "] ";
    task.run();
  }

  cout << "Adding " << id << " to ~/.erln8.d/config" << endl;
  cfg.addErlang(id, outputPath.c_str());
  cfg.save();
  cout << "Done." << endl;
}



BuildTask::BuildTask(string cmd, string description, string failMsg) : 
  cmd(cmd), description(description), failMsg(failMsg) {

}

void BuildTask::run() {
  auto pos = description.length();
  int max = 40;
  cout << Color::blue() << description << "...";
  while(pos < max) {
    cout << " ";
    pos++;
  }
  cout.flush();
  if(system(cmd.c_str()) != 0) {
    cerr << Color::red() << " Failed:" <<  failMsg << endl;
    exit(-1);
  } else {
    cout << Color::green() << "Success" << Color::reset() << endl;
  }
}



RebarBuilder::RebarBuilder(std::string repo, std::string tag, std::string id, std::string build_config)
  : repo(repo), tag(tag), id(id), build_config(build_config) {

}


void RebarBuilder::build(Config& cfg) {
  cout << "Building Rebar " << tag << endl;
}


