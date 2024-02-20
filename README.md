# woinc - an alternative to boinccmd and boincmgr

woinc is a reimplementation of boinccmd and boincmgr of [BOINC](https://boinc.berkeley.edu/). It's a hobby project to learn modern C++ and Qt ~~(and maybe GTK later on)~~.

It's written in C++-14 implementing its own library for communication with the BOINC clients. The GUI is an Qt application supporting Qt5 and Qt6.

woinc should be compatible with C++-14 and above. You may wan't to use at least C++-17 when building against Qt6.

~~This project is work in progress, the API may change without any public notices.~~

**Note:**

I'm not interested in this project anymore, so there won't be any active development
except bugfixes and to keep it working with current compilers and dependencies.

## screenshots? screenshots!

They may not be up to date, but there are some [here](http://83.169.22.26/woinc/)

## woinc consists of

- **libwoinc**: the core library, implementing the communication with the BOINC clients by abstracting it through [commands](https://en.wikipedia.org/wiki/Command_pattern)
- **libwoincui**: periodic queries and async communication with the clients; supports multiple clients
- **woinccmd**: reimplementation of boinccmd using libwoinc but not libwoincui; it's the CLI to the clients
- **woincqt**: reimplementation of boincmgr using libwoincui and Qt5; not supporting multiple clients yet

## ~~roadmap~~

~~Version 1 will be the libs and rebuild of boinccmd and boincmgr in Qt
Not supported are some rarely used features like excluding apps or reordering the UI.
There are few things missing, so version 1 will be released in near future.~~

~~Ideas for next milestones in random order:~~
- ~~migrate from C++14 to C++17; I don't care about backwards compatibility, I'm doing this to learn modern C++, so keep updating :)
  but you may contact me, if you're using this software with a compiler or system not supporting C++17~~
- ~~rewrite libwoincui to respect the command pattern:
  in v1 there is a controller class which imports all of libwoinc,
  i.e. the compiler/linker can't optimize away stuff which isn't used,
  which is contrary to how libwoinc had been designed.
  I also don't think the current API is easy to use in external bindings~~
- ~~python bindings~~
- ~~more UI clients, e.g. GTK or maybe as a web app~~
- ~~instead of coding the Qt-GUI in C++, use QML and Qt Quick; implementing it all "by hand" had been exercise for me to learn Qt; but maybe I'll drop Qt because of their license bullshit~~
- ~~get a usable UI running on the Pinephone~~

~~Mail me, if you have opinions on this software or some features you'd like to be implemented.
Or implement them and create a pull request ;)~~

## dependencies

### runtime

#### libwoinc
- [pugixml](https://pugixml.org/) >= 1.9: for parsing the XML

#### woincqt
- Needed Qt components:
    - QtWidgets: for the UI
    - QtCharts: playing around with statistics, may be made optional in the future
    - QtNetwork: to load images rendered in the news tab, may be made optional in the future

- woincqt 1.0.* supports Qt5
    - it's compiling with qt >= 5.9 (maybe before, I don't know); version qt 5.15.* is supported

- woincqt 1.1.* supports Qt5 and Qt6
    - for Qt5: same as above for woincqt 1.0
    - for Qt6: it's compiling with qt >= 6.6 (maybe before, I don't know; it won't build with 6.0 for sure)

### compiletime

- all of runtime; of course you only need Qt if you want to build woincqt
- some C++-compiler supporting C++-14 standard or above
- cmake >= 3.8
- some tool cmake accepts as generator (make, ninja, ..)
- optional: qttest to run the tests of woincqt

## building
- get the source of woinc, e.g. by cloning the repo
    ```shell script
    $ git clone https://github.com/vmc-coding/woinc.git woinc.git
    ```
- create an out of tree build directory
    ```shell script
    $ mkdir -p woinc.git/build && cd woinc.git/build
    ```
- configure the build with cmake (cross compilation not yet tested, I'd help, if asked for)

    basic:
    ```shell script
    $ cmake ..
    $ make
    ```
    for a parallel build add the -j parameter for the last step, e.g.
    ```
    $ make -j4
    # or if nproc is installed:
    $ make -j $(nproc)
    ```
    of course there are a lot of options to set:
    ```
    -DCMAKE_INSTALL_PREFIX=/tmp/woinc # the directory to install the compiled files to, e.g. via make install
    -DCMAKE_BUILD_TYPE="release" # let cmake set the compiler options; I'm not a fan of this one, please use the next one
    -DCMAKE_CXX_FLAGS="-march=native -O2 -DNDEBUG" # compiler flags, may also be provided by env CXXFLAGS
    ```
    and some woinc specific flags (TRUE|FALSE instead of ON|OFF is valid also)
    ```
    -DWOINC_BUILD_LIB=<ON|OFF>              # build libwoinc
    -DWOINC_BUILD_LIBUI=<ON|OFF>            # build libwoincui
    -DWOINC_BUILD_CLI_UI=<ON|OFF>           # build woincdmd
    -DWOINC_BUILD_CLI_QT=<ON|OFF>           # build woincqt
    -DWOINC_CLI_COMMANDS=<ON|OFF>           # enable some extra commands in woinccmd
    -DWOINC_EXPOSE_FULL_STRUCTURES=<ON|OFF> # also handle data from the client woinc doesn't need but maybe someone using this lib; off by default
    -DWOINC_BUILD_SHARED_LIBS=<ON|OFF>      # build shared instead of static libs of libwoinc and libwoincui
    ```
    and for dev
    ```
    -DWOINC_ENABLE_COVERAGE=<ON|OFF>        # tells the compiler to enable coverage
    -DWOINC_ENABLE_SANITIZER=<ON|OFF>       # enables some sanitizer in the compiler
    -DWOINC_VERBOSE_DEBUG_LOGGING=<ON|OFF>  # enables verbose debug logging
    ```
    example with flags
    ```shell script
    $ git clone https://github.com/vmc-coding/woinc.git woinc.git
    $ mkdir -p woinc.git/build && cd woinc.git/build
    $ cmake -DCMAKE_INSTALL_PREFIX=~/woinc -DCMAKE_CXX_FLAGS="-march=native -O2 -DNDEBUG" ..
    $ make -j $(nproc) install
    $ ~/woinc/bin/woinccmd
    ```

### tests
Build and run all tests with the 'check' target:
```shell script
$ make check
```

## Running woinccmd
Sorry, all I have is
```shell script
$ woinccmd -?
```
No manpage yet. But it's compatible with boinccmd - for the stuff implemented - so you may see their manpage.

## Running woincqt
Just run it and add a client via UI. The application won't remember the user and/or password yet.
Of course that is a really bad way if you've to do it a lot of times,
 e.g. while developing.
So there is a shortcut by assuming the host ist localhost
 and password the first parameter to the program.
This is for dev only, because all other logged in users on your computer may see your password!
