PSL1GHT
=======

PSL1GHT is a lightweight PlayStation 3 homebrew SDK that uses the open-source
PlayStation 3 toolchains to compile user applications that will run from the
XMB menu (GameOS homebrew).


Credits
-------

    AerialX     - Founder, Author
    Parlane     - Author
    phiren      - Author, Awesome, yet humble guy
    Tempus      - PSL1GHT Logo
    lousyphreak - libaudio
    Hermes	    - Some modifications, sysmodule, libpngdec
	BigBoss     - EyeToy support added to libcamera sample.
	ooPo        - ps3libraries


Environment
-----------

A GCC toolchain that supports the PowerPC 64bit architecture is required to
build PSL1GHT and its samples. It also requires the toolchain to provide
a full newlib environment; at the moment only two toolchains do so:

* [ps3chain](http://github.com/HACKERCHANNEL/ps3chain)
* [ps3toolchain](http://github.com/ooPo/ps3toolchain)

The SDK also includes a few standalone tools to help compilation. A host gcc
is required to build raw2h, ps3load, and sprxlinker requires libelf. Python 2.x
is required to run fself.py, sfo.py, and pkg.py.


Building
--------

Run make install in the psl1ght directory to build it all, and make sure to
set the environment variable $PSL1GHT to the folder where you wish to
install it to, for example...

    cd /path/to/psl1ght.git/psl1ght/
    export PSL1GHT=/path/to/psl1ght.git/psl1ght/build
    make
    make install

... for a local build of it. Ensure that $PSL1GHT is set when you are
building any of the examples or other apps that use PSL1GHT.


Current Status
--------------

### Graphics

Currently, PSL1GHT supports a double buffered framebuffer directly in the 
RSX's memory for 2D graphical display.
3D support is not currently implemented.

### Input

PS3 controllers are fully supported, and pressing the PS button brings up the
in-game XMB menu, assuming the framebuffer is working.

But you can't currently quit from the xmb without your ps3 giving up and rebooting.

### Filesystem Access

Full filesystem support is available, with access to the internal PS3 hard
drive, game disc contents, and external devices like USB drives. Only stat,
fstat, and directory iteration is missing, though it can be done using the lv2
filesystem interface directly (see include/psl1ght/lv2/filesystem.h)

### Networking

Berkeley sockets are available for use in PSL1GHT, though some of the
implementation remains incomplete at this time (hostname lookups, for example).

### STDOUT Debugging

By default, PSL1GHT applications redirect stdout and stderr to the lv2 TTY
interface. Kammy's ethdebug module can be used to retrieve this live debugging
information over UDP broadcast packets.
See [Kammy](http://github.com/AerialX/Kammy) for more information and a
precompiled ethdebug hook loader.

### SPRX Linking

Any dynamic libraries available to normal PS3 applications can be used with
PSL1GHT, they just need to be made into a stub library and have the exports
filled out. See any of the examples in psl1ght/sprx for information on the
creation of SPRX stub libraries.

The following libraries are currently supported:

* libio
    * libpad
    * libmouse
* liblv2
* libsysutil
* libgcm_sys
* libsysmodule
* libpngdec