add_library(sekai SHARED 
	mfcc.cpp
	midi.cpp
	SekaiContext.cpp
	VVDReader.cpp)

set(HEADERS 
	midi.h
	mfcc.h
	SekaiContext.h
	VVDReader.h
	vvd.h
)


set(SEKAI_VERSION_MAJOR 0)
set(SEKAI_VERSION_MINOR 3)
set(SEKAI_VERSION_PATCH 0)
set(SEKAI_VERSION_STRING ${SEKAI_VERSION_MAJOR}.${SEKAI_VERSION_MINOR}.${SEKAI_VERSION_PATCH})
set_target_properties(sekai PROPERTIES VERSION ${SEKAI_VERSION_STRING}
                                           SOVERSION ${SEKAI_VERSION_MAJOR})


install(FILES ${HEADERS} DESTINATION include/sekai)

EXECUTE_PROCESS(COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH OUTPUT_VARIABLE
  CMAKE_ARCH_TRIPLET OUTPUT_STRIP_TRAILING_WHITESPACE)

install(TARGETS sekai
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib/${CMAKE_ARCH_TRIPLET}
)

add_executable(wav2vvd wav2vvd.cpp)
add_executable(vvd2wav vvd2wav.cpp)
target_link_libraries(vvd2wav sekai world sndfile)
target_link_libraries(wav2vvd sekai world sndfile)
