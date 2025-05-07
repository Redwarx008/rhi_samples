function(compile_shader TARGET_NAME SHADERS SHADER_INCLUDE_FOLDER GENERATED_DIR GLSLANG_BIN)

    set(WORKING_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    set(ALL_GENERATED_SPV_FILES "")

    if(UNIX)
        execute_process(COMMAND chmod a+x ${GLSLANG_BIN})
    endif()

    foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        string(REPLACE "." "_" HEADER_NAME ${SHADER_NAME})
        string(TOUPPER ${HEADER_NAME} GLOBAL_SHADER_VAR)

        set(SPV_FILE "${GENERATED_DIR}/${SHADER_NAME}.spv")

        if(SHADER_INCLUDE_FOLDER STREQUAL "")
        add_custom_command(
            OUTPUT ${SPV_FILE}
            COMMAND ${GLSLANG_BIN} -V100 ${SHADER} -o ${SPV_FILE} 
            DEPENDS ${SHADER}
            WORKING_DIRECTORY "${WORKING_DIR}")
        else()
        add_custom_command(
            OUTPUT ${SPV_FILE}
            COMMAND ${GLSLANG_BIN} -I${SHADER_INCLUDE_FOLDER} -V100 ${SHADER} -o ${SPV_FILE} 
            DEPENDS ${SHADER}
            WORKING_DIRECTORY "${WORKING_DIR}")
        endif()

        list(APPEND ALL_GENERATED_SPV_FILES ${SPV_FILE})

    endforeach()

    add_custom_target(${TARGET_NAME}
        DEPENDS ${ALL_GENERATED_SPV_FILES} SOURCES ${SHADERS})

endfunction()