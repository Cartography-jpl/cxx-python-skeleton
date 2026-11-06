# CMake reference build of the C++ library and the nanobind python module
# (claude_notes/implementation_plan.md S6.4). autotools is the primary build
# here; this is the small example framework (which uses CMake) can copy. It
# builds libskeleton, the extension module skeleton_swig/_swig_wrap and the
# C++ unit tests:
#
#   cmake -S <srcdir> -B <builddir> -G Ninja -DCMAKE_PREFIX_PATH=$CONDA_PREFIX
#   cmake --build <builddir>
#   ctest --test-dir <builddir>       # C++ and python tests, from the build tree
cmake_minimum_required(VERSION 3.27)
project(swig_rules_skeleton LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python 3.11 COMPONENTS Interpreter Development.Module REQUIRED)
# nanobind's CMake package comes with the python package
execute_process(COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)
find_package(Boost 1.84 REQUIRED COMPONENTS serialization iostreams)
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLITZ REQUIRED IMPORTED_TARGET blitz)
find_path(MDSPAN_INCLUDE_DIR mdspan/mdspan.hpp REQUIRED)
find_program(ADDR2LINE addr2line)
find_program(DOXYGEN doxygen)

# What autoconf's lib/skeleton_config.h has, as far as the code uses it.
set(SKELETON_HAVE_BOOST_SERIALIZATION 1)
file(WRITE ${CMAKE_BINARY_DIR}/skeleton_config.h.cmake
  "#cmakedefine SKELETON_HAVE_BOOST_SERIALIZATION 1\n"
  "#define HAVE_BOOST_SERIALIZATION_BOOST_ARRAY_HPP 1\n"
  "#cmakedefine ADDR2LINE\n"
  "#ifdef ADDR2LINE\n#undef ADDR2LINE\n#define SKELETON_ADDR2LINE ${ADDR2LINE}\n#endif\n")
configure_file(${CMAKE_BINARY_DIR}/skeleton_config.h.cmake
  ${CMAKE_BINARY_DIR}/config/skeleton_config.h)

# The C++ library: lib/*.cc and swig_rules/lib/*.cc, without the tests,
# the bindings and the golden archive generator.
file(GLOB LIB_SRC ${CMAKE_SOURCE_DIR}/lib/*.cc ${CMAKE_SOURCE_DIR}/swig_rules/lib/*.cc)
list(FILTER LIB_SRC EXCLUDE REGEX "(_test|_nb|_nb_module|test_all|global_fixture|golden_archive_generator)\\.cc$")
add_library(skeleton SHARED ${LIB_SRC})
target_include_directories(skeleton PUBLIC
  ${CMAKE_BINARY_DIR}/config ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/swig_rules/lib
  ${MDSPAN_INCLUDE_DIR})
target_compile_definitions(skeleton PUBLIC SWIG_MAPPER_NAMESPACE=Skeleton
  SWIG_HAVE_BOOST_SERIALIZATION)
target_link_libraries(skeleton PUBLIC Boost::serialization Boost::iostreams
  PkgConfig::BLITZ ${CMAKE_DL_LIBS})

# Docstrings from doxygen (an empty table without doxygen).
file(GLOB LIB_HEADERS ${CMAKE_SOURCE_DIR}/lib/*.h)
set(NB_DOC ${CMAKE_BINARY_DIR}/nb_docstrings.cc)
if(DOXYGEN)
  string(REPLACE ";" " " DOX_INPUT "${LIB_HEADERS}")
  add_custom_command(OUTPUT ${NB_DOC}
    COMMAND ${CMAKE_COMMAND} -E rm -rf nb_doc
    COMMAND ${CMAKE_COMMAND} -E env OUTDIR=nb_doc "INPUT=${DOX_INPUT}"
            ${DOXYGEN} ${CMAKE_SOURCE_DIR}/swig_rules/nanobind/doxygen_nanobind.cfg > /dev/null
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/swig_rules/nanobind/doxy2nanobind.py nb_doc/xml ${NB_DOC}
    DEPENDS ${LIB_HEADERS} WORKING_DIRECTORY ${CMAKE_BINARY_DIR} VERBATIM)
else()
  add_custom_command(OUTPUT ${NB_DOC}
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/swig_rules/nanobind/doxy2nanobind.py no_doxygen ${NB_DOC}
    VERBATIM)
endif()

# The python package in the build tree: skeleton_swig/ with the module and
# the python files from swig_rules/.
set(PKG ${CMAKE_BINARY_DIR}/python/skeleton_swig)
file(MAKE_DIRECTORY ${PKG})
foreach(f __init__.py all.py _nanobind_support.py py.typed)
  file(COPY ${CMAKE_SOURCE_DIR}/swig_rules/${f} DESTINATION ${PKG})
endforeach()

file(GLOB NB_SRC ${CMAKE_SOURCE_DIR}/lib/*_nb.cc)
list(REMOVE_ITEM NB_SRC ${CMAKE_SOURCE_DIR}/lib/downstream_nb.cc)
nanobind_add_module(_swig_wrap NB_STATIC ${NB_SRC}
  ${CMAKE_SOURCE_DIR}/lib/skeleton_nb_module.cc
  ${CMAKE_SOURCE_DIR}/swig_rules/nanobind/swig_rules_nb.cc ${NB_DOC})
nanobind_add_module(_skeleton_downstream NB_STATIC ${CMAKE_SOURCE_DIR}/lib/downstream_nb.cc)
foreach(m _swig_wrap _skeleton_downstream)
  target_include_directories(${m} PRIVATE ${CMAKE_SOURCE_DIR}/swig_rules/nanobind)
  target_compile_definitions(${m} PRIVATE SWIG_RULES_NB_PACKAGE="skeleton_swig")
  target_link_libraries(${m} PRIVATE skeleton)
  # Default visibility for the binding code, see swig_rules/nanobind/nanobind.am
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
add_test(NAME cpp COMMAND skeleton_test_all)
set_tests_properties(cpp PROPERTIES ENVIRONMENT "abs_top_srcdir=${CMAKE_SOURCE_DIR}")
add_test(NAME python COMMAND ${Python_EXECUTABLE} -m pytest -q -p no:cacheprovider tests
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/python)
set_tests_properties(python PROPERTIES ENVIRONMENT
  "PYTHONPATH=${CMAKE_BINARY_DIR}/python:${CMAKE_SOURCE_DIR}/python")
