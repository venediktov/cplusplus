cmake_minimum_required(VERSION 2.8)

find_package(
	Boost
	COMPONENTS program_options
	)

ADD_DEFINITIONS(
	-std=c++11 -g #-fno-elide-constructors # Or -std=c++0x
	# Other flags
	)

include_directories(${Boost_INCLUDE_DIRS} ../handlers)

add_executable(
	test_move 
	test_move.cpp
	)

add_executable(
	process_simple 
	process_simple.cpp
	)

add_executable(
	process_exec 
	process_exec.cpp
	)

add_executable(
	pool_test 
	pool.cpp
	)

add_executable(
	matrix_algo_test 
	matrix_algo.cpp
	)

add_executable(
	matrix_algo_disjoint_test 
	matrix_algo_disjoint.cpp
	)

target_link_libraries(
	process_simple
	${Boost_LIBRARIES}
	)

target_link_libraries(
	process_exec
	${Boost_LIBRARIES}
	)

target_link_libraries(
	pool_test
        pthread
	)


add_subdirectory(IBJts)

