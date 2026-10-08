# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  [[CMakeFiles\CV_Assignment_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\CV_Assignment_autogen.dir\ParseCache.txt]]
  "CV_Assignment_autogen"
  )
endif()
