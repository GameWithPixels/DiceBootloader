# DiceBootloader

## Overview

When flashed on the die's memory, this program is the [first](https://en.wikipedia.org/wiki/Bootloader) piece of software to run.
It configures memory system and a few pins of the [microcontroller](https://en.wikipedia.org/wiki/Microcontroller).

Then, it checks for a few seconds if any firmware update request is being made via Bluetooth. If so it proceeds with the update request, and if not it moves on and launch the main program (the die's [firmware](https://github.com/GameWithPixels/DiceFirmware)).

This program is based on Nordic Semiconductor nRF5 SDK Bootloader example. See [here](https://infocenter.nordicsemi.com/topic/sdk_nrf5_v17.0.0/lib_bootloader.html) for more details about the functionalities of the bootloader.

Nordic's SDK, *Make* and *GCC* are required to build the bootloader.

*Note:* currently all our dice come equipped with a [nRF52810](https://www.nordicsemi.com/Products/nRF52810) chip from Nordic.

## Building The Bootloader

### Environment Setup on Windows

#### Nordic SDK 17

At the time of writing, version 17.0.2 is used, but any 17 release should work.

We need Nordic's library to use the Pixel's microcontroller features including Bluetooth Low Energy (BLE).
This library is named [SoftDevice](https://infocenter.nordicsemi.com/topic/ug_gsg_ses/UG/gsg/softdevices.html).

Pixels have a limited amount of memory so we want to use the *SoftDevice* with the smallest memory footprint and with BLE support. S112 is our best choice.

1. Go to Nordic's SDK download [page](https://www.nordicsemi.com/Products/Development-software/nRF5-SDK/Download#infotabs).
2. Select the latest 17.x SDK release
3. Scroll down and optionally unselect all SoftDevices (they are included in the SDK anyways)
4. Scroll down to the bottom of the page, it displays the list of selected packages: you should have at least `nRF5 SDK 17`
5. Download and extract the contents of the `nRF5 SDK 17` zip file to your machine, for example to `C:\nRF5_SDK`

From now on, we'll refer to the SDK install folder as `SDK_ROOT`.

We'll compile the SDK once *GCC* and *Make* are installed.

#### GCC and Make

Nordic lists which toolchains they support in their release notes.

Open `SDK_ROOT/documentation/release_notes.txt` and search for *GCC*. It should read something like `GCC: GCC ARM Embedded 9.2019q4.major`.

The Arm Developer [website](https://developer.arm.com) provides the *GCC* [version](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/9-2019-q4-major) we need.

Download the "Windows 32-bit Installer" and run it. Be sure to check "Add Path" during the installation process.

Go to "Make for Windows" official [website](http://gnuwin32.sourceforge.net/packages/make.htm#download).
Download the "Complete package, except sources" and run the installer. Yes, it dates from 2006!

Alternatively you can use [Chocolatey](https://community.chocolatey.org/packages/make) to install a more recent version of *GNU Make*.

Once the setup is complete, be sure to add the location of *Make* to your system `PATH`. The default installation folder is `C:\Program Files (x86)\GnuWin32\bin`.

#### uECC

Clone (or copy) the [micro-ecc](https://github.com/kmackay/micro-ecc) encryption module sources into `SDK_ROOT/external/micro-ecc/micro-ecc`.

Then run `SDK_ROOT/external/micro-ecc/build_all.bat`.
We recommend using the command line rather than directly running the batch file so the results will stay on screen and can be checked. There should be no error.

*Note:* For more information, see Nordic's documentation [page](https://infocenter.nordicsemi.com/topic/sdk_nrf5_v17.0.2/lib_crypto_backend_micro_ecc.html)
on uECC and a blog [entry](https://devzone.nordicsemi.com/guides/short-range-guides/b/software-development-kit/posts/getting-started-with-nordics-secure-dfu-bootloader)
about generating encryption keys for then secure bootloader and building a firmware package.

### Building

Make sure that the *Makefile* `SDK_ROOT` variable is pointing to the correct folder.

Open a command line and go the folder where this repository is cloned and run `make`.

The output files are placed in the `_build` folder, by default those are release files (not debug).
The one that we want to program to the flash memory is the `.hex` file (more about this format on [Wikipedia](https://en.wikipedia.org/wiki/Intel_HEX)).

## Programming a die (with USB)

When shipping out from the factory the dice electronic boards have no software on it.
The only way to program them at this point is with a wired connection.

For that we're using a [J-Link](https://www.segger.com/products/debug-probes/j-link/models/j-link-plus/) debug probe from Segger.

### Environment Setup on Windows

Nordic's Bluetooth Low Energy development tools are available [here](https://www.nordicsemi.com/Products/Bluetooth-Low-Energy/Development-tools#infotabs).

Download and install "nRF Command Line Tools" for Win32 (version 10.12.2 at the time of writing).
During the installation process, you should be prompted to also install the J-Link drivers.

Optionally you may want to download and install "nRF Connect for Desktop", a suite of visual tools to work with Nordic's BLTE microcontrollers.

We like to use the *Programmer* tool to inspect and sometimes reprogram the flash memory.

### Program the flash memory

Using the provided *Makefile* you may:

* `reset`: restart the device
* `erase`: entirely erase the flash memory
* `flash`: program the bootloader into the die's memory and reboot the device
* `flash_softdevice`: program the *SoftDevice* into the die's memory and reboot the device
* `reflash`: call `erase`, `flash` and `flash_softdevice` in a sequence

The latter is the most useful command.

Example:
```
make reflash
```
