## About

`jacknetump` is a Linux daemon designed specifically for the Zynthian Open Source Synthesizer : https://zynthian.org/ but nothing prevents it to be used on any other Linux machine.

The daemon is used by the Zynthian to create the Network UMP endpoint which can be activated via the Zynthian's Admin menu.


## Install

You can download a precompiled Debian package (for Zynthian) from the releases page:

* [Jacknetump releases](https://github.com/oscaracena/jacknetumpd/releases)


## Build

You can compile the daemon in Linux, with GNU Make. Just clone the repo and run `make`:

    git clone --recurse-submodules https://github.com/oscaracena/jacknetumpd.git
    make


## License and authors

This software is under the terms of the MIT License. And these are the authors:

* Source Code: 2023-2024 Benoit BOUCHEZ
* Debian Package: 2025 Oscar Aceña
<!--  -->