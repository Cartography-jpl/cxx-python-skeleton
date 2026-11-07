# Run by ctest (test "consumer"): install the skeleton build into a
# directory in the build tree, then configure, build and run the consumer
# project against it.
#   cmake -DSKELETON_BUILD=<build dir> -DSKELETON_SOURCE=<this dir>
#         -DPREFIX_PATH=<CMAKE_PREFIX_PATH> -P run.cmake
set(prefix ${SKELETON_BUILD}/consumer_test/install)
set(build ${SKELETON_BUILD}/consumer_test/build)
file(REMOVE_RECURSE ${SKELETON_BUILD}/consumer_test)
function(run)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "failed (${rc}): ${ARGN}")
  endif()
endfunction()
run(${CMAKE_COMMAND} --install ${SKELETON_BUILD} --prefix ${prefix})
run(${CMAKE_COMMAND} -S ${SKELETON_SOURCE} -B ${build}
  "-DCMAKE_PREFIX_PATH=${prefix};${PREFIX_PATH}"
  "-DCMAKE_BUILD_RPATH=${prefix}/lib;${prefix}/lib64")
run(${CMAKE_COMMAND} --build ${build})
run(${build}/consumer)
