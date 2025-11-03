cmake_minimum_required(VERSION 3.20)
project(ARM64_Hypervisor C ASM)

# Configuração para ARM64 nativo no Windows 11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Verificar se estamos em um sistema ARM64
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    message(WARNING "Este projeto é otimizado para ARM64. Sistema atual: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Configurações específicas para Windows ARM64
if(WIN32)
    # Força arquitetura ARM64
    set(CMAKE_GENERATOR_PLATFORM ARM64)
    set(CMAKE_SYSTEM_PROCESSOR ARM64)
    
    # Windows 11 APIs
    add_definitions(-D_WIN32_WINNT=0x0A00)  # Windows 10/11
    add_definitions(-DWINVER=0x0A00)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
    
    # ARM64 específico
    add_definitions(-D_ARM64_)
    add_definitions(-DARM64_HYPERVISOR=1)
endif()

# Diretórios de include
include_directories(${CMAKE_SOURCE_DIR}/include)

# Habilitar assembly ARM64
enable_language(ASM)

# Arquivos fonte C
set(C_SOURCES
    src/main.c
    src/vm.c
    src/exit_handler.c
    src/exception_handlers.c
    src/devices/devices_main.c
    src/devices/uart.c
    src/devices/timer.c
    src/devices/gic.c
)

# Arquivos assembly ARM64
set(ASM_SOURCES
    src/asm/entry.s
)

# Guest code assembly
set(GUEST_SOURCES
    src/guest/hello.s
)

# Headers
set(HEADERS
    include/hypervisor.h
    include/vm.h
    include/devices.h
    include/asm_functions.h
)

# Executável principal do hypervisor
add_executable(hypervisor ${C_SOURCES} ${ASM_SOURCES} ${HEADERS})

# Guest binary (opcional, para teste)
if(ASM_SOURCES)
    add_executable(guest_hello ${GUEST_SOURCES})
    set_target_properties(guest_hello PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE /ENTRY:_start"
        OUTPUT_NAME "guest_hello.bin"
    )
endif()

# Configurações do compilador para ARM64
if(MSVC)
    # Visual Studio 2022 ARM64
    target_compile_options(hypervisor PRIVATE 
        /W4                     # Warning level 4
        /WX                     # Warnings as errors
        /arch:ARMv8.2           # ARM64 architecture
        /O2                     # Optimize for speed
        /Zi                     # Debug information
        /fp:fast               # Fast floating point
    )
    
    # Configurações de link específicas para ARM64
    target_link_options(hypervisor PRIVATE
        /MACHINE:ARM64
        /SUBSYSTEM:CONSOLE
        /DEBUG
        /OPT:REF
        /OPT:ICF
    )
else()
    # GCC/Clang para ARM64 (se disponível)
    target_compile_options(hypervisor PRIVATE 
        -Wall 
        -Wextra 
        -Werror
        -O2
        -march=armv8.2-a
        -mcpu=cortex-a72
        -ffast-math
        -funroll-loops
    )
endif()

# Bibliotecas Windows necessárias para ARM64
if(WIN32)
    target_link_libraries(hypervisor 
        winhvplatform          # Windows Hypervisor Platform
        kernel32               # Windows Kernel
        user32                 # Windows User
        advapi32               # Advanced APIs
        ws2_32                 # Winsock
        bcrypt                 # Cryptography
    )
endif()

# Propriedades específicas do target
set_target_properties(hypervisor PROPERTIES
    OUTPUT_NAME "hypervisor_arm64"
    DEBUG_POSTFIX "_debug"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Configurações específicas para Debug/Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hypervisor PRIVATE DEBUG=1)
    target_compile_definitions(hypervisor PRIVATE _DEBUG=1)
else()
    target_compile_definitions(hypervisor PRIVATE NDEBUG=1)
endif()

# Verificações de compilação específicas para ARM64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    # Testes específicos para ARM64
    include(CheckCSourceCompiles)
    
    check_c_source_compiles("
        #include <windows.h>
        #include <winhvplatform.h>
        int main() {
            WHV_CAPABILITY cap;
            WHvGetCapability(WHvCapabilityCodeHypervisorPresent, &cap, sizeof(cap), NULL);
            return 0;
        }
    " HAVE_WHP_SUPPORT)
    
    if(HAVE_WHP_SUPPORT)
        message(STATUS "✅ Windows Hypervisor Platform support detected")
        target_compile_definitions(hypervisor PRIVATE HAVE_WHP_SUPPORT=1)
    else()
        message(WARNING "⚠️  WHP support not detected - hypervisor may not work")
    endif()
    
    # Verificar recursos ARM64 específicos
    check_c_source_compiles("
        int main() {
            unsigned long val;
            __asm__ volatile(\"mrs %0, midr_el1\" : \"=r\" (val));
            return 0;
        }
    " HAVE_ARM64_SYSREGS)
    
    if(HAVE_ARM64_SYSREGS)
        message(STATUS "✅ ARM64 system registers support detected")
        target_compile_definitions(hypervisor PRIVATE HAVE_ARM64_SYSREGS=1)
    endif()
endif()

# Custom target para verificar sistema
add_custom_target(verify_system
    COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/VERIFY_SYSTEM.ps1"
    COMMENT "Verificando configuração do sistema ARM64"
    VERBATIM
)

# Custom target para limpeza completa
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}"
    COMMENT "Limpeza completa do build"
)

# Informações de build
message(STATUS "=== ARM64 Hypervisor Build Configuration ===")
message(STATUS "Sistema: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Arquitetura: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compilador: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Diretório fonte: ${CMAKE_SOURCE_DIR}")
message(STATUS "Diretório build: ${CMAKE_BINARY_DIR}")

if(WIN32)
    message(STATUS "Plataforma: ${CMAKE_GENERATOR_PLATFORM}")
    message(STATUS "Generator: ${CMAKE_GENERATOR}")
endif()

message(STATUS "=============================================")

# Targets de instalação
install(TARGETS hypervisor 
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HEADERS} DESTINATION include)
install(FILES README.md DEPLOY_GUIDE.md DESTINATION docs)