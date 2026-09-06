# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "src/CMakeFiles/diskoperations_test_autogen.dir/AutogenUsed.txt"
  "src/CMakeFiles/diskoperations_test_autogen.dir/ParseCache.txt"
  "src/CMakeFiles/isowizard_autogen.dir/AutogenUsed.txt"
  "src/CMakeFiles/isowizard_autogen.dir/ParseCache.txt"
  "src/CMakeFiles/ui_test_autogen.dir/AutogenUsed.txt"
  "src/CMakeFiles/ui_test_autogen.dir/ParseCache.txt"
  "src/diskoperations_test_autogen"
  "src/isowizard_autogen"
  "src/ui_test_autogen"
  )
endif()
