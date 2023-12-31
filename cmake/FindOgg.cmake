if (NOT Ogg_SKIP_PKGCONFIG)
	find_package(PkgConfig QUIET)
	pkg_check_modules(_Ogg_PC QUIET ogg)
endif()

if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin")
	set(_CMAKE_FIND_FRAMEWORK "${CMAKE_FIND_FRAMEWORK}")
	set(CMAKE_FIND_FRAMEWORK FIRST)
	set(_SYSTEM_FRAMEWORK_PATHS
		$ENV{HOME}/Library/Frameworks
		/Library/Frameworks)
endif()

find_path(Ogg_INCLUDE_DIR
	NAMES ogg.h
	PATHS
		${_Ogg_PC_INCLUDEDIR}
		${_Ogg_PC_INCLUDE_DIRS}
		${Ogg_ROOT}
	PATH_SUFFIXES ogg)

find_library(Ogg_LIBRARY
	NAMES
		Ogg
		ogg
		ogg_static
		libogg
		libogg_static
	PATHS
		${_SYSTEM_FRAMEWORK_PATHS}
		${_Ogg_PC_LIBDIR}
		${_Ogg_PC_LIBRARY_DIRS}
		${Ogg_ROOT})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg
	FOUND_VAR Ogg_FOUND
	REQUIRED_VARS Ogg_LIBRARY Ogg_INCLUDE_DIR
	VERSION_VAR Ogg_VERSION)

if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin")
	unset(_SYSTEM_FRAMEWORK_PATHS)
	set(CMAKE_FIND_FRAMEWORK "${_CMAKE_FIND_FRAMEWORK}")
	unset(_CMAKE_FIND_FRAMEWORK)
endif()

mark_as_advanced(Ogg_FOUND Ogg_INCLUDE_DIR Ogg_LIBRARY)

if (Ogg_FOUND)
	if (Ogg_LIBRARY MATCHES "[^/]+\\.framework$")
		set(Ogg_FRAMEWORK TRUE)
		set(Ogg_DEFINITIONS "")
	else()
		set(Ogg_DEFINITIONS "${_Ogg_PC_CFLAGS_OTHER}")
	endif()
	set(Ogg_INCLUDE_DIRS "${Ogg_INCLUDE_DIR}")
	set(Ogg_LIBRARIES "${Ogg_LIBRARY}")
	mark_as_advanced(Ogg_FRAMEWORK Ogg_DEFINITIONS Ogg_INCLUDE_DIRS Ogg_LIBRARIES)

	if (NOT TARGET Ogg::ogg)
		if (Ogg_FRAMEWORK)
			add_library(Ogg::ogg INTERFACE IMPORTED)
			set_target_properties(Ogg::ogg PROPERTIES
				INTERFACE_LINK_LIBRARIES "${Ogg_LIBRARIES}"
				INTERFACE_INCLUDE_DIRECTORIES "${Ogg_INCLUDE_DIRS}")
		else()
			add_library(Ogg::ogg UNKNOWN IMPORTED)
			set_target_properties(Ogg::ogg PROPERTIES
				IMPORTED_LOCATION "${Ogg_LIBRARIES}"
				INTERFACE_COMPILE_OPTIONS "${Ogg_DEFINITIONS}"
				INTERFACE_INCLUDE_DIRECTORIES "${Ogg_INCLUDE_DIRS}")
		endif()
	endif()
endif()
