cmake_minimum_required(VERSION 3.20)

# RTOS target selection (CACHE variable, set BEFORE project() for cross-compilation)
set(RTOS_TARGET "posix" CACHE STRING "Target RTOS: posix, freertos, rtthread, threadx, liteos, alios, zephyr, stm32f103")
set(SUPPORTED_RTOS posix freertos rtthread threadx liteos alios zephyr stm32f103)

# STM32 cross-compilation — MUST be before project() to correctly detect ARM compiler
if(RTOS_TARGET STREQUAL "stm32f103")
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_C_COMPILER /Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER /Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-g++)
    set(CMAKE_ASM_COMPILER /Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-gcc)
    set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -O2 -ffunction-sections -fdata-sections" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "-mcpu=cortex-m3 -mthumb -O2 -ffunction-sections -fdata-sections" CACHE STRING "" FORCE)
    set(CMAKE_ASM_FLAGS "-mcpu=cortex-m3 -mthumb" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103/STM32F103ZETX_FLASH.ld -Wl,--gc-sections,--print-memory-usage,--Map=${CMAKE_BINARY_DIR}/stm32.map --specs=nano.specs" CACHE STRING "" FORCE)
    # Bare metal: CMake should not try to link executables during compiler detection
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()

project(rtos_demo LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set_property(CACHE RTOS_TARGET PROPERTY STRINGS ${SUPPORTED_RTOS})

if(NOT RTOS_TARGET IN_LIST SUPPORTED_RTOS)
    message(FATAL_ERROR "Unsupported RTOS_TARGET: ${RTOS_TARGET}. "
            "Supported: ${SUPPORTED_RTOS}")
endif()

message(STATUS "Building for RTOS_TARGET=${RTOS_TARGET}")

# Platform include directory
set(PLATFORM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platform/include)

# Fetch LVGL v9
include(FetchContent)
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.3.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(lvgl)

# Pass STM32 define to LVGL library targets so lv_conf.h conditionals work
if(RTOS_TARGET STREQUAL "stm32f103")
    target_compile_definitions(lvgl PUBLIC RTOS_TARGET_stm32f103)
    if(TARGET lvgl_demos)
        target_compile_definitions(lvgl_demos PUBLIC RTOS_TARGET_stm32f103)
    endif()
    if(TARGET lvgl_examples)
        target_compile_definitions(lvgl_examples PUBLIC RTOS_TARGET_stm32f103)
    endif()
endif()

# Find SDL2 (only for desktop targets)
if(NOT RTOS_TARGET STREQUAL "stm32f103")
    find_package(SDL2 REQUIRED)
endif()

# Platform sources
set(PLATFORM_SRC
    platform/posix/rtos_scheduler.c
    platform/${RTOS_TARGET}/rtos_task.c
    platform/${RTOS_TARGET}/rtos_sync.c
    platform/${RTOS_TARGET}/rtos_time.c
    platform/${RTOS_TARGET}/rtos_mem.c
)

# LVGL port sources
set(LVGL_PORT_SRC
    lvgl_port/lvgl_port.c
)

# Demo app sources
set(DEMO_SRC
    apps/demo/main.c
    apps/demo/dashboard.c
)

# ---- rtos_demo (simple dashboard) ----
add_executable(rtos_demo
    ${PLATFORM_SRC}
    ${LVGL_PORT_SRC}
    ${DEMO_SRC}
)

# ---- complex_demo (LVGL widgets showcase) ----
set(COMPLEX_SRC
    apps/complex/main.c
)
add_executable(complex_demo
    ${PLATFORM_SRC}
    ${LVGL_PORT_SRC}
    ${COMPLEX_SRC}
)

# Shared include dirs setup macro
function(setup_demo_target TARGET)
    target_include_directories(${TARGET} PRIVATE
        ${PLATFORM_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/posix
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_port
        ${lvgl_SOURCE_DIR}
        ${lvgl_SOURCE_DIR}/src
        ${SDL2_INCLUDE_DIRS}
    )

    # Get SDL2 cflags if SDL2_INCLUDE_DIRS is empty (macOS brew)
    if(NOT SDL2_INCLUDE_DIRS)
        execute_process(COMMAND sdl2-config --cflags OUTPUT_VARIABLE SDL2_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
        separate_arguments(SDL2_ARGS UNIX_COMMAND ${SDL2_CFLAGS})
        foreach(arg ${SDL2_ARGS})
            if(arg MATCHES "^-I")
                string(SUBSTRING "${arg}" 2 -1 inc_dir)
                target_include_directories(${TARGET} PRIVATE ${inc_dir})
            endif()
        endforeach()
    endif()

    target_compile_definitions(${TARGET} PRIVATE
        RTOS_TARGET_${RTOS_TARGET}
        LV_CONF_INCLUDE_SIMPLE
        LV_LVGL_H_INCLUDE_SIMPLE
    )

    target_link_libraries(${TARGET} PRIVATE
        lvgl
        lvgl::examples
        SDL2::SDL2
    )
endfunction()

if(NOT RTOS_TARGET STREQUAL "stm32f103")
    setup_demo_target(rtos_demo)
    setup_demo_target(complex_demo)
endif()

# ---- STM32F103 target ----
if(RTOS_TARGET STREQUAL "stm32f103")
    set(STM32_SRC
        platform/stm32f103/bsp.c
        platform/stm32f103/lcd.c
        platform/stm32f103/touch.c
        platform/stm32f103/keypad.c
        platform/stm32f103/usart.c
        platform/stm32f103/lvgl_port.c
        platform/stm32f103/rtos_task.c
        platform/stm32f103/rtos_time.c
        platform/stm32f103/rtos_sync.c
        platform/stm32f103/rtos_mem.c
        platform/stm32f103/startup.s
        apps/stm32/main.c
    )

    add_executable(stm32_demo ${STM32_SRC})
    target_include_directories(stm32_demo PRIVATE
        ${PLATFORM_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103
        ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_port
        ${lvgl_SOURCE_DIR}
        ${lvgl_SOURCE_DIR}/src
    )
    target_compile_definitions(stm32_demo PRIVATE
        RTOS_TARGET_stm32f103
        LV_CONF_INCLUDE_SIMPLE
        LV_LVGL_H_INCLUDE_SIMPLE
    )
    target_link_libraries(stm32_demo PRIVATE lvgl)
endif()
