# CMake reference build of the C++ library and the nanobind python module
# (claude_notes/implementation_plan.md S6.4). autotools is the primary build
# here; this is the small example framework (which uses CMake) can copy. It
# builds libskeleton, the extension module skeleton_cxx/_bindings and the
# C++ unit tests:
#
#   cmake -S <srcdir> -B <builddir> -G Ninja -DCMAKE_PREFIX_PATH=$CONDA_PREFIX
#   cmake --build <builddir>
#   ctest --test-dir <builddir>       # C++ and python tests, from the build tree
cmake_minimum_required(VERSION 3.27)
project(cxx_python_rules_skeleton LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
configure_file(${CMAKE_SOURCE_DIR}/lib/skeleton_config.h.in
  ${CMAKE_BINARY_DIR}/config/skeleton_config.h @ONLY)
file(WRITE ${CMAKE_BINARY_DIR}/skeleton_build_config.h.cmake
  "#cmakedefine ADDR2LINE\n"
  "#ifdef ADDR2LINE\n#undef ADDR2LINE\n#define SKELETON_ADDR2LINE ${ADDR2LINE}\n#endif\n")
configure_file(${CMAKE_BINARY_DIR}/skeleton_build_config.h.cmake
  ${CMAKE_BINARY_DIR}/config/skeleton_build_config.h)

# The C++ library: lib/*.cc and cxx_python_rules/lib/*.cc, without the tests,
# the bindings and the golden archive generator.
file(GLOB LIB_SRC ${CMAKE_SOURCE_DIR}/lib/*.cc ${CMAKE_SOURCE_DIR}/cxx_python_rules/lib/*.cc)
list(FILTER LIB_SRC EXCLUDE REGEX "(_test|_nb|_nb_module|test_all|global_fixture|golden_archive_generator)\\.cc$")
add_library(skeleton SHARED ${LIB_SRC})
# The warnings configure uses (config/m4_local/skeleton_common.m4), for the
# project's own code (not nanobind's).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(SKELETON_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
    -Wsuggest-override -Wnon-virtual-dtor -Wconversion -Wsign-conversion
    -Wnull-dereference -Wcast-qual -Wformat=2 -Wdeprecated-copy-dtor)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND SKELETON_WARNINGS -Wduplicated-cond -Wduplicated-branches
    -Wlogical-op)
endif()
target_compile_options(skeleton PRIVATE ${SKELETON_WARNINGS})
target_include_directories(skeleton PUBLIC
  ${CMAKE_BINARY_DIR}/config ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/cxx_python_rules/lib
  ${MDSPAN_INCLUDE_DIR})
target_link_libraries(skeleton PUBLIC Boost::serialization Boost::iostreams
  PkgConfig::BLITZ Threads::Threads ${CMAKE_DL_LIBS})

# Docstrings from doxygen (an empty table without doxygen).
file(GLOB LIB_HEADERS ${CMAKE_SOURCE_DIR}/lib/*.h)
set(NB_DOC ${CMAKE_BINARY_DIR}/nb_docstrings.cc)
if(DOXYGEN)
  string(REPLACE ";" " " DOX_INPUT "${LIB_HEADERS}")
  add_custom_command(OUTPUT ${NB_DOC}
    COMMAND ${CMAKE_COMMAND} -E rm -rf nb_doc
    COMMAND ${CMAKE_COMMAND} -E env OUTDIR=nb_doc "INPUT=${DOX_INPUT}"
            ${DOXYGEN} ${CMAKE_SOURCE_DIR}/cxx_python_rules/nanobind/doxygen_nanobind.cfg > /dev/null
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cxx_python_rules/nanobind/doxy2nanobind.py nb_doc/xml ${NB_DOC}
    DEPENDS ${LIB_HEADERS} WORKING_DIRECTORY ${CMAKE_BINARY_DIR} VERBATIM)
else()
  add_custom_command(OUTPUT ${NB_DOC}
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cxx_python_rules/nanobind/doxy2nanobind.py no_doxygen ${NB_DOC}
    VERBATIM)
endif()

# The python package in the build tree: skeleton_cxx/ with the module and
# the python files from cxx_python_rules/.
set(PKG ${CMAKE_BINARY_DIR}/python/skeleton_cxx)
file(MAKE_DIRECTORY ${PKG})
foreach(f __init__.py all.py _nanobind_support.py py.typed)
  file(COPY ${CMAKE_SOURCE_DIR}/cxx_python_rules/${f} DESTINATION ${PKG})
endforeach()

file(GLOB NB_SRC ${CMAKE_SOURCE_DIR}/lib/*_nb.cc)
list(REMOVE_ITEM NB_SRC ${CMAKE_SOURCE_DIR}/lib/downstream_nb.cc)
nanobind_add_module(_bindings NB_STATIC ${NB_SRC}
  ${CMAKE_SOURCE_DIR}/lib/skeleton_nb_module.cc
  ${CMAKE_SOURCE_DIR}/cxx_python_rules/nanobind/cxx_python_rules_nb.cc ${NB_DOC})
nanobind_add_module(_skeleton_downstream NB_STATIC ${CMAKE_SOURCE_DIR}/lib/downstream_nb.cc)
foreach(m _bindings _skeleton_downstream)
  target_include_directories(${m} PRIVATE ${CMAKE_SOURCE_DIR}/cxx_python_rules/nanobind)
  target_compile_definitions(${m} PRIVATE CXX_PYTHON_RULES_NB_PACKAGE="skeleton_cxx")
  target_link_libraries(${m} PRIVATE skeleton)
  target_compile_options(${m} PRIVATE ${SKELETON_WARNINGS})
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
file(GLOB TEST_SRC ${CMAKE_SOURCE_DIR}/lib/*_test.cc)
set(unittestdata unit_test_data)
set(abs_top_srcdir ${CMAKE_SOURCE_DIR})
configure_file(${CMAKE_SOURCE_DIR}/lib/global_fixture_default.cc.in
  ${CMAKE_BINARY_DIR}/global_fixture_default.cc @ONLY)
add_executable(skeleton_test_all ${CMAKE_SOURCE_DIR}/lib/test_all.cc
  ${CMAKE_SOURCE_DIR}/lib/global_fixture.cc ${CMAKE_BINARY_DIR}/global_fixture_default.cc
  ${TEST_SRC})
target_link_libraries(skeleton_test_all PRIVATE skeleton)
target_compile_options(skeleton_test_all PRIVATE ${SKELETON_WARNINGS})
add_test(NAME cpp COMMAND skeleton_test_all)
set_tests_properties(cpp PROPERTIES ENVIRONMENT "abs_top_srcdir=${CMAKE_SOURCE_DIR}")
add_test(NAME python COMMAND ${Python_EXECUTABLE} -m pytest -q -p no:cacheprovider tests
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/python)
set_tests_properties(python PROPERTIES ENVIRONMENT
  "PYTHONPATH=${CMAKE_BINARY_DIR}/python:${CMAKE_SOURCE_DIR}/python")
