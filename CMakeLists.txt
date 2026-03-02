cmake_minimum_required(VERSION 3.28)
project(HttpMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings (pero no tratamos como error los de PcapPlusPlus)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-error=pedantic -Wno-error=unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined,leak")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined,leak")

find_package(Threads REQUIRED)

# Buscar libpcap (necesaria para PcapPlusPlus)
find_library(PCAP_LIBRARY
        NAMES pcap libpcap
        PATHS /usr/lib /usr/local/lib /home/linuxbrew/.linuxbrew/lib
)

if(NOT PCAP_LIBRARY)
    message(FATAL_ERROR "libpcap no encontrada. Instala con: sudo apt-get install libpcap-dev")
endif()

message(STATUS "✓ libpcap encontrada: ${PCAP_LIBRARY}")

# PcapPlusPlus paths
set(PCAPPP_BASE "/home/linuxbrew/.linuxbrew/opt/pcapplusplus")
set(PCAPPP_INCLUDE_DIR ${PCAPPP_BASE}/include)
set(PCAPPP_LIB_DIR ${PCAPPP_BASE}/lib)

# Verificar que existen las librerías
set(PCAPPP_LIBS
        ${PCAPPP_LIB_DIR}/libPcap++.a
        ${PCAPPP_LIB_DIR}/libPacket++.a
        ${PCAPPP_LIB_DIR}/libCommon++.a
)

foreach(LIB ${PCAPPP_LIBS})
    if(NOT EXISTS ${LIB})
        message(FATAL_ERROR "Librería no encontrada: ${LIB}")
    endif()
    message(STATUS "✓ ${LIB}")
endforeach()

# Core library
add_library(core STATIC
        src/core/host.cpp
        src/core/http_request.cpp
        src/core/http_parser.cpp    # Nuevo
        src/core/statistics.cpp     # Nuevo
)
target_include_directories(core PUBLIC include)

# Capture library
add_library(capture STATIC
        src/capture/pcap_capture.cpp
)
target_include_directories(capture
        PUBLIC include
        PRIVATE ${PCAPPP_INCLUDE_DIR}
)

# IMPORTANTE: Linkear en orden correcto
target_link_libraries(capture PRIVATE
        ${PCAPPP_LIB_DIR}/libPcap++.a      # PcapPlusPlus
        ${PCAPPP_LIB_DIR}/libPacket++.a    # Packet++
        ${PCAPPP_LIB_DIR}/libCommon++.a    # Common++
        ${PCAP_LIBRARY}                    # libpcap (¡esto es lo que faltaba!)
        Threads::Threads
)

# IO library
add_library(io STATIC
        src/io/cli_parser.cpp
        src/io/console_logger.cpp
)
target_include_directories(io PUBLIC include)
target_link_libraries(io PUBLIC core)

# Executable
add_executable(http-monitor src/main.cpp)
target_include_directories(http-monitor PRIVATE ${PCAPPP_INCLUDE_DIR})
target_link_libraries(http-monitor PRIVATE core capture io)

set_target_properties(http-monitor PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)