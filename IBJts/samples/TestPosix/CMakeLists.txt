cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0054 NEW)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  set(CMAKE_CXX_COMPILER /usr/bin/g++)
  
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()


add_library(ib SHARED IMPORTED)
find_library(IB_LIBRARY_PATH ib HINTS "${CMAKE_CURRENT_LIST_DIR}/../../")
set_target_properties(ib PROPERTIES IMPORTED_LOCATION "${IB_LIBRARY_PATH}")

find_package(
	Boost
	COMPONENTS program_options
	)

ADD_DEFINITIONS(
        -DIB_USE_STD_STRING
	-std=c++11 -g #-fno-elide-constructors # Or -std=c++0x
	# Other flags
	)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    include_directories(${Boost_INCLUDE_DIRS} ../../source/PosixClient/Shared ../../source/PosixClient/src)
    set(TARGET_LINK_IBLIB_LFLAG -L../../source/PosixClient/lib -Wl,-rpath,../../source/PosixClient/lib -liblib)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    include_directories(${Boost_INCLUDE_DIRS} ..\\..\\source\\PosixClient\\Shared ..\\..\\source\\PosixClient\\src)
endif()

add_executable(
        PosixSocketClientTest
        Main.cpp  PosixTestClient.cpp
	)

#link_directories(../../source/PosixClient/lib)

target_link_libraries(
        PosixSocketClientTest
        ${TARGET_LINK_IBLIB_LFLAG}
	${Boost_LIBRARIES}
	)

