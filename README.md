# woinc - an alternative to boinccmd and boincmgr

woinc is a reimplementation of boinccmd and boincmgr of [BOINC](https://boinc.berkeley.edu/). It's a hobby project to learn modern C++ and Qt (and maybe GTK later on).

It's written in C++-14 implementing its own library for communication with the BOINC clients. The GUI is an Qt5 application.

woinc should be compatible with C++-14 and above.

This project is work in progress, the API may change without any public notices.

# screenshots? screenshots!

They may be not up to date, but there are some [here](http://83.169.22.26/tmp/woinc/) 

## woinc consists of

- **libwoinc**: the core library, implementing the communication with the BOINC clients by abstracting it through [commands](https://en.wikipedia.org/wiki/Command_pattern)
- **libwoincui**: periodic queries and async communication with the clients; supports multiple clients
- **woinccmd**: reimplementation of boinccmd using woinc but not woincui; it's the CLI to the clients
- **woincqt**: reimplementation of boincmgr using woincui and Qt5; not supporting multiple clients yet

## dependencies

### runtime

#### libwoinc
- pugixml >= 1.9: for parsing the XML

#### woincqt
- Qt 5: woinqt is compiling with qt >= 5.9 (maybe before, I don't know); version qt >= 5.12 is supported. Needed Qt components:
    - Qt5Widgets: for the UI
    - Qt5Charts: playing around with statistics, may be made optional in the future
    - Qt5Network: to load images rendered in the news tab, may be made optional in the future

### compiletime

- all of runtime; of course you only need Qt if you want to build woincqt
- some C++-compiler supporting C++-14 standard or above
- cmake >= 3.8
- some tool cmake accepts as generator (make, ninja, ..)
- optional: qttest to run the tests of woincqt

### building woinc
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
    of course there are a lot of compiler options to set:
    ```
    -DCMAKE_INSTALL_PREFIX=/tmp/woinc # the directory to install the compiled files to, e.g. via make install
    -DCMAKE_BUILD_TYPE="release" # let cmake set the compiler options; I'm not a fan of this one, please use the next one
    -DCMAKE_CXX_FLAGS="-march=native -O2 -DNDEBUG" # compiler flags, may also be provided by env CXXFLAGS
    ```
    and some woinc specific flags (TRUE|FALSE instead of ON|OFF is valid also)
    ```
    -DWOINC_BUILD_LIB=<ON|OFF>     # build libwoinc
    -DWOINC_BUILD_LIBUI=<ON|OFF>   # build libwoincui
    -DWOINC_BUILD_CLI_UI=<ON|OFF>  # build woincdmd
    -DWOINC_BUILD_CLI_QT=<ON|OFF>  # build woincqt
    -DWOINC_CLI_COMMANDS<OIN|OFF>  # enable some extra commands in woinccmd
    -DWOINC_EXPOSE_FULL_STRUCTURES # also handle data from the client woinc doesn't need but maybe someone using this lib; off by default
    -DWOINC_BUILD_SHARED_LIBS      # build shared instead of static libs of libwoinc and libwoincqt
    ```
    and for dev
    ```
    -DWOINC_ENABLE_COVERAGE         # tells the compiler to enable coverage
    -DWOINC_ENABLE_SANITIZER        # enabled some sanitizer in the compiler
    -DWOINC_VERBOSE_DEBUG_LOGGING   # enables verbose debug logging
    ```
    example with flags
    ```shell script
      $ git clone https://github.com/vmc-coding/woinc.git woinc.git
      $ mkdir -p woinc.git/build && cd woinc.git/build
      $ cmake -DCMAKE_INSTALL_PREFIX=~/woinc -DCMAKE_CXX_FLAGS="-march=native -O2 -DNDEBUG" ..
      $ make -j $(nproc) install
      $ ~/woinc/bin/woincqt
    ```
    
# Running woinccmd
Sorry, all I have is
```shell script
    $ woinccmd -?
```
No manpage yet. But it's compatible with boinccmd - for the stuff implemented - so you may see their manpage.

# Running woincqt
Just run it and add a client via UI. The application won't remember the user and/or password yet.
Of course that is a really bad way if you've to do it a lot of times,
 e.g. while developing.
So there is a shortcut by assuming the host ist localhost
 and password the first parameter to the program.
This is for dev only, because all other users of your PC may see your password!
