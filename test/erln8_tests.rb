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
      pwd = `pwd`.strip()
      testdir = "#{pwd}/testconfig"
      makefile =
      "all:
\tgcc -o erln8_test erln8_test.c
\tgcc -o erl erln8_test.c
\tgcc -o erlc erln8_test.c
install:
\tmkdir -p #{testdir}/.erln8.d/otps/foo/dist/bin
\tcp erl #{testdir}/.erln8.d/otps/foo/dist/bin
\tcp erlc #{testdir}/.erln8.d/otps/foo/dist/bin
  "

    configure =
      "#!/bin/sh
echo \"Fake configure\"
  "

    otpbuild =
      "#!/bin/sh
echo \"Fake otp_build\"
  "
      File.open("./#{root}/erln8_test.c", 'w') { |file| file.write(program) }
      File.open("./#{root}/Makefile", 'w') { |file| file.write(makefile) }
      File.open("./#{root}/otp_build", 'w') { |file| file.write(otpbuild) }
      File.open("./#{root}/configure", 'w') { |file| file.write(configure) }
      `chmod 755 ./#{root}/configure`
      `chmod 755 ./#{root}/otp_build`

      `cd ./#{root} && git init && git add . && git commit -m \"commit #{tag}\"`
      `cd ./#{root} && git tag #{tag}`
    end
  end
end

class Erln8Test < Test::Unit::TestCase

  @@testhome = `pwd`.strip
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
    d = `pwd`.strip()
    assert_equal("Creating erln8.config file: #{d}/testconfig/.erln8.d/config", lines[0])
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

    result = run_cmd "--clone default"
    assert File.exist?("./testconfig/.erln8.d/repos/default/.git")
    result = run_cmd "--clone test_repo_b"
    assert File.exist?("./testconfig/.erln8.d/repos/test_repo_b/.git")
    result = run_cmd "--clone test_repo_c"
    l = result.split("\n")[0]
    assert_equal("fatal: repository './repo_c' does not exist", l)
    result = run_cmd "--clone test_repo_d"
    assert_equal("ERROR: Unknown repository test_repo_d\n", result)
  end

  def test_fetch
    TestRepo.new("repo_a", %w[a b c])
    TestRepo.new("repo_b", %w[d e f])
    result = run_cmd "--init"

    configfile = "./testconfig/.erln8.d/config"
    ini = IniParse.parse(File.read(configfile))
    ini["Erln8"]["color"] = "false"
    ini["Repos"]["default"] = "./repo_a"
    ini["Repos"]["test_repo_b"] = "./repo_b"
    #puts ini.to_ini
    ini.save(configfile)

    ## default repo
    result = run_cmd "--clone default"
    tags = `cd ./testconfig/.erln8.d/repos/default/ && git tag`
    assert_equal(%w[a b c], tags.split("\n").sort)
    `cd ./repo_a && git tag zzz`
    result = run_cmd "--fetch"
    tags = `cd ./testconfig/.erln8.d/repos/default/ && git tag`
    assert_equal(%w[a b c zzz], tags.split("\n").sort)

    ## repo_b
    result = run_cmd "--clone test_repo_b"
    tags = `cd ./testconfig/.erln8.d/repos/test_repo_b/ && git tag`
    assert_equal(%w[d e f], tags.split("\n").sort)

    `cd ./repo_b && git tag xxx`
    result = run_cmd "--fetch --repo test_repo_b"
    tags = `cd ./testconfig/.erln8.d/repos/test_repo_b/ && git tag`
    assert_equal(%w[d e f xxx], tags.split("\n").sort)

    # test for unknown repos
    result = run_cmd "--fetch --repo x"
    assert_equal("ERROR: Unknown repo x\n", result)
  end


  def default_setup
    TestRepo.new("repo_a", %w[a b c])
    result = run_cmd "--init"

    configfile = "./testconfig/.erln8.d/config"
    ini = IniParse.parse(File.read(configfile))
    ini["Erln8"]["color"] = "false"
    ini["Repos"]["default"] = "./repo_a"
    #puts ini.to_ini
    ini.save(configfile)

    run_cmd "--clone default"
  end

  def test_simple_build
    default_setup
    result = run_cmd "--build --id foo --repo default --tag a"
    assert File.exist?("./testconfig/.erln8.d/otps/foo/dist/bin/erl")
  end

  def test_default_repo
    default_setup
    result = run_cmd "--build --id foo --tag a"
    assert File.exist?("./testconfig/.erln8.d/otps/foo/dist/bin/erl")
  end

  def test_default_noid
    default_setup
    result = run_cmd "--build --tag a"
    assert_equal("ERROR: build id not specified", result.split("\n")[0])
  end

  def test_unknown_tag
    default_setup
    result = run_cmd "--build --id foo --tag xyz"
    assert_equal("ERROR: branch or tag xyz does not exist in default Git repo", result.split("\n")[0])
  end

  def test_alt_repo
    TestRepo.new("repo_a", %w[a b c])
    TestRepo.new("repo_b", %w[d e f])
    result = run_cmd "--init"
    configfile = "./testconfig/.erln8.d/config"
    ini = IniParse.parse(File.read(configfile))
    ini["Erln8"]["color"] = "false"
    ini["Repos"]["default"] = "./repo_a"
    ini["Repos"]["repo_b"] = "./repo_b"
    ini.save(configfile)

    run_cmd "--clone default"
    run_cmd "--clone repo_b"

    result = run_cmd "--build --id foo --tag d --repo repo_b"
    assert File.exist?("./testconfig/.erln8.d/otps/foo/dist/bin/erl")
  end

  def test_basic_use
    begin
      default_setup
      result = run_cmd "--build --id builda --repo default --tag a"
      result = run_cmd "--build --id buildb --repo default --tag b"
      `mkdir -p ./foobar1/foo`
      `mkdir -p ./foobar2/foo`

      Dir.chdir "foobar1"
      run_cmd "--use builda"
      output_prompt = run_cmd "--prompt"
      assert_equal("builda", output_prompt)

      output_show = run_cmd "--show"
      assert_equal("builda", output_show.strip)

      Dir.chdir "foo"
      output_prompt = run_cmd "--prompt"
      assert_equal("builda", output_prompt)

      output_show = run_cmd "--show"
      assert_equal("builda", output_show.strip)

      Dir.chdir @@testhome
      Dir.chdir "foobar2"
      run_cmd "--use buildb"
      output = run_cmd "--prompt"
      assert_equal("buildb", output)
    ensure
      Dir.chdir @@testhome
    end
  end

  def run_cmd(cmd)
    c = "ERLN8_HOME=#{@@testhome}/testconfig #{@@testhome}/../erln8 #{cmd} 2>&1"
    `#{c}`
  end

end
