cmake_minimum_required(VERSION 3.12)
project(libudis86 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

if(NOT EXISTS ${PROJECT_SOURCE_DIR}/itab.c OR NOT EXISTS ${PROJECT_SOURCE_DIR}/itab.h)
	find_package(Python3 COMPONENTS Interpreter)
	set(OPTABLE ${PROJECT_SOURCE_DIR}/../docs/x86/optable.xml)
	message("Building itab.c/itab.h...")
	execute_process(
		COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/../scripts/ud_itab.py ${OPTABLE} .
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
endif()

set(FILES
	decode.c
	decode.h
	extern.h
	itab.c
	itab.h
	syn-att.c
	syn-intel.c
	syn.c
	syn.h
	types.h
	udint.h
	udis86.c)

add_library(libudis86 STATIC ${FILES})

target_include_directories(libudis86 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})