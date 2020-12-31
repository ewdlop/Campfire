set(SCRIPT_HEADERS
    include/Scripts/NativeScript.h
    #include/Scripts/Script.h
    include/Scripts/Player.h
)

set(SCRIPT_SRC
    src/Script.cpp
)

if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(NATIVE_SCRIPT_EXPORTS)
endif()

add_library(Scripts
    SHARED
        ${SCRIPT_SRC}
)

target_sources(Scripts
    PRIVATE
        ${SCRIPT_SRC}
    PUBLIC
        ${SCRIPT_HEADERS}
)

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
