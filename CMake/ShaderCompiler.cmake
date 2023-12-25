# https://stackoverflow.com/a/60472877
# Utility function to compile shaders to SPIR-V bytecode

function(compile_shader target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "ENV;FORMAT" "SOURCES")
    foreach(source ${arg_SOURCES})
        add_custom_command(
            OUTPUT ${source}.${arg_FORMAT}
            DEPENDS ${source}
            DEPFILE ${source}.d
            COMMAND
                glslc_exe
                $<$<BOOL:${arg_ENV}>:--target-env=${arg_ENV}>
                $<$<BOOL:${arg_FORMAT}>:-mfmt=${arg_FORMAT}>
                -MD -MF ${source}.d
                -o ${source}.${arg_FORMAT}
                ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        )
        target_sources(${target} PRIVATE ${source}.${arg_FORMAT})
    endforeach()
endfunction()
