# CMake generated Testfile for 
# Source directory: /home/alexander/projects/isowizard/src
# Build directory: /home/alexander/projects/isowizard/build/src
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(diskoperations "/home/alexander/projects/isowizard/build/src/diskoperations_test")
set_tests_properties(diskoperations PROPERTIES  _BACKTRACE_TRIPLES "/home/alexander/projects/isowizard/src/CMakeLists.txt;28;add_test;/home/alexander/projects/isowizard/src/CMakeLists.txt;0;")
add_test(ui "/home/alexander/projects/isowizard/build/src/ui_test")
set_tests_properties(ui PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software" _BACKTRACE_TRIPLES "/home/alexander/projects/isowizard/src/CMakeLists.txt;34;add_test;/home/alexander/projects/isowizard/src/CMakeLists.txt;0;")
