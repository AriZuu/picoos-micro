include_guard(GLOBAL)

set(FILES_PORT
    ports/${PORT}/u_spin.c
    ports/${PORT}/hal5xx6xx/HAL_FLASH.c
    ports/${PORT}/hal5xx6xx/HAL_PMAP.c
    ports/${PORT}/hal5xx6xx/HAL_PMM.c
    ports/${PORT}/hal5xx6xx/HAL_TLV.c
    ports/${PORT}/hal5xx6xx/HAL_UCS.c)

set(PORT_INCLUDES ports/${PORT}/hal5xx6xx)
