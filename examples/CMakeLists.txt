link_libraries(Walnut)

# --------------------- Compiler & Linker Flags -----------------------
# enable VisualStudio Hot Reload feature
if (MSVC)
    add_compile_options($<$<CONFIG:Debug>:/ZI>)
    add_link_options($<$<CONFIG:Debug>:/INCREMENTAL>)
endif()

# don't show console window if on release mode
if (WIN32)
    if (MSVC)
        add_link_options($<IF:$<CONFIG:Debug>,/SUBSYSTEM:CONSOLE,/SUBSYSTEM:WINDOWS>)
    else()
        add_link_options($<$<NOT:$<CONFIG:Debug>>:-mwindows>)
    endif()
endif()

add_subdirectory(ImGuiDemo)
add_subdirectory(Triangle)
