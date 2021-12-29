cmake_minimum_required(VERSION 3.16)

# Set the project name
project (usb-mon)

# Set the C++ standard C++11
set(CMAKE_CXX_STANDARD 11)

# Lib udev
find_library(LINUX_UDEV_LIB udev)
message("UDEV? ${UDEV_LIB}")

# Lib fmt
add_subdirectory(3rdparty/fmt)
find_package(fmt REQUIRED)

# Add an executable
add_executable(usb-mon main.cpp)

# Set the C++ standard to the appropriate standard for using auto
target_compile_features(usb-mon PUBLIC cxx_auto_type)

# Link Libraries
target_link_libraries(usb-mon udev fmt::fmt)

# Print Compile Features
message("List of compile features: ${CMAKE_CXX_COMPILE_FEATURES}")
