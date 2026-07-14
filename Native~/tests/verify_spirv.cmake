if(NOT DEFINED SHADER_DIR)
    message(FATAL_ERROR "SHADER_DIR is required")
endif()

set(shaders
    depth.rgen.spv
    depth.rmiss.spv
    depth.rchit.spv
)

foreach(shader IN LISTS shaders)
    set(path "${SHADER_DIR}/${shader}")
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "Missing SPIR-V output: ${path}")
    endif()

    file(SIZE "${path}" size)
    if(size LESS 20)
        message(FATAL_ERROR "SPIR-V output too small: ${path} (${size})")
    endif()

    file(READ "${path}" magic HEX OFFSET 0 LIMIT 4)
    string(TOLOWER "${magic}" magic)
    if(NOT magic STREQUAL "03022307")
        message(FATAL_ERROR "Invalid SPIR-V magic: ${path} (${magic})")
    endif()
endforeach()

message(STATUS "[OK] generated SPIR-V outputs validated")
