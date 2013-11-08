erln8 - an Erlang version manager
=====

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

## Sample usage
erln8 --init
erln8 --clone default
erln8 --build --tag OTP_R16B02 --id r16b02
erln8 --build --tag OTP_R15B01 --id r15b01_nosched
erln8 --build --repo basho --tag OTP_R15B01 --id r15b01p --config osx_llvm
