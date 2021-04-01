cmake_minimum_required(VERSION 3.16)
project(epaper C)

set(CMAKE_C_STANDARD 11)

add_compile_definitions(_DEFAULT_SOURCE)

add_compile_definitions(DEBUG)
#add_compile_definitions(BDF_DEBUG)
#add_compile_definitions(BDF_TEST)
#add_compile_definitions(BDF_CHARNAME)

add_compile_definitions(USE_BCM2835_LIB RPI EPD)
set(EPD_LIB lib/epd/ER-EPD0583-1.c lib/epd/GUI_BMPfile.c lib/epd/GUI_Paint.c lib/epd/Debug.h lib/epd/DEV_Config.c lib/epd/dev_hardware_SPI.c lib/epd/RPI_sysfs_gpio.c)
#add_compile_definitions(EPD_TEST)

add_compile_definitions(GAPI)
#add_compile_definitions(GAPI_TEST)

add_executable(epaper main.c lib/bdf.c ${EPD_LIB} lib/calendar.c)

target_link_libraries(epaper -lbcm2835 -lm)

target_link_libraries(epaper -liddawc -lyder -lulfius -ljansson)
