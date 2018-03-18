add_library(sekai SHARED 
	src/mfcc.cpp
	src/midi.cpp
	src/SekaiContext.cpp
	src/VVDReader.cpp
	src/OLABuffer.cpp
	src/WorldSynth2.cpp
	src/common.cpp
	src/hzosc.cpp
	src/epr.cpp
)

set(HEADERS 
	src/sekai/midi.h
	src/sekai/mfcc.h
	src/sekai/SekaiContext.h
	src/sekai/VVDReader.h
	src/sekai/OLABuffer.h
	src/sekai/WorldSynth2.h
	src/sekai/vvd.h
	src/sekai/common.h
	src/sekai/hzosc.h
	src/sekai/epr.h
)

ADD_DEFINITIONS(-std=c++11)

set(SEKAI_VERSION_MAJOR 0)
set(SEKAI_VERSION_MINOR 4)
set(SEKAI_VERSION_PATCH 0)
set(SEKAI_VERSION_STRING ${SEKAI_VERSION_MAJOR}.${SEKAI_VERSION_MINOR}.${SEKAI_VERSION_PATCH})
set_target_properties(sekai PROPERTIES VERSION ${SEKAI_VERSION_STRING}
                                           SOVERSION ${SEKAI_VERSION_MAJOR})
                                           
target_link_libraries(sekai fftw3)
                                           
include_directories(src)

install(FILES ${HEADERS} DESTINATION include/sekai)

EXECUTE_PROCESS(COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH OUTPUT_VARIABLE
  CMAKE_ARCH_TRIPLET OUTPUT_STRIP_TRAILING_WHITESPACE)
  
add_executable(wav2vvd src/wav2vvd.cpp)
add_executable(vvd2wav src/vvd2wav.cpp)
target_link_libraries(vvd2wav sekai world sndfile)
target_link_libraries(wav2vvd sekai world sndfile)

install(TARGETS sekai wav2vvd vvd2wav
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib/${CMAKE_ARCH_TRIPLET}
)

#add_executable(realtime_demo examples/realtime_demo/jack.cpp examples/realtime_demo/main.cpp examples/realtime_demo/synth.cpp)
#add_definitions(-std=gnu++0x)
#target_link_libraries(realtime_demo jack world sndfile sekai pthread jsoncpp)

#add_executable(mbrola examples/mbrolaclone/mbrola_dummy.cpp examples/mbrolaclone/voicebank_meta.cpp examples/mbrolaclone/voice_synth.cpp)
#target_link_libraries(mbrola world sekai jsoncpp)
