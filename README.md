erln8 - an Erlang version manager
=====

## What is it?

erln8 (erl-in-ate) allows you to compile and manage multiple versions of Erlang from source. Instead of downloading individual source packages, it downloads the Github OTP mirror so you are essentially downloading all available versions at once  :-) Additionaly, you can add your own git repos to support custom OTP patches etc.

The cool thing about erln8 is that you only need to add it to your PATH to use it. No funny environment tweaking ;-) It works by reading an `erln8.config` config file out of the current directory, or by searching up the directory tree until it finds one. 


## Status
- not yet stable, use at your own risk
- link/unlink broken
- fetch broken
- a build generates an error when it completes
- only supports erl, erlc, escript commands from Erlang dist
	- if there are any other binaries you need from an Erlang distro, just symlink the binary to erln8 (see the Makefile for an example of how this works)
	- I'm currently working on this
- add repo-add, repo-rm


## Installation


### OSX
```
brew install glib git
git clone https://github.com/metadave/erln8.git
cd erln8
make
 # remove Erlang from your path!!
export PATH=$PATH:/where/you/put/erln8
```

### Ubuntu
```
apt-get install libglib2.0-dev
git clone https://github.com/metadave/erln8.git
cd erln8
make
 # remove Erlang from your path!!
export PATH=$PATH:/where/you/put/erln8
```

### Fedora

(Erlang + *general* erln8 deps)

```
sudo yum install gcc glibc-devel make ncurses-devel openssl-devel autoconf glib2-devel.x86_64
```

this can be helpful as well:

```
yum groupinstall "Development tools"
```

git clone https://github.com/metadave/erln8.git
cd erln8
make
 # remove Erlang from your path!!
export PATH=$PATH:/where/you/put/erln8

## Dependencies
* glib-2.0 
* gio-2.0

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


### Shell Completion

erln8 provides shell completion for commands, available Erlang installations, and buildable Erlang installation. 

	source ~/path_to_erln8/bash_completion/erln8

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

## Setting up a default version to use

Simply run `erln8` from your home directory with a version of Erlang that you'd like to use elsewhere. 


##Linking an existing version Erlang

I plan on adding `erln8 --link` and `erln8 --unlink` commands, but for now, it's as easy as adding a new entry to the **[Erlangs]** section of `~/.erln8.d/config`. Below, *r16b02*, *r15b01_nosched*, and *r15b03* are ID's that can be referenced by the `erln8 --use` command (also what's shown by `erln8 --list`). 

For example, Erlang R15B03 is added as the last line:

```
[Erlangs]
r16b02=/Users/dparfitt/.erln8.d/otps/r16b02
r15b01_nosched=/Users/dparfitt/.erln8.d/otps/r15b01_nosched
r15b03=/Users/dparfitt/erlang-R15B03
```

R15B03 is now *linked*, you can see it in `erln8 --list`:
```
[0]:prime:~/.erln8.d$ erln8 --list
Available Erlang installations:
  r16b02
  r15b01_nosched
  r15b03
```

and you can use it with `erln8 --use r15b03`



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
