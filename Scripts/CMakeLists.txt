add_library(Scripts
    SHARED
        src/Script.cpp
)

if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(NATIVE_SCRIPT_EXPORTS)
endif()

target_include_directories(
    Scripts
    PUBLIC
        "include"
)

#target_link_libraries(
#    Scripts
#    PUBLIC
#        Engine
#)
