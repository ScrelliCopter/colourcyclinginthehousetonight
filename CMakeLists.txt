cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(NAME colourcyclinginthehousetonight)
project(${NAME} LANGUAGES C)

option(ENABLE_ASAN "Enable address sanitiser" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if (EMSCRIPTEN)
	set(CMAKE_VERBOSE_MAKEFILE 1)
	add_library(SDL2::SDL2 INTERFACE IMPORTED)
	set_target_properties(SDL2::SDL2 PROPERTIES
		INTERFACE_COMPILE_OPTIONS -sUSE_SDL=2
		INTERFACE_LINK_LIBRARIES "-sUSE_SDL=2")
else()
	find_package(SDL2 REQUIRED)
endif()
find_package(Vorbisfile)

if (NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	include(CheckSymbolExists)
	check_symbol_exists(ftello "stdio.h" HAVE_FTELLO)
	check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)

	include(CheckIncludeFile)
	check_include_file("endian.h" HAVE_ENDIAN_H)
	check_include_file("arm/endian.h" HAVE_ARM_ENDIAN_H)
endif()

add_executable(${NAME}
	src/font.c src/font.h
	src/text.c src/text.h
	src/hsluv.c src/hsluv.h
	src/util.h
	src/lbmio.c src/lbmpal.c src/lbmdef.h
	src/lbm.c src/lbm.h
	src/audio.c src/audio.h
	src/surface.c src/surface.h
	src/display.c src/display.h
	src/main.c)
set_property(TARGET ${NAME} PROPERTY C_STANDARD 99)
target_compile_definitions(${NAME} PRIVATE
	$<$<BOOL:${HAVE_FTELLO}>:HAVE_FTELLO>
	$<$<BOOL:${HAVE_FSEEKO}>:HAVE_FSEEKO>
	$<$<BOOL:${HAVE_ENDIAN_H}>:HAVE_ENDIAN_H>
	$<$<BOOL:${HAVE_ARM_ENDIAN_H}>:HAVE_ARM_ENDIAN_H>
	$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin" AND SDL2_FRAMEWORK_PATH MATCHES "^/Library/Frameworks/")
	target_link_options(${NAME} PRIVATE "-Wl,-rpath,/Library/Frameworks")
endif()
if (TARGET Vorbis::vorbisfile)
	target_compile_definitions(${NAME} PRIVATE USE_VORBISFILE)
	target_link_libraries(${NAME} Vorbis::vorbisfile)
else()
	target_sources(${NAME} PRIVATE src/stb_vorbis.h)
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
if (EMSCRIPTEN)
	target_link_options(${NAME} PRIVATE
		"--preload-file=${CMAKE_SOURCE_DIR}/web/files/lbm@lbm"
		"--preload-file=${CMAKE_SOURCE_DIR}/web/files/audio@audio")
endif()
