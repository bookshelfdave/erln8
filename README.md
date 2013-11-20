erln8 - an Erlang version manager
=====

## What is it?

erln8 (erl-in-ate) allows you to compile and manage multiple versions of Erlang from source. Instead of downloading individual source packages, it downloads the Github OTP mirror so you are essentially downloading all available versions at once  :-) Additionaly, you can add your own git repos to support custom OTP patches etc.

The cool thing about erln8 is that you only need to add it to your PATH to use it. No funny environment tweaking ;-) It works by reading an `erln8.config` config file out of the current directory, or by searching up the directory tree until it finds one. 

**slightly outdated**
- Watch my ~1 minute, low-tech demo of how it works [here](https://vimeo.com/78917182): 

## Status

- only supports erl, erlc, escript commands from Erlang dist
	- if there are any other binaries you need from an Erlang distro, just symlink the binary to erln8 (see the Makefile for an example of how this works)
	- I'm currently working on this
- add repo-add, repo-rm, config-add, config-rm commands
- some TODO's in the source
- planning on adding a command to output something you can use in your prompt
- working on a ncurses based *gui* to allow you to quickly support common tasks


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
ern8 --buildable
```

or

```
ern8 --buildable --repo default
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



##Linking an Existing Erlang

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
