cmake_minimum_required(VERSION 3.0)
project(Test_Queue)

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
    ../src/queue.c
    test_queue.c
)

# Create the main executable target
add_executable(Test_Queue ${SOURCES})


