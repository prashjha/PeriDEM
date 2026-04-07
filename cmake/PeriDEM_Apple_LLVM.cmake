# -------------------------------------------
# PeriDEM — macOS + Homebrew LLVM helpers
# -------------------------------------------
# AppleClang (Xcode) builds skip most of this. Homebrew /opt/homebrew/opt/llvm/bin/clang++
# is CMAKE_CXX_COMPILER_ID "Clang" and needs consistent ar/ranlib, SDK, and libc++ linking.
#
# Usage from top-level CMakeLists.txt:
#   include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PeriDEM_Apple_LLVM.cmake")
#   peridem_apple_llvm_before_project()
#   project(...)
#   peridem_apple_llvm_after_project()

macro(peridem_apple_llvm_before_project)
  # CMake may use /usr/bin/ar with llvm-ranlib for upstream Clang, corrupting .a archives.
  # Set AR/RANLIB before project() so generated static-library rules use llvm-ar consistently.
  if(APPLE)
    set(_pdem_cxx_for_ar "$ENV{CXX}")
    if(NOT _pdem_cxx_for_ar AND DEFINED CMAKE_CXX_COMPILER)
      set(_pdem_cxx_for_ar "${CMAKE_CXX_COMPILER}")
    endif()
    if(_pdem_cxx_for_ar)
      get_filename_component(_PDEM_LLVM_BIN "${_pdem_cxx_for_ar}" DIRECTORY)
      if(EXISTS "${_PDEM_LLVM_BIN}/llvm-ar" AND EXISTS "${_PDEM_LLVM_BIN}/llvm-ranlib")
        set(CMAKE_AR "${_PDEM_LLVM_BIN}/llvm-ar" CACHE FILEPATH "Archiver for static libraries" FORCE)
        set(CMAKE_RANLIB "${_PDEM_LLVM_BIN}/llvm-ranlib" CACHE FILEPATH "Ranlib for static libraries" FORCE)
      endif()
      unset(_PDEM_LLVM_BIN)
    endif()
    unset(_pdem_cxx_for_ar)
  endif()

  # clang-scan-deps (Ninja) may not get -isysroot; libc++ then misses system C headers (e.g. time.h).
  if(APPLE AND NOT CMAKE_OSX_SYSROOT)
    execute_process(
      COMMAND xcrun --sdk macosx --show-sdk-path
      OUTPUT_VARIABLE _pdem_osx_sysroot
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(_pdem_osx_sysroot)
      set(CMAKE_OSX_SYSROOT "${_pdem_osx_sysroot}" CACHE PATH "macOS SDK root" FORCE)
    endif()
    unset(_pdem_osx_sysroot)
  endif()
endmacro()

macro(peridem_apple_llvm_after_project)
  # Link Homebrew libc++ when using upstream Clang + explicit SDK (avoids missing std::__1::__hash_memory).
  if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    get_filename_component(_PDEM_LLVM_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_PDEM_LLVM_ROOT "${_PDEM_LLVM_BIN}" DIRECTORY)
    set(_PDEM_LLVM_LIBCXX "${_PDEM_LLVM_ROOT}/lib/c++")
    if(EXISTS "${_PDEM_LLVM_LIBCXX}/libc++.dylib")
      set(_PDEM_LLVM_LIBCXX_LINK "-L${_PDEM_LLVM_LIBCXX} -Wl,-rpath,${_PDEM_LLVM_LIBCXX}")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_PDEM_LLVM_LIBCXX_LINK}")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${_PDEM_LLVM_LIBCXX_LINK}")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${_PDEM_LLVM_LIBCXX_LINK}")
    endif()
    unset(_PDEM_LLVM_LIBCXX_LINK)
    unset(_PDEM_LLVM_LIBCXX)
    unset(_PDEM_LLVM_ROOT)
    unset(_PDEM_LLVM_BIN)
  endif()
endmacro()
