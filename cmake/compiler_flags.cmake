# Compiler flags for all WRITEOVER-07 targets.
# MSVC: /W4 /WX /utf-8 /permissive-. GCC/Clang: equivalent guard set.
function(writeover_configure_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /utf-8 /permissive- /EHsc)
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Werror -Wpedantic
        )
    endif()

    if(WO_ENABLE_DEV)
        target_compile_definitions(${target} PRIVATE WO_ENABLE_DEV=1)
    else()
        target_compile_definitions(${target} PRIVATE WO_ENABLE_DEV=0)
    endif()
endfunction()