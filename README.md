
# CPU Usage Tracker

The project provides a application that monitors the CPU usage of a system. It consists of multiple threads responsible for reading CPU data, analyzing the data, and logging the results. The application uses a queue to store CPU usage data and handles graceful termination through signal handlers.

## Features

* Monitors and calculates CPU usage of the system.
* Utilizes multiple threads for data collection and analysis.
* Implements a circular queue to store CPU usage data.
* Gracefully terminates the application when a SIGTERM signal is received.
* Includes a watchdog thread to monitor the liveliness of reader and analyzer threads.
* Logs CPU usage data to a file at regular intervals.
* Prints the average CPU usage to the console.

## Dependancies

The project has the following dependencies:

* Linux operating system
* GCC (GNU Compiler Collection)
* Required pthread library

## Usage

* After building the project, execute the generated binary.
* The CPU Usage Tracker application will start monitoring the CPU usage.
* The program will log CPU usage data to a file and print the average CPU usage to the console at regular intervals.
* Send a SIGTERM signal to terminate the application gracefully.

## System Build

1. Install GCC (GNU Compiler Collection) if it is not already installed.
2. Clone the repository to your local machine.
3. Navigate to the project directory in your terminal.
4. Create build directory.

        mkdir build
5. Navigate to the created directory:

        cd build
6. Generate build files using CMake:

        cmake ..
7. Build the system by runnin the provided Makefile using:

        make
8. Run executable:

        ./CPU_Tracker
## Unit Testing

The CPU Usage Tracker project includes a unit test to verify the functionality of the queue.h and queue.c files. These tests ensure that the circular queue implementation behaves as expected and produces the desired results.

Execute the compiled unit test executable. The unit test will run, and the test result will be displayed in the terminal. If all tests pass successfully, you should see the message "All tests passed!".

![Test Queue Screenshot](https://i.imgur.com/pPvE04W.png)

## Memory Leak Testing

The CPU Usage Tracker project has been checked for memory leaks to make sure your application is managing memory properly.

### Running the Memory Leak Tests

1. Ensure that you have Valgrind installed on your system. If it is not installed, you can install it using your system's package manager or by downloading it from the Valgrind website.
2. Build the CPU Usage Tracker project by following the system build instructions provided in the README.
3. Open a terminal and navigate to the project's root directory.
4. Run the CPU Usage Tracker application using Valgrind's memcheck tool with the following command:

        valgrind --leak-check=full --show-leak-kinds=all ./CPU_Tracker

5. Valgrind will execute the CPU Usage Tracker application and provide a report on any detected memory leaks. If there are no memory leaks, you should see a message  "All heap blocks were freed -- no leaks are possible."

![Valgrind Screenshot](https://i.imgur.com/GJ3pSTU.png)

## References and Resources

[Proc documentation](https://www.kernel.org/doc/Documentation/filesystems/proc.txt)

[Queue implementation](https://www.youtube.com/watch?v=FcIubL92gaI)

[Catch SIGTERM, exit gracefully](https://airtower.wordpress.com/2010/06/16/catch-sigterm-exit-gracefully/)

[Valgrind manual](https://valgrind.org/docs/manual/quick-start.html)

[Unit testing with asserts](http://www.electronvector.com/blog/unit-testing-with-asserts)

[CMake build system manual](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html)
