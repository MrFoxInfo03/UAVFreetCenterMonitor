cmake_minimum_required(VERSION 3.30.2)
project(mavlink_soft)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(collector src/collector.cpp)

#MAVSDK
set(MAVSDK_PATH ${CMAKE_SOURCE_DIR}/mavsdk-windows-x86)

include_directories(${MAVSDK_PATH}/include)
link_directories(${MAVSDK_PATH}/lib)

add_subdirectory(nats.c)
target_link_libraries(mavlink_soft PRIVATE nats)

target_link_libraries(collector 
    mavsdk
)

add_custom_command(TARGET collector POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${MAVSDK_PATH}/bin/*.dll
    $<TARGET_FILE_DIR:collector>
)