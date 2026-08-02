cmake_minimum_required(VERSION 3.20)

project(SimpleDoorLock NONE)

# ---- Toolchain -------------------------------------------------------
find_program(SDCC sdcc REQUIRED)
find_program(SDAS sdas8051 REQUIRED)

# ---- Target MCU: STC12C5A56S2 (56KB flash) --------------------------
# uni-STC only ships 12C5AxxS2/PDIP40.h; registers are identical across
# the STC12C5AxxS2 series (and for the LQFP44 package), so the PDIP40
# header is used either way.
set(MCU_VARIANT   STC12C5A56S2_PDIP40)
# Exact Hz matters: the UART baud reload divides MCU_FREQ directly, and
# 11059000 (truncated kHz) would round the reload to 0xFE -> 172800 baud.
set(MCU_FREQ_HZ   11059200)
set(MEMORY_MODEL  --model-large)
set(IRAM_SIZE     256)
set(XRAM_SIZE     1024)
set(FLASH_SIZE    57344)
# Grown from 112 to 160: the deepest chain (MainLoop -> task callback ->
# printf -> putchar -> ringBuf_push...) plus a preempting UART ISR needs
# more than 112 B; IRAM has 51 B spare after the data region, use it.
set(STACK_SIZE    160)
set(HAS_DUAL_DPTR y)

# ---- uni-STC locations ----------------------------------------------
set(UNISTC     ${CMAKE_CURRENT_SOURCE_DIR}/uni-stc)
set(HAL_DIR    ${UNISTC}/hal)
set(DRIVER_DIR ${UNISTC}/drivers)

# ---- Include directories --------------------------------------------
set(INCLUDES
    -I${CMAKE_CURRENT_SOURCE_DIR}/src
    -I${CMAKE_CURRENT_SOURCE_DIR}/src/userTasks
    -I${CMAKE_CURRENT_SOURCE_DIR}/compoent/DEBUG_PRINT
    -I${CMAKE_CURRENT_SOURCE_DIR}/compoent/c-linked-list/include
    -I${CMAKE_CURRENT_SOURCE_DIR}/compoent/ringBuffer/include
    -I${CMAKE_CURRENT_SOURCE_DIR}/compoent/BUS_IO_management
    -I${CMAKE_CURRENT_SOURCE_DIR}/compoent/EventScheduling/include
    -I${CMAKE_CURRENT_SOURCE_DIR}/compoent/debounce/include
    -I${CMAKE_CURRENT_SOURCE_DIR}/compoent/CommandParse/include
    -I${HAL_DIR}
    -I${UNISTC}/include
    -I${DRIVER_DIR}
)

set(CPPFLAGS
    -DMCU_FREQ=${MCU_FREQ_HZ}ul
    -DFLASH_SIZE=${FLASH_SIZE}
    -DBUILD_FOR_${MCU_VARIANT}
    -DBUSMANAGE_DEBUG
    ${INCLUDES}
)

# C99 as required by the application logic.
set(CFLAGS -mmcs51 ${MEMORY_MODEL} --std-c99 --opt-code-size ${CPPFLAGS})

set(LDFLAGS -mmcs51 ${MEMORY_MODEL} --std-c99
    --iram-size ${IRAM_SIZE}
    --xram-loc 0
    --xram-size ${XRAM_SIZE}
    --stack-size ${STACK_SIZE}
    --code-size ${FLASH_SIZE}
)

# ---- Sources ---------------------------------------------------------
# Application + ported door lock logic
set(APP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board_bus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/userTMR_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/userUART_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tickBroadcast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/userTasks/doorLock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/userTasks/userButton.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/userTasks/userTask_doorLock.c
    # TEMP: command tree disabled for isolation testing
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/userTasks/userTask_cmds.c
)

# Ported framework components
set(COMP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/compoent/ringBuffer/ringBuffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/compoent/EventScheduling/EventSchedul.c
    ${CMAKE_CURRENT_SOURCE_DIR}/compoent/BUS_IO_management/bus_io_management.c
    ${CMAKE_CURRENT_SOURCE_DIR}/compoent/debounce/debounce.c
    # TEMP: command tree disabled for isolation testing
    # ${CMAKE_CURRENT_SOURCE_DIR}/compoent/CommandParse/CommandParseTree.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/compoent/CommandParse/cmdUserStringParser.c
)

# uni-STC HAL modules actually used (timer/uart use direct SFRs)
set(HAL_SRCS
    ${HAL_DIR}/gpio-hal.c
    ${HAL_DIR}/reset-hal.c
)

# ---- Build dir & object list ----------------------------------------
set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/obj)
file(MAKE_DIRECTORY ${OBJ})

# Compile a .c into a flattened .rel under ${OBJ}.
# Header dependency tracking: SDCC's native -MMD writes a make-style depfile
# (<output>.d) listing every header the module actually includes; Ninja reads
# it (DEPFILE, requires the Ninja generator) and rebuilds exactly the modules
# a header edit invalidates. Pass -MMD to sdcc itself, NOT via -Wp — routing
# it through the preprocessor directly breaks SDCC's pipeline.
function(sdcc_compile src out_var)
    get_filename_component(name ${src} NAME_WE)
    get_filename_component(dir  ${src} DIRECTORY)
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" tag ${dir})
    set(rel ${OBJ}/${tag}_${name}.rel)
    add_custom_command(
        OUTPUT ${rel}
        COMMAND ${SDCC} ${CFLAGS} -MMD -o ${rel} -c ${src}
        DEPFILE ${OBJ}/${tag}_${name}.d
        DEPENDS ${src} ${ARGN}
        COMMENT "CC ${name}.c"
        VERBATIM
    )
    set(${out_var} ${rel} PARENT_SCOPE)
endfunction()

# Stamp touched on every build so Main.c (which embeds __DATE__/__TIME__)
# is always recompiled and the firmware timestamp stays current. Only Main.c
# depends on it; the rest of the build stays incremental.
set(BUILD_STAMP ${CMAKE_CURRENT_BINARY_DIR}/build_time.stamp)
add_custom_target(gen_build_time ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${BUILD_STAMP}
    BYPRODUCTS ${BUILD_STAMP}
    COMMENT "Refreshing build timestamp"
)

set(OBJS "")
foreach(src ${APP_SRCS} ${COMP_SRCS} ${HAL_SRCS})
    if(src MATCHES "Main\\.c$")
        sdcc_compile(${src} rel ${BUILD_STAMP})
    else()
        sdcc_compile(${src} rel)
    endif()
    list(APPEND OBJS ${rel})
endforeach()

# Dual-DPTR runtime support (STC12 has dual DPTR)
if(HAS_DUAL_DPTR STREQUAL "y")
    set(CRTX ${OBJ}/crtxinit.rel)
    add_custom_command(
        OUTPUT ${CRTX}
        COMMAND ${SDAS} -plosgffw ${CRTX} ${HAL_DIR}/crtxinit.asm
        DEPENDS ${HAL_DIR}/crtxinit.asm
        COMMENT "AS crtxinit.asm"
        VERBATIM
    )
    list(APPEND OBJS ${CRTX})
endif()

# ---- Link ------------------------------------------------------------
set(FW ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex)
add_custom_command(
    OUTPUT ${FW}
    COMMAND ${SDCC} ${LDFLAGS} -o ${FW} ${OBJS}
    DEPENDS ${OBJS}
    COMMENT "LD ${PROJECT_NAME}.hex"
    VERBATIM
)

add_custom_target(firmware ALL DEPENDS ${FW})

# ---- Copy to output/ -------------------------------------------------
add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/output
    COMMAND ${CMAKE_COMMAND} -E copy ${FW} ${CMAKE_CURRENT_SOURCE_DIR}/output/${PROJECT_NAME}.hex
    COMMENT "Copying firmware to output/"
)

# ---- compile_commands.json for clangd --------------------------------
# The build drives SDCC through custom commands, so CMAKE_EXPORT_COMPILE_COMMANDS
# would be empty. Emit a clang-friendly database so clangd can enumerate and
# background-index every translation unit (enables go-to-definition into .c
# files, not just headers). Regenerated on every reconfigure.
set(CDB_ARGS
    "\"clang\"" "\"-xc\"" "\"-std=c99\""
    "\"-include\"" "\"stdbool.h\""
    "\"-DMCU_FREQ=${MCU_FREQ_HZ}ul\""
    "\"-DFLASH_SIZE=${FLASH_SIZE}\""
    "\"-DBUILD_FOR_${MCU_VARIANT}\""
    "\"-DBUSMANAGE_DEBUG\""
)
foreach(inc ${INCLUDES})
    list(APPEND CDB_ARGS "\"${inc}\"")
endforeach()
list(APPEND CDB_ARGS
    "\"-IC:/Program Files/SDCC/include/mcs51\""
    "\"-IF:/mingw64/x86_64-w64-mingw32/include\""
    "\"-Wno-#warnings\"" "\"-Wno-unknown-pragmas\""
    "\"-Wno-unused-variable\"" "\"-Wno-unused-function\"" "\"-Wno-unused-parameter\""
)
string(REPLACE ";" ", " CDB_ARGS_STR "${CDB_ARGS}")

set(CDB_CONTENT "[\n")
set(_first TRUE)
foreach(src ${APP_SRCS} ${COMP_SRCS} ${HAL_SRCS})
    if(NOT _first)
        string(APPEND CDB_CONTENT ",\n")
    endif()
    set(_first FALSE)
    string(APPEND CDB_CONTENT
        "  {\n"
        "    \"directory\": \"${CMAKE_CURRENT_SOURCE_DIR}\",\n"
        "    \"arguments\": [${CDB_ARGS_STR}, \"-c\", \"${src}\"],\n"
        "    \"file\": \"${src}\"\n"
        "  }")
endforeach()
string(APPEND CDB_CONTENT "\n]\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json "${CDB_CONTENT}")
