find_package(Ogg)

if (NOT Vorbis_SKIP_PKGCONFIG)
	find_package(PkgConfig QUIET)
	pkg_check_modules(_Vorbis_PC QUIET vorbis)
endif()

if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin")
	set(_CMAKE_FIND_FRAMEWORK "${CMAKE_FIND_FRAMEWORK}")
	set(CMAKE_FIND_FRAMEWORK FIRST)
	set(_SYSTEM_FRAMEWORK_PATHS
		$ENV{HOME}/Library/Frameworks
		/Library/Frameworks)
endif()

find_library(Vorbis_LIBRARY
	NAMES
		Vorbis
		vorbis
		vorbis_static
		libvorbis
		libvorbis_static
	PATHS
		${_SYSTEM_FRAMEWORK_PATHS}
		${_Vorbis_PC_LIBDIR}
		${_Vorbis_PC_LIBRARY_DIRS}
		${Vorbis_ROOT})

if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin" AND Vorbis_LIBRARY MATCHES "[^/]+\\.framework$")
	set(Vorbis_FRAMEWORK TRUE)
	find_path(Vorbis_INCLUDE_DIR codec.h PATHS ${Vorbis_LIBRARY} PATH_SUFFIXES Headers)
else()
	find_path(Vorbis_INCLUDE_DIR
		NAMES vorbis.h
		PATHS
			${_Vorbis_PC_INCLUDEDIR}
			${_Vorbis_PC_INCLUDE_DIRS}
			${Vorbis_ROOT}
		PATH_SUFFIXES vorbis)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis
	FOUND_VAR Vorbis_FOUND
	REQUIRED_VARS Vorbis_LIBRARY Vorbis_INCLUDE_DIR
	VERSION_VAR Vorbis_VERSION)

if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin")
	unset(_SYSTEM_FRAMEWORK_PATHS)
	set(CMAKE_FIND_FRAMEWORK "${_CMAKE_FIND_FRAMEWORK}")
	unset(_CMAKE_FIND_FRAMEWORK)
endif()

mark_as_advanced(Vorbis_FOUND Vorbis_INCLUDE_DIR Vorbis_LIBRARY)

if (Vorbis_FOUND)
	set(Vorbis_INCLUDE_DIRS "${Vorbis_INCLUDE_DIR}")
	set(Vorbis_LIBRARIES "${Vorbis_LIBRARY}")
	if (Vorbis_FRAMEWORK)
		set(Vorbis_DEFINITIONS "")
	else()
		set(Vorbis_DEFINITIONS "${_Vorbis_PC_CFLAGS_OTHER}")
	endif()
	mark_as_advanced(Vorbis_INCLUDE_DIRS Vorbis_LIBRARIES Vorbis_DEFINITIONS)

	if (NOT TARGET Vorbis::vorbis)
		if (Vorbis_FRAMEWORK)
			add_library(Vorbis::vorbis INTERFACE IMPORTED)
			target_link_libraries(Vorbis::vorbis INTERFACE "${Vorbis_LIBRARIES}" Ogg::ogg)
			target_include_directories(Vorbis::vorbis INTERFACE "${Vorbis_INCLUDE_DIRS}")
		else()
			add_library(Vorbis::vorbis UNKNOWN IMPORTED)
			set_target_properties(Vorbis::vorbis PROPERTIES
				IMPORTED_LOCATION "${Vorbis_LIBRARIES}"
				INTERFACE_COMPILE_OPTIONS "${Vorbis_DEFINITIONS}"
				INTERFACE_LINK_LIBRARIES Ogg::ogg
				INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIRS}")
		endif()
	endif()
endif()
