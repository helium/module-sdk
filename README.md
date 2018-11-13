[![Build Status](https://travis-ci.com/helium/module-sdk.svg?token=35YrBmyVB8LNrXzjrRop&branch=master)](https://travis-ci.com/helium/module-sdk)

# Helium Module SDK

This repository contains libraries and example applications for developing
Helium Apps which sit above the open-source [TockOS kernel](https://github.com/helium/tock). This Module SDK is a Helium curated fork of Tock's [libtock-c](https://github.com/tock/libtock-c).

# Installing Dependencies

## Windows 10 (64-bit)
    
  Download [TI XDS Emulation Package](http://software-dl.ti.com/dsps/forms/self_cert_export.html?prod_no=ti_emupack_setup_8.0.803.0_win_32.exe&ref_url=http://software-dl.ti.com/dsps/dsps_public_sw/sdo_ccstudio/emulation) so that we can communicate to the LaunchXL over USB.
  
  Download [this zipfile](https://s3.us-east-2.amazonaws.com/helium-module-sdk/module-sdk-deps.zip) which contains OpenOCD, the ARM GNU toolchain, elf2tab, and make.
  
  The zipfile has a script called `set_paths.ps1` to help, but you can manually add the three paths to your environment's Path:
  * `[PATH_TO_DIRECTORY]\cargo\bin`
  * `[PATH_TO_DIRECTORY]\gnu_arm_embedded\7 2018-q2-update\bin`
  * `[PATH_TO_DIRECTORY]\OpenOCD\0.10.0-9-20181016-1725\bin`
  * `[PATH_TO_DIRECTORY]\make\`
  
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
  
  Install [rustup](https://rustup.rs/); a compilation utility is managed by it.
 
  ```
  $ curl https://sh.rustup.rs -sSf | sh
  ```
 
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
  
  Install [rustup](https://rustup.rs/); a compilation utility is managed by it. 
  
  ```
  $ curl https://sh.rustup.rs -sSf | sh
  ```
  
  Install Tockloader (this requires Python3):

  ```
  $ pip3 install tockloader --user
  
  ```
  Proceed to the Getting Started section.
  
# Getting Started

  ## Compile
  
  Changing directory until you are in `module-sdk/examples/blink`. If `make` starts building then ARM GNU is properly setup. Success here verifies that make, arm-none-eabi, and elf2tab are properly installed.
   
  ## Upload
  
  From the same directory, type `tockloader install --board launchxl-cc26x2r1 --openocd './build/blink.tab'`. Even without hardware, you can test this and you will simply see openocd fail to reach the debugger. If you have hardware, you should see blinking LEDs! Success here verifies that tockloader and openocd are propery installed. 

# Common Issues
 
  ## "Command not found" command line response. Application is not in system path.
  
  If you have installed all of the above componenets correctly, check to make sure that the application install directory is in the system path. This can be found by checking either you're .bashrc, .zprofile, or .zshrc file, typically located in your `$HOME` directory. 
  `ls -a ~/` or `ls -a $HOME/` to locate file and view/edit with text editor of your choice (vim, emacs, sublime etc...)
  
  If this is happening when attempting to envoke tockloader, check which python you have installed and where that python version installs executables. If you system has many different python versions, they may not all be listed in your system path.

  ## Example application fails to build with `make` due to elf2tab

  Check that you have installed Rust through Rustup instead of installing from Rust source. Rustup will manage your toolchain and fetch elf2tab when you envoke `make` and rust+cargo seems to fail at this.

  ```
  $ openocd -c "source [find board/ti_cc26x2_launchpad.cfg]; init; halt; flash erase_address 0x30000 0x10000; soft_reset_halt; resume; exit"
  ``` 

# Flashing the kernel
 
  For some reason or another, you may need to reflash the kernel. tools/kernel contains the necessary binaries and some helper scripts for flashing. Running `./flash_kernel.sh` from that directory should be enough.

  On a Windows sytem, you will want to try the command directly
  ```openocd -f flash-kernel.openocd```

# Pinout

| DIO | Signal |
|:--:|:--:|
|DIO12 | UART0_RX |
|DIO13 | UART0_TX |
|DIO21 | UART1_RX |
|DIO11 | UART1_TX |
|DIO22 | I2C0_SCL |
|DIO5 | I2C0_SDA |
|DIO16 | TDO |
|DIO17 | TDI |
|DIO6 | RED_LED |
|DIO7 | GREEN_LED |
|DIO15 | BUTTON_1 |
|DIO14 | BUTTON_2 |
|DIO20 | GPIO0 |
|DIO23 | ADC0 |
|DIO24 | ADC1 |
|DIO25 | ADC2 |
|DIO26 | ADC3 |
|DIO27 | ADC4 |
|DIO18 | PWM0 |
|DIO19 | PWM1 |
|DIO28 | RF24 |
|DIO29 | RFHIPA |
|DIO30 | RFSUBG |
