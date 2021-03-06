cmake_minimum_required(VERSION 3.5)
project(f446_softser)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Add source files
#aux_source_directory(Src/ SOURCE_FILES)
file(GLOB SOURCE_FILES Src/*.c)
file(GLOB HEADER_FILES Inc/*.h)
file(GLOB MAC_SOURCE_FILES mac-software/Src/*.c)
file(GLOB MAC_HEADER_FILES mac-software/Inc/*.h)

add_executable(f446_softser ${SOURCE_FILES} ${HEADER_FILES}
        ${MAC_SOURCE_FILES} ${MAC_HEADER_FILES})
add_executable(cctest App/cctest.c Src/fec.c Src/rs.c App/implx86.c)
add_executable(ptrtest App/ptrtest.c Src/crc16.c App/implx86.c)

# Include directories
include_directories(App)
include_directories(Inc)
include_directories(mac-software/Inc)
include_directories(Drivers/STM32F4xx_HAL_Driver/Inc)
include_directories(Drivers/STM32F4xx_HAL_Driver/Inc/Legacy)
include_directories(Drivers/CMSIS/Include)
include_directories(Drivers/CMSIS/RTOS/Template)
include_directories(Drivers/CMSIS/Device/ST/STM32F4xx/Include)
include_directories(C:\\Users\\Adzil\\Documents\\uVision\ Projects\\f446-softser\\MDK-ARM\\RTE)
include_directories(C:\\Keil_v5\\ARM\\PACK\\ARM\\CMSIS\\4.5.0\\CMSIS\\Include)
include_directories(C:\\Keil_v5\\ARM\\PACK\\ARM\\CMSIS\\4.5.0\\CMSIS\\RTOS\\RTX\\INC)
include_directories(C:\\Keil_v5\\ARM\\PACK\\Keil\\STM32F4xx_DFP\\2.7.0\\Drivers\\CMSIS\\Device\\ST\\STM32F4xx\\Include)

# Definitions for STM32 libraries
add_definitions(-D__UVISION_VERSION="518" -D_RTE_ -DSTM32F446xx
        -DUSE_HAL_DRIVER -DSTM32F446xx)

target_compile_options(ptrtest PUBLIC -fshort-enums)