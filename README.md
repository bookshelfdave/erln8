erln8 - an Erlang version manager
=====

## What is it?

erln8 (erl-in-ate) allows you to compile and manage multiple versions of Erlang from source. Instead of downloading individual source packages, it downloads the Github OTP mirror so you are essentially downloading all available versions at once  :-) Additionaly, you can add your own git repos to support custom OTP patches etc.

The cool thing about erln8 is that you only need to add it to your PATH to use it. No funny environment tweaking ;-) It works by reading an `erln8.config` config file out of the ***current directory***, or by ***searching up the directory tree until it finds one.*** This allows you to "set a version of Erlang to use for a project and forget it"."


## Status

[![Build Status](https://travis-ci.org/metadave/erln8.png)](https://travis-ci.org/metadave/erln8)

- somewhat stable, I'm sure there are bugs.

## Installation


***NOTE:*** erln8 version 0.9.0 breaks backwards compatibility. You'll need to `rm -rf ~/.erln8.d` and reinitialize erln8.

### OSX
```
brew install glib git
```

See the [Building][building] section below to continue.

### Ubuntu

(Erlang + *general* erln8 deps)

```
sudo apt-get install build-essential libncurses5-dev openssl libssl-dev fop xsltproc unixodbc-dev build-essential libglib2.0-dev git
```

See the [Building][building] section below to continue.

### Fedora

(Erlang + *general* erln8 deps)

```
sudo yum install gcc glibc-devel make ncurses-devel openssl-devel autoconf glib2-devel.x86_64 git
```

this can be helpful as well:
```
yum groupinstall "Development tools"
```

See the [Building][building] section below to continue.

### Building
[building]:

```
 # remove Erlang from your path!!

git clone https://github.com/metadave/erln8.git
cd erln8
make
sudo make install
 # the default location is /usr/local/bin/erln8
 # OR
make DESTDIR=/some/path install
```

## Dependencies
* git
* glib-2.0 
* gio-2.0

## Quickstart

To create erln8 config files, clone the default OTP repo, and build Erlang R16B02, simply run:

```
erln8 --quickstart
```

Depending on your system, it could take quite awhile to download the OTP Git repository and build Erlang. It might be a nice time to get a beer/coffee/tea/Tab. 

**Note**: there aren't any options passed to the `configure` script during build when  --quickstart is used. On OSX, this defaults to a measly 32-bits install.


## Initial setup

***If you haven't added the erln8 directory to your path, do it now. If you already have Erlang in your path, REMOVE IT!**

Run this once, it creates `~/.erln8.d` and `~/.erln8.d/config`

```
erln8 --init
```

Next, you'll need to clone the OTP source repo from Github if you don't have Erlang installed. This default repo is cloned to to `~/.erln8.d/repos/default`.

```
erln8 --clone default
```

To see a list of versions you can build, run:

```
ernl8 --buildable
```

or

```
ernl8 --buildable --repo default
```
replacing `default` with whatever erln8-configured Git repo you want to use.


If you already have Erlang installed, skip down to Linking an Existing Erlang below.
The following command will build Erlang R16B02 using the OTP_R16B02 tag from Git. This build can now be referred to by the --id value (see the erln8 --use example below). Also, see the ~/.erln8.d/config file for specific configs or add your own, or run erln8 --configs.

```
  erln8 --build --tag OTP_R16B02 --id r16b02 --config osx_llvm
  erln8 --build --tag OTP_R15B01 --id r15b01 --config osx_llvm
```

You can specify alternate Git repos to build from. `erln8 --repos` lists available repos or look in ~/.erln9.d/config

```
  erln8 --build --repo basho --tag OTP_R15B01 --id r15b01p --config osx_llvm
```

##Linking an existing version Erlang

If you already have a version of Erlang build at an alternate location, you can *link* erlang to it.

For example, I have an old copy of Erlang R14B04 located in `/Users/dparfitt/erlang_R14B04`, and I'd like to reference it by the ID `R14B04`:

```
erln8 --link /Users/dparfitt/erlang_R14B04 --id R14B04
```

You'll be able to run `erln8 --list` and see the linked version:

```
R16B02b3:slag:~$ erln8 --list
R16B02b3 -> /Users/dparfitt/.erln8.d/otps/R16B02b3
R16B02 -> /Users/dparfitt/.erln8.d/otps/R16B02
R14B04 -> /Users/dparfitt/.erln8.d/otps/R14B04
```


If I want to *unlink* this version of Erlang:
version of Erlang:

**NOTE:** This does ***NOTE*** remove the Erlang directory that erln8 has linked to.

```
erln8 --unlink --id R14B04
``` 

at which point, R14B04 won't show up as a version of Erlang to use.
```
R16B02b3:slag:~$ erln8 --list
R16B02b3 -> /Users/dparfitt/.erln8.d/otps/R16B02b3
R16B02 -> /Users/dparfitt/.erln8.d/otps/R16B02
```

## Usage

To see which versions of Erlang are available for use from erln8, run:

```
  erln8 --list
```

For any directory that has an Erlang project, you can run erln8 with the `--use `option to *mark that directory and all subdirectories to use a specific version of Erlang WHENEVER YOU CD INTO THAT DIRECTORY.*

```
 erln8 --use r16b02
 ```

If you need to change the version of Erlang that's already been configured for the current directory:
```
 erln8 --use r16b01 --force
 ```


If you want to find out which version of Erlang the cwd is using, 

```
erln8 --show
```


This command simply creates an `erln8.config` file in the cwd. You can even edit it by hand to specify the configured version of Erlang by **id**, or just rerun `erln8 --use some_other_version` to change the value.

```
[0]:prime:~$ cat erln8.config
[Config]
Erlang=r16b02
```

## Shell Completion

erln8 provides shell completion for commands, available Erlang installations, and buildable Erlang installation. 

	source ~/path_to_erln8/bash_completion/erln8

## Setting up a default version to use

Simply run `erln8` from your home directory with a version of Erlang that you'd like to use elsewhere. 

**NOTE**: if you are building/running Erlang source from outside of your home directory, you'll need to run erln8 at a location higher up the directory tree.

## Keeping your source repos up-to-date

To get the latest OTP tags/branchs, you'll need to issue a fetch:

```
erln8 --fetch default
```
where `default` is the repo name. If you have multiple repositories configured, you'll need to issue fetch once per repo.


## Customizing your shell prompt

* Don't customize your prompt until you have erln8 fully installed.

The `--prompt` parameter displays the configured version of Erlang for the current working directory. 

```
erln8 --prompt
```

For example, if you are using *bash*, you could add the following snippet to your `.bash_profile`:

```
function erl_version() {
  erln8 --prompt
}

 # note, this also displays git info for the cwd
PS1='\[$(tput bold)\]\[$(tput setaf 2)\]$(erl_version)\[$(tput sgr0)\]:\[\e[0;36m\]$(__git_ps1 "[%s]")\[\e[m\]\[$(tput setaf 3)\]\h\[$(tput sgr0)\]:\w\$ '; [[ $EUID == 0 ]] &&
PS1='\[$(tput bold)\]\[$(tput setaf 2)\]$(erl_version)\[$(tput sgr0)\]:\[\e[0;36m\]$(__git_ps1 "[%s]")\[\e[m\]\[$(tput setaf 3)\]\h\[$(tput sgr0)\]:\w\$ '

```

![foo](https://www.evernote.com/shard/s55/sh/937bb22f-ac98-455b-8e6f-8367caf6fdea/0cd1beba8166544470fef351cb614c7e/deep/0/Shell.png)

## Specifying a build config

erln8 allows you to pass arbitrary flags to the `configure` script as part of a **config**. To add a config, edit `~/.erln8.d/config` and append a ***name=configstring*** to the `[Configs]` section.

##### *Example:*

```
[Configs]
osx_llvm=--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit
```

To specify environment variables to be used during a build, create a config ***name=configstring*** as show above, as well as an additional ***name_env=vars*** setting. Any config that has an accompanying *

##### *Example:*

```
[Configs]
osx_gcc=--disable-hipe --enable-smp-support --enable-threads --enable-kernel-poll --enable-darwin-64bit
osx_gcc_env=CC=gcc-4.2 CPPFLAGS='-DNDEBUG' MAKEFLAGS='-j 3'
```

## Specifying a *default* build config

If `--config` isn't specified as a parameter to a `--build`, erln8 will use the `default_config` config variable specified in the `Erln8` section of `~/.erln8.d/config. This is simply the name of a config as configured above.

```
[Erln8]
default_config=default
```

For example, if `--config` isn't specified as a parameter to a `--build`, the config `osx_llvm` will be used:

```
[Erln8]
default_config=osx_llvm
```


## erln8 config file format

* comments begin with the `#` character and may appear anywhere in the file
* all keys and values are **case sensitive**

####See also:

0. https://developer.gnome.org/glib/2.38/glib-Key-value-file-parser.html
1. http://freedesktop.org/wiki/Specifications/desktop-entry-spec/

## Disabling color and/or the erln8 startup banner

To disable the erln8 startup banner, change the appropriate config settings located in the `[Erln8]` section of `~/.erln8.d/config`. Boolean values are either `true` or `false`, and are case sensitive.

* **banner**
	* Show the version of Erlang that erln8 is running upon startup
	* `true` or `false`
	
* **color**
	* `true` or `false`

#### Example:

```
[Erln8]
banner=false
color=false
```


#Contributing

Fork this repo, create a feature branch using something like this:
	
	git checkout -b branch_name
	
and submit a pull request. 

Please send me an email (dparfitt at basho dot com) and let me know if you want to work on any features.

Only friendly pull requests accepted.

#License

http://www.apache.org/licenses/LICENSE-2.0.html

---

© 2013 Dave Parfitt
