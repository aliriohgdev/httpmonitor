cmake_minimum_required(VERSION 3.28)
project(HttpMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined,leak")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined,leak")

option(ENABLE_THREAD_SANITIZER "Enable ThreadSanitizer" OFF)
if(ENABLE_THREAD_SANITIZER)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
endif()

find_package(PcapPlusPlus REQUIRED)
find_package(Threads REQUIRED)

# Core library
add_library(core STATIC
        src/core/host.cpp
        src/core/http_request.cpp
)
target_include_directories(core PUBLIC include)

# Capture library (commented out for now)
# add_library(capture STATIC
#     src/capture/pcap_capture.cpp
#     src/capture/packet_processor.cpp
# )
# target_include_directories(capture PUBLIC include)
# target_link_libraries(capture PRIVATE PcapPlusPlus::PcapPlusPlus Threads::Threads)

# IO library
add_library(io STATIC
        src/io/cli_parser.cpp
        src/io/console_logger.cpp
)
target_include_directories(io PUBLIC include)
target_link_libraries(io PUBLIC core)

# Executable
add_executable(http-monitor src/main.cpp)
target_link_libraries(http-monitor PRIVATE core io)

set_target_properties(http-monitor PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)