cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/stm32_gcc.cmake)

project(stm32-oop C CXX ASM)
set(EXECUTABLE stm32-oop-f4)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(HAL_COMP_LIST I2C RCC GPIO CORTEX UART DMA)
set(CMSIS_COMP_LIST "")

list(APPEND CMSIS_COMP_LIST STM32F4)
list(APPEND HAL_COMP_LIST STM32F4)

find_package(CMSIS COMPONENTS "${CMSIS_COMP_LIST}" REQUIRED)
find_package(HAL COMPONENTS "${HAL_COMP_LIST}" REQUIRED)

add_subdirectory(etl)

file(GLOB SOURCES "Src/*.c" "Src/*.cpp")
add_executable(${EXECUTABLE} ${SOURCES})
target_include_directories(${EXECUTABLE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
target_link_libraries(${EXECUTABLE}
    HAL::STM32::F4::RCC
    HAL::STM32::F4::DMA
    HAL::STM32::F4::GPIO
    HAL::STM32::F4::UART
    HAL::STM32::F4::CORTEX
    CMSIS::STM32::F411CE
    STM32::NoSys
    etl::etl
)

stm32_generate_binary_file(${EXECUTABLE})
stm32_print_size_of_target(${EXECUTABLE})

add_custom_target(flash ALL
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program ${EXECUTABLE}.bin verify reset exit 0x08000000"
    DEPENDS ${EXECUTABLE}.bin
)
