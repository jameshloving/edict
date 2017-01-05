cmake_minimum_required (VERSION 2.6)
project (Edict)
 
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# cross compile for LEDE
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER /home/ubuntu/lede-sdk-brcm47xx-mips74k_gcc-5.4.0_musl-1.1.15.Linux-x86_64/staging_dir/toolchain-mipsel_74kc_gcc-5.4.0_musl-1.1.15/bin/mipsel-openwrt-linux-gcc)
SET(CMAKE_CXX_COMPILER /home/ubuntu/lede-sdk-brcm47xx-mips74k_gcc-5.4.0_musl-1.1.15.Linux-x86_64/staging_dir/toolchain-mipsel_74kc_gcc-5.4.0_musl-1.1.15/bin/mipsel-openwrt-linux-g++)
SET(CMAKE_FIND_ROOT_PATH /home/ubuntu/lede-sdk-brcm47xx-mips74k_gcc-5.4.0_musl-1.1.15.Linux-x86_64/staging_dir) 

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBRARIES OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-lstdc++ -static")

# add the executable
add_executable(query query.cpp)
