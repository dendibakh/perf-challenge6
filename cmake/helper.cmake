# https://cmake.org/documentation/

# Check usage of 'build' subdirectory
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message("CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
  message("Please always use the 'build' subdirectory:")
  if(MSVC)
    message(FATAL_ERROR "git clean -dfx & cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -T ClangCL")
  else()
    message(FATAL_ERROR "git clean -dfx & mkdir build & cd build & cmake -DCMAKE_BUILD_TYPE=Release ..")
  endif()
endif()

# Just use the variable
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  message("Please consider: cmake -DCMAKE_BUILD_TYPE=Release ..")
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Set compiler options
if(NOT MSVC)
  set(CMAKE_C_FLAGS "-O3 -ffast-math -march=native ${CMAKE_C_FLAGS}")
else()
  include("${CMAKE_CURRENT_LIST_DIR}/msvc_simd_isa.cmake")
  if(SUPPORT_MSVC_AVX512)
    set(MSVC_SIMD_FLAGS "/arch:AVX512")
  elseif(SUPPORT_MSVC_AVX2)
    set(MSVC_SIMD_FLAGS "/arch:AVX2")
  elseif(SUPPORT_MSVC_AVX)
    set(MSVC_SIMD_FLAGS "/arch:AVX")
  else()
    set(MSVC_SIMD_FLAGS "")
  endif()
  set(CMAKE_C_FLAGS "/O2 /fp:fast ${MSVC_SIMD_FLAGS} ${CMAKE_C_FLAGS}")
endif()

# Set Windows stack size as on Linux: 2MB on 32-bit, 8MB on 64-bit
if (WIN32)
  math(EXPR stack_size "${CMAKE_SIZEOF_VOID_P}*${CMAKE_SIZEOF_VOID_P}*128*1024")
  if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:${stack_size}")
  else()
    # compiling with clang + lld
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker /stack:${stack_size}")
  endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")

# https://github.com/google/benchmark
find_package(benchmark REQUIRED)
set(BENCHMARK_LIBRARY "benchmark::benchmark")

# Find source files
file(GLOB srcs *.c *.h *.cpp *.hpp *.cxx *.hxx *.inl)
list(FILTER srcs EXCLUDE REGEX ".*bench.cpp$")
list(FILTER srcs EXCLUDE REGEX ".*validate.cpp$")

# Add main targets
add_executable(wordcount bench.cpp ${srcs} ${EXT_BENCHMARK_srcs})
add_executable(validate_wordcount validate.cpp ${srcs} ${EXT_VALIDATE_srcs})

# Check optional arguments
if(NOT DEFINED CI)
  set(CI OFF)
endif()
if(NOT DEFINED VALIDATE_ARGS)
  set(VALIDATE_ARGS "")
endif()
if(NOT DEFINED BENCHMARK_ARGS)
  set(BENCHMARK_ARGS "")
endif()

if("${BENCHMARK_MIN_TIME}" STREQUAL "")
  set(BENCHMARK_MIN_TIME "1")
endif()
set(BENCHMARK_BENCHMARK_ARGS --benchmark_min_time=${BENCHMARK_MIN_TIME} --benchmark_out_format=json --benchmark_out=result.json)

if(CI)
  # Add CI targets without dependencies
  get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(isMultiConfig)
    add_custom_target(validate
      COMMAND ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/validate_wordcount ${VALIDATE_ARGS}
      VERBATIM)

    add_custom_target(benchmark
      COMMAND ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/wordcount ${BENCHMARK_ARGS} ${BENCHMARK_BENCHMARK_ARGS}
      VERBATIM)
  else()
    add_custom_target(validate
      COMMAND ${PROJECT_BINARY_DIR}/validate_wordcount ${VALIDATE_ARGS}
      VERBATIM)

    add_custom_target(benchmark
      COMMAND ${PROJECT_BINARY_DIR}/wordcount ${BENCHMARK_ARGS} ${BENCHMARK_BENCHMARK_ARGS}
      VERBATIM)
  endif()
else()
  # Add robust execution targets
  add_custom_target(validate
    COMMAND validate_wordcount ${VALIDATE_ARGS}
    VERBATIM)

  add_custom_target(benchmark
    COMMAND wordcount ${BENCHMARK_ARGS} ${BENCHMARK_BENCHMARK_ARGS}
    VERBATIM)
endif()

# Other settings
if(NOT MSVC)
  if (WIN32)
    target_link_libraries(wordcount ${BENCHMARK_LIBRARY} shlwapi)
    target_link_libraries(validate_wordcount ${BENCHMARK_LIBRARY} shlwapi)
  else()
    target_link_libraries(wordcount ${BENCHMARK_LIBRARY} pthread m)
    target_link_libraries(validate_wordcount ${BENCHMARK_LIBRARY} pthread m)
  endif()

  # MinGW
  if(MINGW)
    target_link_libraries(wordcount shlwapi)
    target_link_libraries(validate_wordcount shlwapi)
  endif()
else()
  target_link_libraries(wordcount Shlwapi.lib ${BENCHMARK_LIBRARY})
  target_link_libraries(validate_wordcount Shlwapi.lib ${BENCHMARK_LIBRARY})

  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT wordcount)

  string(REPLACE ";" " " BENCHMARK_ARGS_STR "${BENCHMARK_ARGS}")
  string(REPLACE ";" " " VALIDATE_ARGS_STR "${VALIDATE_ARGS}")

  # Since CMake 3.13.0
  set_property(TARGET wordcount PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "${BENCHMARK_ARGS_STR}")
  set_property(TARGET validate_wordcount PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "${VALIDATE_ARGS_STR}")

  # Hide helper projects
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  set_target_properties(validate_wordcount PROPERTIES FOLDER CI)
  set_target_properties(benchmark PROPERTIES FOLDER CI)
endif()
