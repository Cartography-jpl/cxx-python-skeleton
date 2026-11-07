# CMake reference build of the C++ library and the nanobind python module
# (claude_notes/implementation_plan.md S6.4). autotools is the primary build
# here; this is the small example framework (which uses CMake) can copy. It
# builds libskeleton, the extension module skeleton_cxx/_bindings and the
# C++ unit tests:
#
#   cmake -S <srcdir> -B <builddir> -G Ninja -DCMAKE_PREFIX_PATH=$CONDA_PREFIX
#   cmake --build <builddir>
#   ctest --test-dir <builddir>       # C++ and python tests, from the build tree
#   cmake --install <builddir> --prefix <prefix>
#
# The install has the library, its headers (include/skeleton, as with
# autotools) and a CMake package, so another project can
# find_package(skeleton) and link skeleton::skeleton (cmake/consumer_test
# is an example, run by ctest). The python module isn't installed by CMake
# here; framework packages its python its own way. Paths are relative to
# this directory (CMAKE_CURRENT_SOURCE_DIR), so the project also builds
# when pulled into another one with add_subdirectory or FetchContent.
cmake_minimum_required(VERSION 3.27)
project(skeleton VERSION 0.10 LANGUAGES CXX)

# -std=c++23, not gnu++23, as configure does. The library asks for C++23
# with target_compile_features (below), which users of it inherit.
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CheckCXXCompilerFlag)

find_package(Python 3.12 COMPONENTS Interpreter Development.Module REQUIRED)
# nanobind's CMake package comes with the python package
execute_process(COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)
find_package(Boost 1.84 REQUIRED COMPONENTS serialization iostreams)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(BLITZ REQUIRED IMPORTED_TARGET blitz)
find_path(MDSPAN_INCLUDE_DIR mdspan/mdspan.hpp REQUIRED)
find_program(ADDR2LINE addr2line)
find_program(DOXYGEN doxygen)

# The configuration headers autoconf makes: skeleton_config.h (from the
# same template; installed headers include it) and skeleton_build_config.h
# (private, as far as the code uses it).
set(skeleton_no_serialization "")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lib/skeleton_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config/skeleton_config.h @ONLY)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/skeleton_build_config.h.cmake
  "#cmakedefine ADDR2LINE\n"
  "#ifdef ADDR2LINE\n#undef ADDR2LINE\n#define SKELETON_ADDR2LINE ${ADDR2LINE}\n#endif\n")
configure_file(${CMAKE_CURRENT_BINARY_DIR}/skeleton_build_config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config/skeleton_build_config.h)

# The C++ library: lib/*.cc and cxx_python_rules/lib/*.cc, without the tests,
# the bindings and the golden archive generator.
file(GLOB LIB_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/lib/*.cc)
list(FILTER LIB_SRC EXCLUDE REGEX "(_test|_nb|_nb_module|test_all|global_fixture|golden_archive_generator)\\.cc$")
add_library(skeleton SHARED ${LIB_SRC})
# The warnings configure uses (config/m4_local/skeleton_common.m4), for the
# project's own code (not nanobind's).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(SKELETON_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
    -Wsuggest-override -Wnon-virtual-dtor -Wconversion -Wsign-conversion
    -Wcast-qual -Wformat=2 -Wdeprecated-copy-dtor)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND SKELETON_WARNINGS -Wduplicated-cond -Wduplicated-branches
    -Wlogical-op)
endif()
target_compile_options(skeleton PRIVATE ${SKELETON_WARNINGS})
target_compile_features(skeleton PUBLIC cxx_std_23)
# As configure does: libstdc++'s assertions in a debug build, Boost's
# asserts off otherwise (defined empty, as skeleton_build_config.h does).
set(SKELETON_BUILD_DEFINITIONS $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>
  $<$<NOT:$<CONFIG:Debug>>:BOOST_DISABLE_ASSERTS=>)
target_compile_definitions(skeleton PRIVATE ${SKELETON_BUILD_DEFINITIONS})
# The installed headers: lib/ and cxx_python_rules/lib/ without the ones
# only the tests and bindings use, and the configuration header.
file(GLOB SKELETON_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h)
list(FILTER SKELETON_HEADERS EXCLUDE REGEX
  "/(unit_test_support|global_fixture|skeleton_nb_opaque|.*_nb)\\.h$")
file(GLOB RULES_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/lib/*.h)
target_sources(skeleton PUBLIC FILE_SET HEADERS
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/lib
    ${CMAKE_CURRENT_BINARY_DIR}/config
  FILES ${SKELETON_HEADERS} ${RULES_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/config/skeleton_config.h)
target_include_directories(skeleton PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/config
  PUBLIC $<BUILD_INTERFACE:${MDSPAN_INCLUDE_DIR}>)
target_link_libraries(skeleton PUBLIC Boost::serialization Boost::iostreams
  PkgConfig::BLITZ Threads::Threads ${CMAKE_DL_LIBS})

# Docstrings from doxygen (an empty table without doxygen).
file(GLOB LIB_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h)
set(NB_DOC ${CMAKE_CURRENT_BINARY_DIR}/nb_docstrings.cc)
if(DOXYGEN)
  string(REPLACE ";" " " DOX_INPUT "${LIB_HEADERS}")
  add_custom_command(OUTPUT ${NB_DOC}
    COMMAND ${CMAKE_COMMAND} -E rm -rf nb_doc
    COMMAND ${CMAKE_COMMAND} -E env OUTDIR=nb_doc "INPUT=${DOX_INPUT}"
            ${DOXYGEN} ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/nanobind/doxygen_nanobind.cfg > /dev/null
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/nanobind/doxy2nanobind.py nb_doc/xml ${NB_DOC}
    DEPENDS ${LIB_HEADERS} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} VERBATIM)
else()
  add_custom_command(OUTPUT ${NB_DOC}
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/nanobind/doxy2nanobind.py no_doxygen ${NB_DOC}
    VERBATIM)
endif()

# The python package in the build tree: skeleton_cxx/ with the module and
# the python files from cxx_python_rules/.
set(PKG ${CMAKE_CURRENT_BINARY_DIR}/python/skeleton_cxx)
file(MAKE_DIRECTORY ${PKG})
foreach(f __init__.py all.py _nanobind_support.py py.typed)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/${f} DESTINATION ${PKG})
endforeach()

file(GLOB NB_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*_nb.cc)
list(REMOVE_ITEM NB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib/downstream_nb.cc)
nanobind_add_module(_bindings NB_STATIC ${NB_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/skeleton_nb_module.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/nanobind/cxx_python_rules_nb.cc ${NB_DOC})
nanobind_add_module(_skeleton_downstream NB_STATIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/downstream_nb.cc)
foreach(m _bindings _skeleton_downstream)
  target_include_directories(${m} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cxx_python_rules/nanobind)
  target_compile_definitions(${m} PRIVATE CXX_PYTHON_RULES_NB_PACKAGE="skeleton_cxx")
  target_link_libraries(${m} PRIVATE skeleton)
  target_compile_options(${m} PRIVATE ${SKELETON_WARNINGS})
  target_compile_definitions(${m} PRIVATE ${SKELETON_BUILD_DEFINITIONS})
  # nanobind and python headers as system headers (as configure does), so
  # the warnings above don't apply to them: with GCC an -isystem directory
  # that is also given with -I is searched as a system directory.
  target_include_directories(${m} SYSTEM PRIVATE ${Python_INCLUDE_DIRS}
    ${nanobind_ROOT}/../include ${nanobind_ROOT}/../ext/robin_map/include)
  # Default visibility for the binding code, see cxx_python_rules/nanobind/nanobind.am
  set_target_properties(${m} PROPERTIES CXX_VISIBILITY_PRESET default
    LIBRARY_OUTPUT_DIRECTORY ${PKG} BUILD_RPATH $<TARGET_FILE_DIR:skeleton>)
endforeach()

# Tests
enable_testing()
file(GLOB TEST_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/*_test.cc)
set(unittestdata unit_test_data)
set(abs_top_srcdir ${CMAKE_CURRENT_SOURCE_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lib/global_fixture_default.cc.in
  ${CMAKE_CURRENT_BINARY_DIR}/global_fixture_default.cc @ONLY)
add_executable(skeleton_test_all ${CMAKE_CURRENT_SOURCE_DIR}/lib/test_all.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/global_fixture.cc ${CMAKE_CURRENT_BINARY_DIR}/global_fixture_default.cc
  ${TEST_SRC})
target_link_libraries(skeleton_test_all PRIVATE skeleton)
target_compile_options(skeleton_test_all PRIVATE ${SKELETON_WARNINGS})
target_compile_definitions(skeleton_test_all PRIVATE ${SKELETON_BUILD_DEFINITIONS})
# Boost.Test's macros use __COUNTER__, which clang >= 22 reports under
# -Wpedantic (as configure does, only where the warning exists).
check_cxx_compiler_flag(-Wc2y-extensions SKELETON_HAVE_WC2Y)
if(SKELETON_HAVE_WC2Y)
  target_compile_options(skeleton_test_all PRIVATE -Wno-c2y-extensions)
endif()
add_test(NAME cpp COMMAND skeleton_test_all)
set_tests_properties(cpp PROPERTIES ENVIRONMENT "abs_top_srcdir=${CMAKE_CURRENT_SOURCE_DIR}")
# pytest's temporary directory in the build directory (pytest.ini's
# ./pytest_tmp would be in the source tree, shared with other builds).
add_test(NAME python COMMAND ${Python_EXECUTABLE} -m pytest -q -p no:cacheprovider
  --basetemp=${CMAKE_CURRENT_BINARY_DIR}/pytest_tmp tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python)
set_tests_properties(python PROPERTIES ENVIRONMENT
  "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/python:${CMAKE_CURRENT_SOURCE_DIR}/python")

# Install: the library, its headers and the CMake package.
install(TARGETS skeleton EXPORT skeletonTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/skeleton)
install(EXPORT skeletonTargets NAMESPACE skeleton::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/skeleton)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/skeletonConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/skeletonConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/skeleton)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/skeletonConfigVersion.cmake
  COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/skeletonConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/skeletonConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/skeleton)

# A small project that uses the installed package (find_package(skeleton)):
# install into a directory in the build tree, then configure, build and run
# cmake/consumer_test against it.
add_test(NAME consumer COMMAND ${CMAKE_COMMAND}
  -DSKELETON_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  -DSKELETON_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/consumer_test
  -DPREFIX_PATH=${CMAKE_PREFIX_PATH}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/consumer_test/run.cmake)
