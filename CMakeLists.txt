cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)
pico_sdk_init()
add_executable(PicoMiteWeb
    PicoMite.c
    regex.c 
    MMBasic.c
    Operators.c
    Custom.c
    Functions.c
    Commands.c
    Memory.c
    FileIO.c
    ff.c
    ffsystem.c
    ffunicode.c
    mmc_stm32.c
    Draw.c
    Editor.c
    XModem.c
    MM_Misc.c
    External.c
    MATHS.c
    Onewire.c
    I2C.c
    SPI.c
    Serial.c
    SPI-LCD.c
    BmpDecoder.c
    Touch.c
    GPS.c
    Audio.c
    CFunction.c
    Keyboard.c
    SSD1963.c
    picojpeg.c
    lfs.c
    lfs_util.c
	hxcmod.c
    cJSON.c
    mqtt.c
    MMMqtt.c
    MMTCPclient.c
    MMtelnet.c
    MMntp.c
    MMtcpserver.c
    tftp.c
    MMtftp.c
    MMudp.c
    VS1053.c
)
set_source_files_properties(mmc_stm32.c PROPERTIES COMPILE_FLAGS -O2)
set_source_files_properties(ff.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(GUI.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(BmpDecoder.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(GPS.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(I2C.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(lfs.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(picojpeg.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(regex.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(hxcmod.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(MATHS.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(Editor.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(MMtls.c PROPERTIES COMPILE_FLAGS -Os)
set_source_files_properties(cJSON.c PROPERTIES COMPILE_FLAGS -Os)
target_include_directories(PicoMiteWeb PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )

pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
pico_set_boot_stage2(PicoMiteWeb slower_boot2)
Pico_enable_stdio_usb(PicoMiteWeb 1)
pico_enable_stdio_uart(PicoMiteWeb 0)
pico_add_extra_outputs(PicoMiteWeb)
target_compile_options(PicoMiteWeb PRIVATE  -DPICOMITEWEB 
                                            -DNDEBUG
	                                        -DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=64 
											-DPICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0 
											-DPICO_STACK_SIZE=0x1000 
											-DPICO_CORE1_STACK_SIZE=0x0 
											-DPICO_HEAP_SIZE=0x2000 
											-DPICO_CYW43_ARCH_POLL
											-DCYW43_HOST_NAME="WebMite" 
                                            -Wall
											-O2)
target_link_libraries(PicoMiteWeb
    pico_stdlib
    hardware_flash
    hardware_irq
    hardware_adc
    hardware_pwm
    hardware_i2c
    hardware_spi
    hardware_dma
    hardware_exception 
	hardware_pio
    pico_cyw43_arch_lwip_poll
) 
