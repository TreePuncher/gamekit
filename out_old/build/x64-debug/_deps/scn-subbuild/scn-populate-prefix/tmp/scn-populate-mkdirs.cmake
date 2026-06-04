# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "E:/repos/gamekit/out/build/x64-debug/_deps/scn-src")
  file(MAKE_DIRECTORY "E:/repos/gamekit/out/build/x64-debug/_deps/scn-src")
endif()
file(MAKE_DIRECTORY
  "E:/repos/gamekit/out/build/x64-debug/_deps/scn-build"
  "E:/repos/gamekit/out/build/x64-debug/_deps/scn-subbuild/scn-populate-prefix"
  "E:/repos/gamekit/out/build/x64-debug/_deps/scn-subbuild/scn-populate-prefix/tmp"
  "E:/repos/gamekit/out/build/x64-debug/_deps/scn-subbuild/scn-populate-prefix/src/scn-populate-stamp"
  "E:/repos/gamekit/out/build/x64-debug/_deps/scn-subbuild/scn-populate-prefix/src"
  "E:/repos/gamekit/out/build/x64-debug/_deps/scn-subbuild/scn-populate-prefix/src/scn-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/repos/gamekit/out/build/x64-debug/_deps/scn-subbuild/scn-populate-prefix/src/scn-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/repos/gamekit/out/build/x64-debug/_deps/scn-subbuild/scn-populate-prefix/src/scn-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
