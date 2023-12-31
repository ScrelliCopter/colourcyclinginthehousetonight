find_package(Vorbis)

if (Vorbis_FOUND AND Vorbis_FRAMEWORK)
	set(Vorbisfile_FRAMEWORK TRUE)
	set(Vorbisfile_INCLUDE_DIR "${Vorbis_INCLUDE_DIRS}")
	set(Vorbisfile_LIBRARY "${Vorbis_LIBRARIES}")
	mark_as_advanced(Vorbisfile_FRAMEWORK)
else()
	find_package(PkgConfig QUIET)
	pkg_check_modules(_Vorbisfile_PC QUIET vorbisfile)

	if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin")
		set(_CMAKE_FIND_FRAMEWORK "${CMAKE_FIND_FRAMEWORK}")
		set(CMAKE_FIND_FRAMEWORK NEVER)
	endif()

	find_path(Vorbisfile_INCLUDE_DIR
		NAMES vorbis/vorbisfile.h
		PATHS
			${_Vorbisfile_PC_INCLUDEDIR}
			${_Vorbisfile_PC_INCLUDE_DIRS}
			${Vorbisfile_ROOT}
			${Vorbis_INCLUDE_DIRS})

	get_filename_component(Vorbisfile_LIBRARY_DIR "${Vorbis_LIBRARIES}" DIRECTORY CACHE)

	find_library(Vorbisfile_LIBRARY
		NAMES
			vorbisfile
			vorbisfile_static
			libvorbisfile
			libvorbisfile_static
		PATHS
			${_Vorbisfile_PC_LIBDIR}
			${_Vorbisfile_PC_LIBRARY_DIRS}
			${Vorbisfile_ROOT}
			${Vorbisfile_LIBRARY_DIR})

	mark_as_advanced(Vorbisfile_LIBRARY_DIR)

	if (CMAKE_C_PLATFORM_ID STREQUAL "Darwin")
		set(CMAKE_FIND_FRAMEWORK "${_CMAKE_FIND_FRAMEWORK}")
		unset(_CMAKE_FIND_FRAMEWORK)
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbisfile
	FOUND_VAR Vorbisfile_FOUND
	REQUIRED_VARS Vorbisfile_LIBRARY Vorbisfile_INCLUDE_DIR
	VERSION_VAR Vorbisfile_VERSION)

mark_as_advanced(Vorbisfile_FOUND Vorbisfile_INCLUDE_DIR Vorbisfile_LIBRARY Vorbisfile_VERSION)

if (Vorbisfile_FOUND AND NOT TARGET Vorbis::vorbisfile)
	set(Vorbisfile_INCLUDE_DIRS "${Vorbisfile_INCLUDE_DIR}")
	set(Vorbisfile_LIBRARIES "${Vorbisfile_LIBRARY}")
	if (Vorbisfile_FRAMEWORK)
		set(Vorbisfile_DEFINITIONS "")
		add_library(Vorbis::vorbisfile INTERFACE IMPORTED)
		set_target_properties(Vorbis::vorbisfile PROPERTIES
			INTERFACE_LINK_LIBRARIES Vorbis::vorbis
			INTERFACE_INCLUDE_DIRECTORIES "${Vorbisfile_INCLUDE_DIRS}")
	else()
		set(Vorbisfile_DEFINITIONS "${_Vorbis_PC_CFLAGS_OTHER}")
		add_library(Vorbis::vorbisfile UNKNOWN IMPORTED)
		set_target_properties(Vorbis::vorbisfile PROPERTIES
			IMPORTED_LOCATION "${Vorbisfile_LIBRARIES}"
			INTERFACE_LINK_LIBRARIES Vorbis::vorbis
			INTERFACE_COMPILE_OPTIONS "${Vorbisfile_DEFINITIONS}"
			INTERFACE_INCLUDE_DIRECTORIES "${Vorbisfile_INCLUDE_DIRS}")
	endif()
	mark_as_advanced(Vorbisfile_DEFINITIONS Vorbisfile_INCLUDE_DIRS Vorbisfile_LIBRARIES)
endif()
