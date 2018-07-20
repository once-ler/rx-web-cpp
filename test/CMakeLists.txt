cmake_minimum_required(VERSION 3.2)

project(testing VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "-v")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(OpenSSL)
find_package(Threads)
find_package(Boost 1.62.0 COMPONENTS thread date_time program_options filesystem system regex REQUIRED)

set(RXCPP ${PROJECT_SOURCE_DIR}/../../RxCpp/Rx/v2/src)
set(RXWEB ${PROJECT_SOURCE_DIR}/../)
set(SIMPLE_WEB_SERVER ${PROJECT_SOURCE_DIR}/../../Simple-Web-Server)
set(SIMPLE_WEBSOCKET_SERVER ${PROJECT_SOURCE_DIR}/../../Simple-WebSocket-Server)
# asyncplusplus for testing: https://github.com/Amanieu/asyncplusplus.git
set(ASYNCPLUSPLUS ${PROJECT_SOURCE_DIR}/../../asyncplusplus/include)
# json for testing: https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
set(JSON ${PROJECT_SOURCE_DIR}/../../json/src)

MESSAGE( STATUS "RXWEB: " ${RXWEB} )

include_directories(
  /usr/local/include
  "${RXCPP}" 
  "${RXWEB}"
  "${SIMPLE_WEB_SERVER}" 
  "${SIMPLE_WEBSOCKET_SERVER}"
  "${ASYNCPLUSPLUS}"
  "${JSON}"
)

link_directories("${PROJECT_SOURCE_DIR}/../../asyncplusplus/build")

file(GLOB_RECURSE CPP_SOURCES "${PROJECT_SOURCE_DIR}/test.cpp")

add_executable(testing ${CPP_SOURCES})

target_link_libraries(
  testing 
  ${Boost_LIBRARIES} 
  Threads::Threads 
  OpenSSL::Crypto 
  OpenSSL::SSL
  async++
)
