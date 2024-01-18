add_executable(Triangle
    main.cpp TriangleLayer.cpp
    TriangleLayer.h utils.h
)
target_include_directories(Triangle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(Triangle PRIVATE shaderc)

# Set the source directory for shader files
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

# Set the destination directory for shader files in the build directory
set(SHADER_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# List all shader files
file(GLOB SHADER_FILES ${SHADER_SOURCE_DIR}/shader.*)

# Create a custom target for copying shader files
add_custom_target(CopyShaders
    COMMENT "Copying shader files to build directory"
    SOURCES ${SHADER_FILES}  # Mark shader files as sources for the target
)

# Create a custom command to copy shader files
add_custom_command(TARGET CopyShaders PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_FILES} ${SHADER_DEST_DIR}
)

# Link the executable or library target with the CopyShaders target
add_dependencies(Triangle CopyShaders)
