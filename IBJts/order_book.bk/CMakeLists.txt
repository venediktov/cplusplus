cmake_minimum_required(VERSION 2.8)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/../lib)
  set(CMAKE_CXX_COMPILER /usr/bin/g++)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()


find_package(
	Boost
	COMPONENTS 
        program_options 
        serialization 
        system 
        log 
        thread
	)

ADD_DEFINITIONS(
        -DIB_USE_STD_STRING
        -DBOOST_LOG_DYN_LINK
	-std=c++11 -g #-fno-elide-constructors # Or -std=c++0x
	# Other flags
	)

#TODO: think of a better way to find this package
include_directories(${Boost_INCLUDE_DIRS} ../source/PosixClient/Shared)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    include_directories(${Boost_INCLUDE_DIRS} ../source/PosixClient/Shared ../source/PosixClient/src)
    set(TARGET_LINK_IBLIB_LFLAG -L../source/PosixClient/lib -Wl,-rpath,${CMAKE_CURRENT_SOURCE_DIR}/../source/PosixClient/lib -liblib)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    include_directories(${Boost_INCLUDE_DIRS} ..\..\source\PosixClient\Shared ..\..\source\PosixClient\src)
endif()


add_executable(
	order_book
        logger.cpp
	ordermgr.cpp
	)

target_link_libraries(
	order_book
        ${TARGET_LINK_IBLIB_LFLAG}
	${Boost_LIBRARIES}
        pthread
        rt
	)

