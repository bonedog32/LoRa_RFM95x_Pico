# LoRa_RFM95x_Pico
Using the LoRa RFM95x Breakout Board on Raspberry Pico/Pico2 with C++ ONLY
-------------------------
This project was developed for my own need for a set of libraries, functions, and code to use the HopeRF RFM95x module mounted on a breakout board using only C++. Inspiration was gleaned from [RadioHead](https://www.airspayce.com/mikem/arduino/RadioHead/index.html) libraries and a port of [raspi-lora](https://pypi.org/project/raspi-lora/) for microPython. I verified the hardware worked using the microPython code before developing the software.

The software requires the latest PICO-SDK environment installed, which includes support for Pico, Pico2 and Pico_w. There are many places on the Internet which describe installing the PICO-SDK toolchain.

The CMakeFiles.txt requires this line at the top: set(PICO_BOARD pico2) # (options: pico2, pico_w, pico) to denote which Pico is being used. Initial development was performed on a Raspberry PI 5, but should work on PI 4 also. With the appropriate prerequisites installed, it also works on Fedora 42.

For ease of use and necessity, Client and Server software are in different subdirectories. To simplify maintenance, I placed the files RFM95.h, RFM95fun.cpp and RFM95reg.h one level higher, with soft links from the Client and Server directories for those three files.

Page number references are from the [Low Power Long Range Transceiver Module Model No.:RFM95W/96W/98W](https://hoperf.com) document, Version 2.0.
