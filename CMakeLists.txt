cmake_minimum_required(VERSION 3.0)
project(CPU_Tracker)

set(CMAKE_C_STANDARD 99)

# Set the compiler with CC
if(DEFINED ENV{CC})
    set(CMAKE_C_COMPILER $ENV{CC})
endif()

# Include headers
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

# Add source files
set(SOURCES
    main.c
    src/queue.c
    src/handler.c
    src/reader.c
    src/analyzer.c
    src/printer.c
    src/watchdog.c
    src/logger.c
)

add_executable(CPU_Tracker ${SOURCES})

# Link pthread library
find_package(Threads REQUIRED)
target_link_libraries(CPU_Tracker Threads::Threads)
