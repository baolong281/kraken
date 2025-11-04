cmake_minimum_required(VERSION 3.10)
project(MyFixApp VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(POCO_INCLUDE_DIR
  NAMES Poco/Poco.h
  HINTS /usr/local/include /usr/include
)

find_path(HTTPLIB_INCLUDE_DIR
  NAMES httplib.h
  HINTS /opt/homebrew/Cellar/cpp-httplib/0.26.0/
)

find_package(Boost REQUIRED)
find_library(POCO_FOUNDATION_LIBRARY
  NAMES PocoFoundation
  HINTS /usr/local/lib /usr/lib
)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

find_library(POCO_NET_LIBRARY       PocoNet       HINTS /usr/local/lib /usr/lib)
find_library(POCO_NETSSL_LIBRARY    PocoNetSSL    HINTS /usr/local/lib /usr/lib)


set(OPENSSL_ROOT_DIR /opt/homebrew/Cellar/openssl@3/3.5.2)
set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
set(OPENSSL_LIB_DIR ${OPENSSL_ROOT_DIR}/lib)

find_library(OPENSSL_CRYPTO_LIBRARY crypto HINTS ${OPENSSL_LIB_DIR})
find_library(OPENSSL_SSL_LIBRARY ssl HINTS ${OPENSSL_LIB_DIR})


add_executable(app
  src/main.cpp
  src/feed.cpp
  src/order_book.cpp
)

target_include_directories(app
    PRIVATE
        ${POCO_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        include       
        ${HTTPLIB_INCLUDE_DIR}
)

target_link_libraries(app PRIVATE
    ${POCO_FOUNDATION_LIBRARY}
    ${POCO_NET_LIBRARY}
    ${POCO_NETSSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    glaze::glaze
)

install(TARGETS app
  RUNTIME DESTINATION bin
)

configure_file(
    ${CMAKE_SOURCE_DIR}/get_kraken_token.py
    ${CMAKE_BINARY_DIR}/get_kraken_token.py
    COPYONLY
)
