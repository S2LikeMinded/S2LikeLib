add_library(S2LL
	S2LL/Core/E2.cpp
	S2LL/Core/Polygon.cpp
	S2LL/Parser/Shapefile.cpp
)

target_include_directories(S2LL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

file(COPY S2LL/
	DESTINATION ${CMAKE_HOME_DIRECTORY}/include/S2LL
	FILES_MATCHING PATTERN "*.hpp"
)
