cmake_minimum_required(VERSION 3.16)

include(generic_compile_options)

set(target_name learn_opengl)

set(target_src_root ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(src_content_dir ${CMAKE_CURRENT_SOURCE_DIR}/content)
option(ENABLE_RENDER_ANNOTATIONS "Whether rendering annotations should be enabled" OFF)

file(GLOB_RECURSE headers_list "${target_src_root}/*.hpp")
file(GLOB_RECURSE sources_list "${target_src_root}/*.cpp")

add_executable(${target_name}
	${headers_list}
	${sources_list}
	"${CMAKE_CURRENT_SOURCE_DIR}/learn_opengl.natvis")

set_generic_compile_options(${target_name} PUBLIC)
target_link_libraries(${target_name}
	eigen
	stb
	glad
	glfw
	fmt
	glm
	spdlog
	imgui
	tinyobjloader
	nlohmann_json::nlohmann_json
	CppReflection
	EverydayTools
	${OPENGL_LIBRARIES})
target_compile_definitions(${target_name} PUBLIC
	-DGLM_FORCE_RADIANS
	-DTINYOBJLOADER_IMPLEMENTATION
	-DSPDLOG_FMT_EXTERNAL
	-DTINYOBJLOADER_IMPLEMENTATION)
target_include_directories(${target_name} PUBLIC ${target_src_root})

if(ENABLE_RENDER_ANNOTATIONS)
	target_compile_definitions(${target_name} PUBLIC "ENABLE_RENDER_ANNOTATIONS")
endif()

add_custom_command(TARGET ${target_name}
	POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
	${src_content_dir}
	$<TARGET_FILE_DIR:${target_name}>/content)
