cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(NAME colourcyclinginthehousetonight)
project(${NAME} LANGUAGES C)

option(ENABLE_ASAN "Enable address sanitiser" OFF)

set(CMAKE_C_STANDARD 99)

find_package(SDL2 REQUIRED)
include(CheckSymbolExists)
include(CheckIncludeFile)

if (NOT MSVC)
	check_symbol_exists(ftello "stdio.h" HAVE_FTELLO)
	check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
	check_include_file("endian.h" HAVE_ENDIAN_H)
	check_include_file("arm/endian.h" HAVE_ARM_ENDIAN_H)
endif()

set(SOURCES
	src/font.c src/font.h
	src/text.c src/text.h
	src/hsluv.c src/hsluv.h
	src/stb_vorbis.h
	src/util.h
	src/lbmio.c src/lbmpal.c src/lbmdef.h
	src/lbm.c src/lbm.h
	src/audio.c src/audio.h
	src/surface.c src/surface.h
	src/display.c src/display.h
	src/main.c)

add_executable(${NAME} ${SOURCES})
target_compile_definitions(${NAME} PRIVATE
	$<$<BOOL:${HAVE_FTELLO}>:HAVE_FTELLO>
	$<$<BOOL:${HAVE_FSEEKO}>:HAVE_FSEEKO>
	$<$<BOOL:${HAVE_ENDIAN_H}>:HAVE_ENDIAN_H>
	$<$<BOOL:${HAVE_ARM_ENDIAN_H}>:HAVE_ARM_ENDIAN_H>
	$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
if (APPLE)
	target_link_options(${NAME} PRIVATE "-Wl,-rpath,/Library/Frameworks")
endif()
target_link_libraries(${NAME}
	$<$<PLATFORM_ID:Windows>:SDL2::SDL2main>
	SDL2::SDL2
	$<$<C_COMPILER_ID:Clang,GNU>:m>)
target_compile_options(${NAME} PRIVATE
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -pedantic -Wno-unused-parameter>
	$<$<C_COMPILER_ID:MSVC>:/W4 /wd4100>)
if (ENABLE_ASAN)
	target_compile_options(${NAME} PRIVATE
		$<$<OR:$<C_COMPILER_ID:AppleClang,Clang>,$<CONFIG:Debug>>:-fsanitize=address -fno-omit-frame-pointer>)
	target_link_options(${NAME} PRIVATE
		$<$<OR:$<C_COMPILER_ID:AppleClang,Clang>,$<CONFIG:Debug>>:-fsanitize=address -shared-libasan>)
endif()
