cmake_minimum_required(VERSION 3.22)

set(BUILD_SHARED_LIBS YES)

find_package(WIL CONFIG REQUIRED)

configure_file(resource.h resource.h COPYONLY)
configure_file(resource.rc.in resource.rc @ONLY)

add_library(lldext
	"injectdll.cpp"
	"ext.cpp"
	"ext.def"
	"${CMAKE_CURRENT_BINARY_DIR}/resource.rc"
)

set_property(TARGET lldext PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(lldext PRIVATE
	WIL::WIL
	dbgeng
)

target_compile_definitions(lldext PRIVATE
	EXT_MAJOR_VER=${lldext_VERSION_MAJOR}
	EXT_MINOR_VER=${lldext_VERSION_MINOR}
	EXT_PATCH_VER=${lldext_VERSION_PATCH}
	EXT_TWEAK_VER=${lldext_VERSION_TWEAK}
)

set_target_properties(lldext PROPERTIES
	LINK_OPTIONS LINKER:/MANIFEST:NO
)

install(TARGETS lldext RUNTIME DESTINATION ".")
install(FILES $<TARGET_PDB_FILE:lldext> DESTINATION ".")

