#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MAVSDK::mavsdk" for configuration "Debug"
set_property(TARGET MAVSDK::mavsdk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MAVSDK::mavsdk PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/mavsdkd.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/mavsdkd.dll"
  )

list(APPEND _cmake_import_check_targets MAVSDK::mavsdk )
list(APPEND _cmake_import_check_files_for_MAVSDK::mavsdk "${_IMPORT_PREFIX}/lib/mavsdkd.lib" "${_IMPORT_PREFIX}/bin/mavsdkd.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
