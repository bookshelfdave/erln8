require 'rubygems'
require 'bundler/setup'
require "test/unit"
require 'iniparse'
class TestRepo
  def initialize(root, tags)
    `rm -rf #{root}`
    `mkdir #{root}`
    tags.each do |tag|
      program =
            "#include <stdio.h>
            int main(int argc, char* argv[]) {
                printf(\"Erln8 Test #{tag}\");
                return 0;
            }"
      makefile =
      "all:
\tgcc -o erln8_test erln8_test.c
  "

    configure =
      "#!/bin/sh
echo \"Fake configure\"
  "
      File.open("./#{root}/erln8_test.c", 'w') { |file| file.write(program) }
      File.open("./#{root}/Makefile", 'w') { |file| file.write(makefile) }
      File.open("./#{root}/configure", 'w') { |file| file.write(configure) }
      `cd ./#{root} && git init && git add . && git commit -m \"commit #{tag}\"`
      `cd ./#{root} && git tag #{tag}`
      `chmod 755 ./#{root}/configure`
    end
  end
end

class Erln8Test < Test::Unit::TestCase

  @@teardown = true
  def setup
    `mkdir testconfig`
  end

  def test_fake_repo
    TestRepo.new("repo_a", ["a","b","c"])
    TestRepo.new("repo_b", ["d","e","f"])
    assert File.exist?("./repo_a/.git")
    assert File.exist?("./repo_b/.git")
  end

  def teardown
    if @@teardown == true then
      %w[a b c].each do |repo|
        `rm -rf ./repo_#{repo}`
      end
      `rm -rf ./testconfig`
    end
  end

  def test_init
    result = run_cmd "--init"
    lines = result.split("\n")
    assert_equal("Creating erln8 config file: ./testconfig/.erln8.d/config", lines[0])
    assert File.exist?("./testconfig/.erln8.d/config")
    assert File.exist?("./testconfig/.erln8.d/repos")
    assert File.exist?("./testconfig/.erln8.d/logs")
    assert File.exist?("./testconfig/.erln8.d/otps")
  end

  def test_default_clone
    TestRepo.new("repo_a", ["a","b","c"])
    result = run_cmd "--init"
    configfile = "./testconfig/.erln8.d/config"
    config = File.read(configfile)
    File.open(configfile, "w") do |file|
      file.puts config.gsub("https://github.com/erlang/otp.git","./repo_a")
    end
    result = run_cmd "--clone default"
    assert File.exist?("./testconfig/.erln8.d/repos/default/.git")
  end


  def test_ini
    TestRepo.new("repo_a", ["a","b","c"])
    result = run_cmd "--init"
    configfile = "./testconfig/.erln8.d/config"
    ini = IniParse.parse(File.read(configfile))
    assert ini["Repos"]["default"] == "https://github.com/erlang/otp.git"
    assert init["Erln8"]["color"] == "false"
    assert init["Erln8"]["banner"] == "false"
  end

  def test_ini
    TestRepo.new("repo_a", ["a","b","c"])
    result = run_cmd "--init"
    configfile = "./testconfig/.erln8.d/config"
    ini = IniParse.parse(File.read(configfile))
    ini["Repos"]["default"] = "./repo_a"
    ini.save(configfile)
  end

  def test_other_clone
    TestRepo.new("repo_a", %w[a b c])
    TestRepo.new("repo_b", %w[d e f])
    result = run_cmd "--init"

    configfile = "./testconfig/.erln8.d/config"
    ini = IniParse.parse(File.read(configfile))
    ini["Erln8"]["color"] = "false"
    ini["Repos"]["default"] = "./repo_a"
    ini["Repos"]["test_repo_b"] = "./repo_b"
    ini["Repos"]["test_repo_c"] = "./repo_c"
    #puts ini.to_ini
    ini.save(configfile)

    #@@teardown = false
    result = run_cmd "--clone default"
    assert File.exist?("./testconfig/.erln8.d/repos/default/.git")
    result = run_cmd "--clone test_repo_b"
    assert File.exist?("./testconfig/.erln8.d/repos/test_repo_b/.git")
    result = run_cmd "--clone test_repo_c"
    assert_equal("fatal: repository './repo_c' does not exist\n",result)
    result = run_cmd "--clone test_repo_d"
    assert_equal("ERROR: Unknown repository test_repo_d\n", result)
  end





  def run_cmd(cmd)
    c = "ERLN8_HOME=./testconfig ../erln8 #{cmd} 2>&1"
    #puts c
    `#{c}`
  end
end
