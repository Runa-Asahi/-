# CMake generated Testfile for 
# Source directory: C:/Users/25686/Desktop/OOP
# Build directory: C:/Users/25686/Desktop/OOP/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("smoke" "C:/Users/25686/Desktop/OOP/build/fanlx_app.exe" "--demo")
set_tests_properties("smoke" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;36;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
add_test("cache_behavior" "C:/Users/25686/Desktop/OOP/build/cache_tests.exe")
set_tests_properties("cache_behavior" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;42;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
add_test("group_behavior" "C:/Users/25686/Desktop/OOP/build/group_tests.exe")
set_tests_properties("group_behavior" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;45;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
add_test("persistence_behavior" "C:/Users/25686/Desktop/OOP/build/persistence_tests.exe")
set_tests_properties("persistence_behavior" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;52;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
