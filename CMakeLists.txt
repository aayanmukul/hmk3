cmake_minimum_required(VERSION 3.18)
project(HW3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(assimp REQUIRED)

# GLM is header-only; find it via pkg-config or just include system path
find_path(GLM_INCLUDE_DIR glm/glm.hpp HINTS /usr/include)

file(GLOB SOURCES "src/*.cpp")

add_executable(HW3 ${SOURCES})

target_include_directories(HW3 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
    ${GLM_INCLUDE_DIR}
)

target_link_libraries(HW3 PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    assimp
)

if(UNIX AND NOT APPLE)
    target_link_libraries(HW3 PRIVATE dl pthread)
endif()

# Copy assets to build/bin/assets
set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/assets")
set(DEST_DIR   "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "${SOURCE_DIR}/*")

add_custom_target(copy_assets ALL COMMENT "Copying assets...")
foreach(SRC_FILE ${ASSET_FILES})
    file(RELATIVE_PATH REL_PATH "${SOURCE_DIR}" "${SRC_FILE}")
    set(DST_FILE "${DEST_DIR}/${REL_PATH}")
    get_filename_component(DST_DIR "${DST_FILE}" DIRECTORY)
    add_custom_command(TARGET copy_assets PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${DST_FILE}"
    )
endforeach()

# Copy shaders to build/bin/shaders
set(SHADER_SRC "${PROJECT_SOURCE_DIR}/shaders")
set(SHADER_DST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")

file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${SHADER_SRC}/*")

add_custom_target(copy_shaders ALL COMMENT "Copying shaders...")
foreach(SRC_FILE ${SHADER_FILES})
    get_filename_component(FNAME "${SRC_FILE}" NAME)
    add_custom_command(TARGET copy_shaders PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_DST}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${SHADER_DST}/${FNAME}"
    )
endforeach()

add_dependencies(HW3 copy_assets copy_shaders)
