avr-emu [![Build Status](https://travis-ci.org/cemeyer/avr-emu.png?branch=master)](https://travis-ci.org/cemeyer/avr-emu)
===================

This is an AVR emulator. It is a work in progress, and is not expected to be
very good.

Alternatives
============

Several AVR emulators exist. The ones I discovered are IMAVR, GNU AVR,
SimulAVR, and simavr. IMAVR has unclear licensing and poor code quality. GNU
AVR is fairly old GUI simulator. It's not clear it would work well for my
purposes. simavr is a much newer project, started in 2009-2010; unfortunately
it is GPLv3 and seems to have constant bugs (evidenced by the Github issues
list). Simulavr is an older project, but has also seen recent work (as recently
as 2014). It is GPLv2+.

Building
========

You will need `glib` and its development files (specifically, package config
`.pc` files) installed. On Fedora, you can install these with `yum install
glib2 glib2-devel`. On Ubuntu, use `apt-get install libglib2.0-0
libglib2.0-dev`.

`make` will build the emulator, `avr-emu`.

This is not packaged for installation at this time. Patches welcome.

Simple Emulation
================

Invoke `avr-emu <romfile>`.

Tracing
=======

Use the `-t=TRACE_FILE` option to `avr-emu` to log a binary trace of all
instructions executed to `TRACE_FILE`. Use the `-x` flag to dump in hex format
instead of binary.

GDB: Installing avr-gdb
==========================

avr-gdb is `gdb` targetted at remote debugging AVR binaries.

On Fedora Linux, simply install `avr-gdb`.

GDB: Debugging Emulated ROMs
============================

Invoke `avr-emu -g <romfile>` to wait for GDB on startup. The emulator binds
TCP port 3713 and waits for the first client to connect. Use `avr-gdb` from
another terminal to connect with:

    avr-gdb -ex 'target remote localhost:3713'

Supported commands are:
* reading/writing registers
* reading/writing memory
* (instruction) stepping, reverse-stepping
* breakpoints, continue

TODO:
* Memory watchpoints
* reverse-continue

GDB: Reverse debugging
======================

In gdb, you can use `reverse-stepi` (or `rsi` for short) to step backwards.

See https://github.com/cemeyer/msp430-emu-uctf#gdb-reverse-debugging for a
short example of the mechanism.

License
=======

cemeyer/avr-emu is released under the terms of the MIT license. See LICENSE.
Basically, do what you will with it.

Hacking
=======

Try it out! Let me know what you don't like; send me patches, or file issues. I
can't promise I'll fix anything quickly, but I'd rather know what's wrong.

Style: The C sources attempt to follow BSD's
[style(9)](http://www.freebsd.org/cgi/man.cgi?query=style&sektion=9). Style fix
patches are welcome.

Most of the emulator lives in `main.c`. Most of the GDB remote stub lives in
`gdbstub.c`. There are instruction emulation unit tests in `check_instr.c`.
