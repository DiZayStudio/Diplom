cmake_minimum_required(VERSION 3.20)

project(CppDiplomProject)

# TODO: Define by your own needs:
set(OPENSSL_ROOT_DIR "C:\\Program Files\\OpenSSL-Win64")

set(BOOST_ROOT "C:\\local\\boost_1_84_0")

set(libpqxx_DIR "C:\\Work\\libpqxx-7.7.5")

# TODO: If you are running on Windows below 10, change this:
add_compile_definitions(_WIN32_WINNT=0x0A01)

# Boost:
find_package(Boost 1.84.0 REQUIRED COMPONENTS system thread locale)

# PQXX
set(PostgreSQL_ADDITIONAL_VERSIONS "15")

add_subdirectory(${libpqxx_DIR} pqxx) 

# OpenSSL
find_package(OpenSSL REQUIRED)

add_subdirectory(spider)

add_subdirectory(http_server)