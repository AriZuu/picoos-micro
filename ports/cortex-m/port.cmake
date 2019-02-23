include_guard(GLOBAL)

set(FILES_PORT ports/${PORT}/u_generic_spin.c)

if(CPU)

  file(GLOB FILES_CPU ports/${PORT}/${CPU}/*.c)

endif()

set(FILES_PORT ${FILES_PORT} ${FILES_CPU})

