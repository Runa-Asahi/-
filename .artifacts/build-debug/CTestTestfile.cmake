# CMake generated Testfile for 
# Source directory: C:/Users/25686/Desktop/OOP
# Build directory: C:/Users/25686/Desktop/OOP/.artifacts/build-debug
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("smoke" "C:/Users/25686/Desktop/OOP/.artifacts/build-debug/fanlx_app.exe" "--demo")
set_tests_properties("smoke" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;35;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
add_test("cache_behavior" "C:/Users/25686/Desktop/OOP/.artifacts/build-debug/cache_tests.exe")
set_tests_properties("cache_behavior" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;41;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
add_test("group_behavior" "C:/Users/25686/Desktop/OOP/.artifacts/build-debug/group_tests.exe")
set_tests_properties("group_behavior" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;44;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
add_test("persistence_behavior" "C:/Users/25686/Desktop/OOP/.artifacts/build-debug/persistence_tests.exe")
set_tests_properties("persistence_behavior" PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/25686/Desktop/OOP/CMakeLists.txt;51;add_test;C:/Users/25686/Desktop/OOP/CMakeLists.txt;0;")
