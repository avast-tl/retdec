@PACKAGE_INIT@

find_package(Threads REQUIRED)
if(UNIX OR MINGW)
	find_package(ZLIB REQUIRED)
endif()

if(NOT TARGET llvm-libs)
	add_library(llvm-libs INTERFACE)
	add_library(retdec::llvm-libs ALIAS llvm-libs)
	foreach(LLVM_LIB @PACKAGE_LLVM_LIBS_PATHS@)
		target_link_libraries(llvm-libs INTERFACE
			${LLVM_LIB}
		)
	endforeach(LLVM_LIB)
endif()

if(NOT TARGET retdec::llvm)
    include(${CMAKE_CURRENT_LIST_DIR}/retdec-llvm-targets.cmake)
endif()