cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(NAME colourcyclinginthehousetonight)
project(${NAME} LANGUAGES C)

option(ENABLE_ASAN "Enable address sanitiser" OFF)
option(USE_VORBISFILE "Opportunistically use Vorbisfile if available" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if (EMSCRIPTEN)
	set(CMAKE_VERBOSE_MAKEFILE 1)
endif()
find_package(SDL3 REQUIRED)
if (USE_VORBISFILE)
	find_package(Vorbisfile)
endif()

if (NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	include(CheckSymbolExists)
	check_symbol_exists(ftello "stdio.h" HAVE_FTELLO)
	check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
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
	$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin" AND SDL3_FRAMEWORK_PATH MATCHES "^/Library/Frameworks/")
	set_property(TARGET ${NAME} PROPERTY BUILD_RPATH "/Library/Frameworks")
endif()
if (USE_VORBISFILE AND TARGET Vorbis::vorbisfile)
	target_compile_definitions(${NAME} PRIVATE USE_VORBISFILE)
	target_link_libraries(${NAME} Vorbis::vorbisfile)
else()
	target_sources(${NAME} PRIVATE src/stb_vorbis.h)
endif()
target_link_libraries(${NAME} SDL3::SDL3 $<$<C_COMPILER_ID:Clang,GNU>:m>)
target_compile_options(${NAME} PRIVATE
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -pedantic -Wno-unused-parameter>
	$<$<C_COMPILER_ID:AppleClang,Clang>:-Weverything -Wno-declaration-after-statement -Wno-padded>
	$<$<C_COMPILER_ID:AppleClang>:-Wno-covered-switch-default>
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
