cmake_minimum_required(VERSION 3.20)
project(FanLXInstantMessaging LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
  add_compile_options(/W4 /utf-8)
endif()
add_library(nlohmann_json INTERFACE)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
target_include_directories(nlohmann_json INTERFACE third_party)
# 持久化层实现（源文件按层归入 src/persistence，头文件在 include/persistence）
add_library(fanlx_persistence
  src/persistence/DemoSeedFanLX.cpp
  src/persistence/SnapshotCodecFanLX.cpp
  src/persistence/SchemaValidatorFanLX.cpp
  src/persistence/PreparedCommitFanLX.cpp
  src/persistence/DataDirectoryLockFanLX.cpp
  src/persistence/AtomicFileWriterFanLX.cpp
  src/persistence/JsonFileRepositoryFanLX.cpp
  src/persistence/TransactionCoordinatorFanLX.cpp
  src/persistence/PersistenceApplicationFanLX.cpp)
target_include_directories(fanlx_persistence PUBLIC include)
target_link_libraries(fanlx_persistence PUBLIC nlohmann_json::nlohmann_json)
if(MINGW)
  target_compile_options(fanlx_persistence PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_executable(fanlx_app src/main.cpp)
target_include_directories(fanlx_app PRIVATE include)
target_link_libraries(fanlx_app PRIVATE fanlx_persistence)
if(MINGW)
  target_link_options(fanlx_app PRIVATE -municode)
  target_compile_options(fanlx_app PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME smoke COMMAND fanlx_app --demo)
add_executable(cache_tests tests/cache_tests.cpp)
target_include_directories(cache_tests PRIVATE include)
if(MINGW)
  target_compile_options(cache_tests PRIVATE -UNDEBUG)
endif()
add_test(NAME cache_behavior COMMAND cache_tests)
add_executable(group_tests tests/group_tests.cpp)
target_include_directories(group_tests PRIVATE include)
add_test(NAME group_behavior COMMAND group_tests)
add_executable(persistence_tests tests/persistence_tests.cpp)
target_link_libraries(persistence_tests PRIVATE fanlx_persistence)
if(MINGW)
  target_compile_options(persistence_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_link_options(persistence_tests PRIVATE -municode)
endif()
add_test(NAME persistence_behavior COMMAND persistence_tests)
