add_executable(world_rtsynth jack.cpp main.cpp synth.cpp)
add_definitions(-std=gnu++0x)
target_link_libraries(world_rtsynth jack world sndfile sekai)
