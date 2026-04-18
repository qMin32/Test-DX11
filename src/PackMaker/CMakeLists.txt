file(GLOB_RECURSE FILE_SOURCES "*.h" "*.c" "*.cpp")

add_executable(PackMaker ${FILE_SOURCES})
set_target_properties(PackMaker PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_link_libraries(PackMaker
	libzstd_static
	sodium
)

add_custom_command(TARGET PackMaker POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:PackMaker>" "${CMAKE_SOURCE_DIR}/../../Client/assets/PackMaker.exe"
	COMMENT "Copying PackMaker to Client/assets/"
)
