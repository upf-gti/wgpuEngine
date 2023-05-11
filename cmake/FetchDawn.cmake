include(FetchContent)

FetchContent_Declare(
	dawn
	GIT_REPOSITORY https://github.com/blitz-research/dawn
	GIT_TAG        openxr
	GIT_SUBMODULES
	GIT_SHALLOW ON
)

function(make_dawn_available)
	FetchContent_GetProperties(dawn)
	if (NOT dawn_POPULATED)
		FetchContent_Populate(dawn)
		find_package(PythonInterp 3 REQUIRED)

		message(STATUS "Running fetch_dawn_dependencies:")
		execute_process(
			COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/fetch_dawn_dependencies.py"
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/dawn-src"
		)

		set(DAWN_BUILD_SAMPLES OFF)
		set(TINT_BUILD_TINT OFF) # TODO
		set(TINT_BUILD_SAMPLES OFF)
		set(TINT_BUILD_DOCS OFF)
		set(TINT_BUILD_TESTS OFF)

		set(DAWN_ENABLE_D3D12 OFF)
		set(DAWN_ENABLE_METAL OFF)
		set(DAWN_ENABLE_NULL OFF)
		set(DAWN_ENABLE_DESKTOP_GL OFF)
		set(DAWN_ENABLE_OPENGLES OFF)
		set(DAWN_ENABLE_VULKAN ON)

		set(TINT_BUILD_SPV_READER OFF)
		set(TINT_BUILD_WGSL_READER ON)
		set(TINT_BUILD_GLSL_WRITER OFF)
		set(TINT_BUILD_HLSL_WRITER OFF)
		set(TINT_BUILD_MSL_WRITER OFF)
		set(TINT_BUILD_SPV_WRITER ON)
		set(TINT_BUILD_WGSL_WRITER ON) # Because of crbug.com/dawn/1481
		set(TINT_BUILD_FUZZERS OFF)
		set(TINT_BUILD_SPIRV_TOOLS_FUZZER OFF)
		set(TINT_BUILD_AST_FUZZER OFF)
		set(TINT_BUILD_REGEX_FUZZER OFF)
		set(TINT_BUILD_BENCHMARKS OFF)
		set(TINT_BUILD_TESTS OFF)
		set(TINT_BUILD_AS_OTHER_OS OFF)
		set(TINT_BUILD_REMOTE_COMPILE OFF)

		add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR})
	endif ()

	set(AllDawnTargets
		core_tables
		dawn_common
		dawn_glfw
		dawn_headers
		dawn_native
		dawn_platform
		dawn_proc
		dawn_utils
		dawn_wire
		dawncpp
		dawncpp_headers
		emscripten_bits_gen
		enum_string_mapping
		extinst_tables
		webgpu_dawn
		webgpu_headers_gen
	)

	set(AllGlfwTargets
		glfw
		update_mappings
	)
	
	foreach (Target ${AllDawnTargets})
		if (TARGET ${Target})
			set_property(TARGET ${Target} PROPERTY FOLDER "External/Dawn")
		endif()
	endforeach()

	foreach (Target ${AllGlfwTargets})
		if (TARGET ${Target})
			set_property(TARGET ${Target} PROPERTY FOLDER "External/GLFW3")
		endif()
	endforeach()

	# This is likely needed for other targets as well
	# TODO: Notify this upstream
	target_include_directories(dawn_utils PUBLIC "${CMAKE_BINARY_DIR}/_deps/dawn-src/src")
endfunction()
