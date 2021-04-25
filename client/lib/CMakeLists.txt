# TODO revise; dubious
if(MINGW)
	message("dest: ${CMAKE_BINARY_DIR}")
	file(COPY "lua51.dll" DESTINATION ${CMAKE_BINARY_DIR})
	file(COPY "zlib1.dll" DESTINATION ${CMAKE_BINARY_DIR})
	file(COPY "glew32.dll" DESTINATION ${CMAKE_BINARY_DIR})
	file(COPY "libpng16-16.dll" DESTINATION ${CMAKE_BINARY_DIR})
	file(COPY "SDL2.dll" DESTINATION ${CMAKE_BINARY_DIR})
endif()
