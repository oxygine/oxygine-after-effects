cmake_minimum_required (VERSION 2.6)
project (HelloViewerAE)

add_subdirectory(../../../../oxygine-framework/ oxygine-framework)
add_definitions(${OXYGINE_DEFINITIONS})
include_directories(${OXYGINE_INCLUDE_DIRS})
link_directories(${OXYGINE_LIBRARY_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OXYGINE_CXX_FLAGS}")



file(GLOB AESRC #RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/
		../../../ae-movie/src/*.c)
file(GLOB AEHDR #RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/
		../../..//ae-movie/src/*.h)
set (AELIBSOURCES ${AESRC} ${AEHDR})
source_group(aesdk FILES ${AELIBSOURCES})	


file(GLOB OXAESRC #RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/
		../../../src/ae/*.cpp)
file(GLOB OXAEHEADERS #RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/
		../../../src/ae/*.h)
set (OXAESOURCES ${OXAESRC} ${OXAEHEADERS})
source_group(oxae FILES ${OXAESOURCES})	





add_executable(HelloViewerAE ../src/example.cpp ../src/main.cpp  ../src/example.h ../src/test.cpp ../src/AEMovieWork.cpp ${AELIBSOURCES} ${OXAESOURCES})
target_link_libraries(HelloViewerAE ${OXYGINE_CORE_LIBS})

include_directories(../../../src/)


add_definitions(-DAE_MOVIE_STREAM_NO_CACHE=1 -DAE_TIME_DEFINE=1 -DAE_TIME_MILLISECOND=1 -DAE_MOVIE_SAFE=1 -DAEVIEWER=1)
include_directories(../../../src/)
include_directories(../../../ae-movie/include)



if (WIN32) #disable console mode for VC++
	set_target_properties(HelloViewerAE PROPERTIES WIN32_EXECUTABLE TRUE)
endif(WIN32)



if (EMSCRIPTEN)
	SET(CMAKE_EXECUTABLE_SUFFIX ".html")	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s NO_EXIT_RUNTIME=1 -s WASM=0 -s WARN_ON_UNDEFINED_SYMBOLS=1 --memory-init-file 0 -s TOTAL_MEMORY=50331648 -s FORCE_FILESYSTEM=1")
	em_link_pre_js(HelloViewerAE  ${OXYGINE_JS_LIBRARIES}  ${CMAKE_CURRENT_SOURCE_DIR}/data.js)
endif(EMSCRIPTEN)