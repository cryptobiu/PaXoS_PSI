
#project(fronend)


 

#############################################
#               Build fronend.exe           #
#############################################

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -ggdb -rdynamic")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g3 -ggdb -rdynamic")

file(GLOB_RECURSE SRC_FRONTEND ${CMAKE_SOURCE_DIR}/frontend/*.cpp)
include_directories(${CMAKE_SOURCE_DIR}/frontend/ $ENV{HOME} xxHash ${CMAKE_SOURCE_DIR}/libOTe
        ${CMAKE_SOURCE_DIR}/libOTe/cryptoTools/cryptoTools ${CMAKE_SOURCE_DIR}/libOTe/cryptoTools/cryptoTools/Crypto)

add_executable(frontend ${SRC_FRONTEND})


#############################################
#            Link our libraries             #
#############################################

link_directories(/usr/ssl/lib/ $ENV{HOME}/libscapi/install/lib)

#############################################
#                 Link libOTe               #
#############################################


find_library(
    libOTe_Tests_LIB
    NAMES libOTe_Tests
    HINTS "${libOTe_Dirs}/lib/")

find_library(
    cryptoTools_Tests_LIB
    NAMES tests_cryptoTools
    HINTS "${libOTe_Dirs}/lib/")

target_link_libraries(frontend libPSI ${Boost_LIBRARIES}
        ${CMAKE_SOURCE_DIR}/frontend/xxHash/cmake_unofficial/libxxhash.so givaro blas gomp linbox ${CMAKE_SOURCE_DIR}/thirdparty/linux/ntl/src/libntl.a gmp
        $ENV{HOME}/libscapi/libscapi.a crypto)
