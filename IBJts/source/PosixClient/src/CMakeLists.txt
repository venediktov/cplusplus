cmake_minimum_required(VERSION 2.8)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/../lib)
  set(CMAKE_CXX_COMPILER /usr/bin/g++)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()


find_package(
	Boost
	COMPONENTS program_options
	)

ADD_DEFINITIONS(
        -DIB_USE_STD_STRING
	-std=c++11 -g #-fno-elide-constructors # Or -std=c++0x
	# Other flags
	)

include_directories(${Boost_INCLUDE_DIRS} ../Shared)

add_library(
	iblib
        SHARED
        EClientSocketBase.cpp  EPosixClientSocket.cpp 	
	)

target_link_libraries(
        iblib
	${Boost_LIBRARIES}
	)

