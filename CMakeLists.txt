cmake_minimum_required(VERSION 2.8)
project(bitfun)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

SET(BF_INC_DIR "/bf/git/BitFunnel")
SET(BF_LIB_DIR "/bf/cmake/BitFunnel")

################################################################################

string(COMPARE EQUAL ${CMAKE_SYSTEM_NAME} "Linux" LINUX)

if (WIN32 OR CYGWIN)
  set(BITFUNNEL_PLATFORM_POSIX 0)
  set(BITFUNNEL_PLATFORM_WINDOWS 1)
else()
  set(BITFUNNEL_PLATFORM_POSIX 1)
  set(BITFUNNEL_PLATFORM_WINDOWS 0)
endif()


if(BITFUNNEL_PLATFORM_WINDOWS)
  # Note: -D works for both MSVC and GCC/Clang.
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -DBITFUNNEL_PLATFORM_WINDOWS")
else()
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -DBITFUNNEL_PLATFORM_POSIX")
endif()


if (WIN32 OR CYGWIN)
    set(NATIVEJIT_PLATFORM_POSIX 0)
    set(NATIVEJIT_PLATFORM_WINDOWS 1)
else()
    set(NATIVEJIT_PLATFORM_POSIX 1)
    set(NATIVEJIT_PLATFORM_WINDOWS 0)
endif()


if(NATIVEJIT_PLATFORM_WINDOWS)
  # Note: -D works for both MSVC and GCC/Clang.
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -DNATIVEJIT_PLATFORM_WINDOWS")
else()
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -DNATIVEJIT_PLATFORM_POSIX")
endif()


# TODO: consider fixing weak-vtables.
# TODO: fix sign-conversion. There's a lot in CmdLineParser.
# TODO: can we fix format-non-literal?
# TODO: we can remove -Wno-switch-enum when we get rid of StreamId as an enum.
# TODO: only disable shorten-64-to-32 where we check for overflow and then throw. This is done in RowId.
# TODO: see issue #147 about non-virtual destructor.
# Wno-missing-prototypes is because we commonly declare functions without prototypes in tests.
# Wno-float-equal is because we have tests for serializing and deserialization of floats that compare exact values.
# Wno-exit-time-destructors is for Logging
# Wno-switch-enum is because we often have a "default" case that throws for all invalid types.
set(GTEST_REQUIRED_FLAGS "-Wno-shift-sign-overflow -Wno-missing-noreturn -Wno-used-but-marked-unused -Wno-deprecated -Wno-missing-variable-declarations -Wno-global-constructors -Wno-weak-vtables")
set(WEVERYTHING_TODO_FLAGS "-Wno-format-nonliteral -Wno-non-virtual-dtor")
set(WEVERYTHING_FLAGS "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-undef -Wno-missing-prototypes -Wno-float-equal -Wno-exit-time-destructors -Wno-switch-enum ${GTEST_REQUIRED_FLAGS} ${WEVERYTHING_TODO_FLAGS}")
# The standard way to use msan is to compile libc++ with msan.
# TODO: figure out how to make msan linker flags generic.
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -L/home/leah/dev/libcxx_msan/lib -lc++abi -I/home/leah/dev/libcxx_msan/include -I/home/leah/dev/libcxx_msan/include/c++/v1")
# set(MSAN_FLAGS "-fsanitize=memory")

if(MSVC)
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} /W4 /WX -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
  set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_CXX_FLAGS} /MTd")
  set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_CXX_FLAGS} /MT /Z7")
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /DEBUG" )
elseif(CMAKE_COMPILER_IS_GNUCXX)
  # Need gnu++ instead of c++ so that GTest can access fdopen() etc.
  set(CMAKE_CXX_FLAGS "-msse4.2 -std=gnu++14 -Wall -Wextra -Werror -fstrict-aliasing -Wstrict-aliasing")
else()
  # TODO: define a target for -Weverything.
  # set(CMAKE_CXX_FLAGS "-msse4.2 -std=c++14 -Wall -Wextra -Werror -Wold-style-cast ${WEVERYTHING_FLAGS} -Wno-missing-braces")
  # TODO: define a target for msan.
  # set(CMAKE_CXX_FLAGS "-msse4.2 -std=c++14 -Wall -Wextra -Werror -Wold-style-cast ${MSAN_FLAGS} -Wno-missing-braces")
  set(CMAKE_CXX_FLAGS "-msse4.2 -std=c++14 -Wall -Wextra -Werror -Wno-missing-braces")
endif()


if(LINUX)
    find_package(Threads REQUIRED)
    if(CMAKE_USE_PTHREADS_INIT)
        # TODO: pthreads are an issue in general on linux.
        # TODO: We need a permanent recommendation for when/how to use libc++.
        # On clang for at least some platforms, we require `libc++-dev`.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    endif()
endif()


# This allows the solution to group projects into folders like src and test.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

macro(COMBINE_FILE_LISTS)
    if (BITFUNNEL_PLATFORM_WINDOWS)
        set(CPPFILES ${CPPFILES} ${WINDOWS_CPPFILES})
        set(PUBLIC_HFILES ${PUBLIC_HFILES} ${WINDOWS_PUBLIC_HFILES})
        set(PRIVATE_HFILES ${PRIVATE_HFILES} ${WINDOWS_PRIVATE_HFILES})
    else (BITFUNNEL_PLATFORM_WINDOWS)
        set(CPPFILES ${CPPFILES} ${POSIX_CPPFILES})
        set(PUBLIC_HFILES ${PUBLIC_HFILES} ${POSIX_PUBLIC_HFILES})
        set(PRIVATE_HFILES ${PRIVATE_HFILES} ${POSIX_PRIVATE_HFILES})
    endif (BITFUNNEL_PLATFORM_WINDOWS)
endmacro()

###############################################################################


include_directories(
    "${CMAKE_SOURCE_DIR}/src"
    "${BF_INC_DIR}/inc"
    "${BF_INC_DIR}/NativeJIT/inc"
    "${BF_INC_DIR}/src/Plan/src"
    "${BF_INC_DIR}/src/Common/Configuration/src"
    "${BF_INC_DIR}/src/Common/Utilities/src"
)

add_executable(bitfun 
	src/main.cpp
	src/QueryRunner.cpp
	src/server.cpp
)

target_link_libraries(bitfun 
   "${BF_LIB_DIR}/src/Index/src/libIndex.a"
   "${BF_LIB_DIR}/src/Chunks/src/libChunks.a"
   "${BF_LIB_DIR}/src/Plan/src/libPlan.a"
   "${BF_LIB_DIR}/src/Common/Configuration/src/libConfiguration.a"
   "${BF_LIB_DIR}/src/Common/CsvTsv/src/libCsvTsv.a"
   "${BF_LIB_DIR}/src/Common/Utilities/src/libUtilities.a"
   "${BF_LIB_DIR}/src/Data/src/libData.a"
   "${BF_LIB_DIR}/NativeJIT/src/NativeJIT/libNativeJIT.a"
   "${BF_LIB_DIR}/NativeJIT/src/CodeGen/libCodeGen.a"
)

