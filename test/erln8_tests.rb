require "test/unit"

class Erln8Test < Test::Unit::TestCase
  def mkrepo
    `mkdir bar`
    `rm -rf ./foo`
    `mkdir foo`
    program =
"#include <stdio.h>
int main(int argc, char* argv[]) {
  printf(\"Erln8 Test\");
  return 0;
}"
    makefile =
"
all:
\tgcc -o erln8_test erln8_test.c
"

   configure =
"
#!/bin/sh
echo \"Fake configure\"
"
File.open("./foo/erln8_test.c", 'w') { |file| file.write(program) }
File.open("./foo/Makefile", 'w') { |file| file.write(makefile) }
File.open("./foo/configure", 'w') { |file| file.write(configure) }
    `cd ./foo && git init && git add . && git commit -m \"initial commit\"`
    `cd ./foo && git tag test_a`
    `cd ./foo && git tag test_b`
    `cd ./foo && git tag test_c`
    `chmod 755 ./foo/configure`
  end

  def setup
    mkrepo
  end

  def teardown
    `rm -rf ./foo`
    `rm -rf ./bar`
  end

  def test_init
    result = run_cmd "--init"
    lines = result.split("\n")
    assert_equal("Creating erln8 config file: ./bar/.erln8.d/config", lines[0])
    assert File.exist?("./bar/.erln8.d/config")
    assert File.exist?("./bar/.erln8.d/repos")
    assert File.exist?("./bar/.erln8.d/logs")
    assert File.exist?("./bar/.erln8.d/otps")
  end

  def test_default_clone
    result = run_cmd "--init"
    configfile = "./bar/.erln8.d/config"
    config = File.read(configfile)
    File.open(configfile, "w") do |file|
      file.puts config.gsub("https://github.com/erlang/otp.git","./foo")
    end
    result = run_cmd "--clone default"
    assert File.exist?("./bar/.erln8.d/repos/default/.git")
  end

  def run_cmd(cmd)
    c = "ERLN8_HOME=./bar ../erln8 #{cmd} 2>&1"
    #puts c
    `#{c}`
  end

end
