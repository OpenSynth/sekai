add_library(sekai SHARED 
	src/mfcc.cpp
	src/midi.cpp
	src/SekaiContext.cpp
	src/VVDReader.cpp
	src/common.cpp
)

set(HEADERS 
	src/sekai/midi.h
	src/sekai/mfcc.h
	src/sekai/SekaiContext.h
	src/sekai/VVDReader.h
	src/sekai/vvd.h
	src/sekai/common.h
)


set(SEKAI_VERSION_MAJOR 0)
set(SEKAI_VERSION_MINOR 3)
set(SEKAI_VERSION_PATCH 0)
set(SEKAI_VERSION_STRING ${SEKAI_VERSION_MAJOR}.${SEKAI_VERSION_MINOR}.${SEKAI_VERSION_PATCH})
set_target_properties(sekai PROPERTIES VERSION ${SEKAI_VERSION_STRING}
                                           SOVERSION ${SEKAI_VERSION_MAJOR})
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


