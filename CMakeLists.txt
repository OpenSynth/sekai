add_library(sekai SHARED epr.cpp hzosc.cpp  mfcc.cpp  midi.cpp obOlaBuffer.cpp common.h epr.h hzosc.h mfcc.h midi.h note.h obOlaBuffer.h vvd.h)

set(HEADERS 
	common.h
	epr.h
	hzosc.h
	mfcc.h
	midi.h
	note.h
	obOlaBuffer.h
	vvd.h
)


set(SEKAI_VERSION_MAJOR 0)
set(SEKAI_VERSION_MINOR 2)
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
