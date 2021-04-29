cmake_minimum_required(VERSION 3.17)
project(untitled1 C)

set(CMAKE_C_STANDARD 99)

set(LWIP_DIR /home/david/repos/lwip)
set(LWIP_CONTRIB_DIR /home/david/repos/lwip/contrib)

set(lwipcontribportunix_SRCS  ${LWIP_CONTRIB_DIR}/ports/unix/port/sys_arch.c)

include(${LWIP_DIR}/src/Filelists.cmake)

set (LWIP_INCLUDE_DIRS
        "${LWIP_DIR}/src/include"
        "${LWIP_CONTRIB_DIR}/ports/unix/port/include")

list(REMOVE_ITEM lwipnoapps_SRCS "${LWIP_DIR}/src/netif/slipif.c")
list(REMOVE_ITEM lwipnoapps_SRCS "${LWIP_DIR}/src/core/ip4.c")
list(REMOVE_ITEM lwipnoapps_SRCS "${LWIP_DIR}/src/core/ipv6/ip6.c")

add_executable(untitled1 main.c ipv6_stub.c ip_inout.c sio.c ${lwipnoapps_SRCS} ${lwipcontribportunix_SRCS})


target_include_directories(untitled1 PRIVATE ${LWIP_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
find_library(LIBUTIL util)
find_library(LIBPTHREAD pthread)
find_library(LIBRT rt)
target_link_libraries(untitled1 PUBLIC ${LIBUTIL} ${LIBPTHREAD} ${LIBRT})
