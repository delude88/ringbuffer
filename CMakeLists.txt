cmake_minimum_required(VERSION 3.8)

project(Ringbuffer VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

find_package(Threads REQUIRED)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

add_executable(RingBufferTest EXCLUDE_FROM_ALL)
target_sources(RingBufferTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(RingBufferTest Ringbuffer Threads::Threads)
target_compile_options(RingBufferTest PUBLIC -O3)


include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion
)

export(
        TARGETS ${PROJECT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        FILE "${PROJECT_NAME}Config.cmake"
)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    install(
            DIRECTORY "include/"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
            TARGETS ${PROJECT_NAME}
            EXPORT "${PROJECT_NAME}Config"
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
            EXPORT "${PROJECT_NAME}Config"
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )
endif ()