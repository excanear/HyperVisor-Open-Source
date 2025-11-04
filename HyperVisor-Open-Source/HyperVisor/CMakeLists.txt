# Desenvolvido por: Escanearcpl
cmake_minimum_required(VERSION 3.20)
project(HyperVisor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configurações específicas para Windows ARM64
if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR ARM64)
    add_definitions(-D_WIN32_WINNT=0x0A00)  # Windows 10+
    add_definitions(-DWINVER=0x0A00)
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.c
    src/vm.c
    src/exit_handler.c
    src/exception_handlers.c
    src/devices/devices_main.c
    src/devices/uart.c
    src/devices/timer.c
    src/devices/gic.c
)

# Headers
set(HEADERS
    include/hypervisor.h
    include/vm.h
    include/devices.h
)

# Create executable
add_executable(hypervisor ${SOURCES} ${HEADERS})

# Link Windows libraries
if(WIN32)
    target_link_libraries(hypervisor 
        winhvplatform
        kernel32
        user32
    )
endif()

# Compiler flags
if(MSVC)
    target_compile_options(hypervisor PRIVATE 
        /W4 
        /WX     # Warnings as errors
        /arch:ARMv8.0
    )
else()
    target_compile_options(hypervisor PRIVATE 
        -Wall 
        -Wextra 
        -Werror
        -march=armv8-a
    )
endif()

# Debug/Release configs
set_target_properties(hypervisor PROPERTIES
    DEBUG_POSTFIX "_d"
)

# Install rules
install(TARGETS hypervisor 
    RUNTIME DESTINATION bin
)
