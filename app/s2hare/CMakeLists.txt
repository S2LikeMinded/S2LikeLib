add_executable(S2Hare
	main.cpp
	Hare.cpp
)

# Sources embed UTF-8 literals (e.g. U+00B0 degree sign); MSVC needs /utf-8
# to interpret them as UTF-8 instead of the active code page.
if(MSVC)
	target_compile_options(S2Hare PRIVATE /utf-8)
endif()

target_link_libraries(S2Hare PRIVATE
	S2LL
	S2LL_AppSupport
)

install(TARGETS S2Hare
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
