SET(ETISS_PREFIX ${PROJECT_SOURCE_DIR})

PROJECT(etiss_softvector)

SET(TARGET_PLATFORM_NAME "riscv")
SET(INCLUDE_TESTS 0)
SET(TARGET_ARCH "RV64GCV")

include(FetchContent)

# print the content of SOFTVECTOR_LOCAL_PATH
message(STATUS "SOFTVECTOR_LOCAL_PATH: $ENV{SOFTVECTOR_LOCAL_PATH}")

# Check for local softvector path from environment variable
if(DEFINED ENV{SOFTVECTOR_LOCAL_PATH} AND EXISTS $ENV{SOFTVECTOR_LOCAL_PATH}/CMakeLists.txt)
    # Use local version
    message(STATUS "Using local softvector from: $ENV{SOFTVECTOR_LOCAL_PATH}")
    set(softvector_srcs_SOURCE_DIR $ENV{SOFTVECTOR_LOCAL_PATH})
    set(softvector_srcs_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/softvector_local)
    set(EXTERNAL_INSTALL_LOCATION $ENV{SOFTVECTOR_LOCAL_PATH})
    
    INSTALL(
        FILES ${softvector_srcs_SOURCE_DIR}/include/target/${TARGET_PLATFORM_NAME}/softvector.h
        DESTINATION include/jit/etiss/jit)
    SET(SOFTVECTOR_BUILD_DOC OFF)
else()
    # Use GitHub version (original behavior)
    message(STATUS "Fetching softvector from GitHub")
    FetchContent_Declare(softvector_srcs
        GIT_REPOSITORY https://github.com/danielschloms/softvector.git
        # GIT_TAG master
        GIT_TAG daniel-dev
    )

    if(NOT softvector_srcs_POPULATED)
        FetchContent_Populate(softvector_srcs)
        FetchContent_GetProperties(softvector_srcs)
        set(EXTERNAL_INSTALL_LOCATION ${softvector_srcs_SOURCE_DIR})
        INSTALL(
            FILES ${softvector_srcs_SOURCE_DIR}/include/target/${TARGET_PLATFORM_NAME}/softvector.h
            DESTINATION include/jit/etiss/jit)
        SET(SOFTVECTOR_BUILD_DOC OFF)
    endif()
endif()

ADD_SUBDIRECTORY(${softvector_srcs_SOURCE_DIR} ${softvector_srcs_BINARY_DIR})

##################### SOFTVECTOR JIT LIBRARY #####################

ADD_LIBRARY(${PROJECT_NAME} SHARED
  etiss_softvector.cpp
)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
  ${ETISS_PREFIX}/include_c
  ${softvector_srcs_SOURCE_DIR}/include/target/${TARGET_PLATFORM_NAME}
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} INTERFACE softvector)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD VERBATIM
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/jit/etiss/jit/
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" ${CMAKE_BINARY_DIR}/include/jit/etiss/jit/
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:softvector>" ${CMAKE_BINARY_DIR}/include/jit/etiss/jit/
  COMMAND ${CMAKE_COMMAND} -E copy ${softvector_srcs_SOURCE_DIR}/include/target/${TARGET_PLATFORM_NAME}/softvector.h ${CMAKE_BINARY_DIR}/include/jit/etiss/jit/
)

INSTALL(FILES
  libsoftvector.h
  DESTINATION include/jit/etiss/jit
)
InstallJitLib(${PROJECT_NAME})
InstallJitLib(softvector)
