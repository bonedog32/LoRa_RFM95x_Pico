cmake_minimum_required(VERSION 3.13..3.28)
set(PICO_BOARD pico2) # needed for Pico 2 (options: pico2, pico_w, pico)
include(pico_sdk_import.cmake)
# In .bashrc: export PICO_SDK_PATH=/home/sam/pico1/pico-sdk

# Name the project here
project(loraServer C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME}
        server.cpp
	RFM95fun.cpp
        )

target_sources (${PROJECT_NAME}
        PUBLIC 
        #${CMAKE_CURRENT_LIST_DIR}/filename.cpp
        #${CMAKE_CURRENT_LIST_DIR}/filename.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/  
)

# Pull in our dependencies
target_link_libraries(${PROJECT_NAME}
    pico_stdlib 
    hardware_gpio 
    hardware_spi
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
#pico_enable_stdio_uart(${PROJECT_NAME} 0)
