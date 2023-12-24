include(ShaderCompiler)

add_executable(Triangle
    main.cpp TriangleLayer.cpp
    TriangleLayer.h utils.h
)
target_include_directories(Triangle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

compile_shader(Triangle
    FORMAT bin
    SOURCES
        shaders/shader.vert
        shaders/shader.frag
)
