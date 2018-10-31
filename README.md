[![Build Status](https://travis-ci.com/helium/module-sdk.svg?token=35YrBmyVB8LNrXzjrRop&branch=master)](https://travis-ci.com/helium/module-sdk)

# Helium Module SDK

This directory contains libraries and example applications for developing
Helium Apps that sit above the kernel.


# Installing Depedencies

## Windows 10 (64-bit)
  
  Install [Cygwin](https://cygwin.com/install.html) so that Makefiles work. Cygwin has lots of things to install, 
  <a href="https://s3.us-east-2.amazonaws.com/helium-module-sdk/cygwin.png" target="_blank">but all you need is Make</a>.
  
  Download [TI XDS Emulation Package](http://software-dl.ti.com/dsps/forms/self_cert_export.html?prod_no=ti_emupack_setup_8.0.803.0_win_32.exe&ref_url=http://software-dl.ti.com/dsps/dsps_public_sw/sdo_ccstudio/emulation) so that we can communicate to the LaunchXL over USB.
  
  Download [this zipfile](https://s3.us-east-2.amazonaws.com/helium-module-sdk/module-sdk-deps.zip) which contains OpenOCD, the ARM GNU toolchain, and elf2tab. 
  
  The zipfile has a script called `set_paths.ps1` to help, but you can manually add the three paths to your environment's Path:
  * `[PATH_TO_DIRECTORY]\cargo\bin`
  * `[PATH_TO_DIRECTORY]\gnu_arm_embedded\7 2018-q2-update\bin`
  * `[PATH_TO_DIRECTORY]\OpenOCD\0.10.0-9-20181016-1725\bin`
  
  If you haven't done it yet, `C:\cygwin64\bin` should also be added to Paths (assuming that's where you installed Cygwin).
  
  NOTE: to use the script, you need to enable Powershell scripts. <a href="https://s3.us-east-2.amazonaws.com/helium-module-sdk/powershell.png" target="_blank">Open up Powershell as admin</a>, and type `Set-ExecutionPolicy Unrestricted`. Now you can execute the `set_paths.ps1` which will put all four paths in your environment path.
  
  Install Tockloader (depends on [Python3](https://www.python.org/downloads/release/python-371/)):
  ```
  $ pip3 install tockloader
  ```
  Proceed to the Getting Started section.

## MacOS

  Install the cross compiler for embedded targets: `arm-none-eabi-` (depends on [homebrew](https://brew.sh/))
  ```
  $ brew tap ARMmbed/homebrew-formulae && brew update && brew install arm-none-eabi-gcc
  ```
  Install the most recent version of openocd:

  ```
  $ brew install openocd --HEAD
  ```
  Download [TI XDS Emulation Package](http://software-dl.ti.com/dsps/forms/self_cert_export.html?prod_no=ti_emupack_setup_8.0.803.0_osx_x86_64.app.zip&ref_url=http://software-dl.ti.com/dsps/dsps_public_sw/sdo_ccstudio/emulation) so that we can communicate to the LaunchXL over USB.
  
  Install [cargo/Rust](https://doc.rust-lang.org/cargo/getting-started/installation.html); a compilation utility is managed by it. 
  
  Install Tockloader (depends on Python3):
  ```
  $ pip3 install tockloader
  ```
  Proceed to the Getting Started section.
  
## Debian/Ubuntu (64-bit)

  Install the cross compiler for embedded targets: `arm-none-eabi-`.
  ```
  $ sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa && sudo apt update && sudo apt install gcc-arm-embedded
  ```
  You can either download and compile the most recent OpenOCD or you can use [this prebuilt verison](https://s3.us-east-2.amazonaws.com/helium-module-sdk/openocd_linux_64.tar.xz).

  Download [TI XDS Emulation Package](http://software-dl.ti.com/dsps/forms/self_cert_export.html?prod_no=ti_emupack_setup_8.0.803.0_linux_x86_64.bin&ref_url=http://software-dl.ti.com/dsps/dsps_public_sw/sdo_ccstudio/emulation) so that we can communicate to the LaunchXL over USB.
  
  Install [cargo/Rust](https://doc.rust-lang.org/cargo/getting-started/installation.html); a compilation utility is managed by it. 
  
  Install Tockloader (this requires Python3):

  ```
  $ pip3 install tockloader --user
  
  ```
  Proceed to the Getting Started section.
  
# Getting Started

  ## Compile
  
  Changing directory until you are in `module-sdk/examples/blink`. If `make` starts building then ARM GNU is properly setup. Success here verifies that make, arm-none-eabi, and elf2tab are properly installed.
   
  ## Upload
  
  From the same directory, type `tockloader install --board launchxl-cc26x2r1 --openocd`. Even without hardware, you can test this and you will simply see openocd fail to reach the debugger. If you have hardware, you should see blinking LEDs! Success here verifies that tockloader and openocd are propery installed.  
