# How to setup the OTA

## Create Server with OTA files
To enable OTA you need to run a server with two files:

### firmware.bin
the binfile that will be installed with OTA. This comes from the build output off PlatformIO in Visual Studio Code for instance.

### bin_version.txt
a simple text file with the bin_version of the binfile. Example: 1.6

## PressPlay setup
For the OTA in PressPlay we used a server running on 10.10.0.2 on port 5001.

This server has the two files that are described above.

### defines in code:
  - #define URL_fw_Version "http://10.10.0.2:5001/bin_version.txt"
  - #define URL_fw_Bin "http://10.10.0.2:5001/firmware.bin"
