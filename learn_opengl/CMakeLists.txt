cmake_minimum_required(VERSION 3.20)

set(target_name learn_opengl)

set(target_src_root ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(content_dir_rel content)
set(shaders_dir_rel ${content_dir_rel}/shaders)
set(textures_dir_rel ${content_dir_rel}/textures)
set(models_dir_rel ${content_dir_rel}/models)

set(src_content_dir  ${CMAKE_CURRENT_SOURCE_DIR}/${content_dir_rel})
set(src_shaders_dir  ${CMAKE_CURRENT_SOURCE_DIR}/${shaders_dir_rel})
set(src_textures_dir ${CMAKE_CURRENT_SOURCE_DIR}/${textures_dir_rel})
set(src_models_dir ${CMAKE_CURRENT_SOURCE_DIR}/${models_dir_rel})

set(dst_content_dir  ${CMAKE_CURRENT_BINARY_DIR}/${content_dir_rel})
set(dst_shaders_dir  ${CMAKE_CURRENT_BINARY_DIR}/${shaders_dir_rel})
set(dst_textures_dir ${CMAKE_CURRENT_BINARY_DIR}/${textures_dir_rel})
set(dst_models_dir ${CMAKE_CURRENT_BINARY_DIR}/${models_dir_rel})

file(GLOB_RECURSE headers_list "${target_src_root}/*.hpp")
file(GLOB_RECURSE sources_list "${target_src_root}/*.cpp")

add_executable( ${target_name}
	${headers_list}
	${sources_list}
	"${CMAKE_CURRENT_SOURCE_DIR}/learn_opengl.natvis")

target_link_libraries(${target_name}
	stb
	glad
	glfw
	fmt
	glm
	spdlog
	imgui
	tinyobjloader
	nlohmann_json::nlohmann_json
	${OPENGL_LIBRARIES})
target_compile_definitions(${target_name} PUBLIC
	-DGLM_FORCE_RADIANS
	-DTINYOBJLOADER_IMPLEMENTATION
	-DSPDLOG_FMT_EXTERNAL
	-DTINYOBJLOADER_IMPLEMENTATION)
target_include_directories(${target_name} PUBLIC ${target_src_root})

if(MSVC)
	set(compile_opts
		/W4
		/permissive # Enforces standards conformance.
		/w14242 # 'identfier': conversion from 'type1' to 'type1', possible loss of data
		/w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
		/w14263 # 'function': member function does not override any base class virtual member function
		/w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
		/w14287 # 'operator': unsigned/negative constant mismatch
		/we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
		/w14296 # 'operator': expression is always 'boolean_value'
		/w14311 # 'variable': pointer truncation from 'type1' to 'type2'
		/w14545 # expression before comma evaluates to a function which is missing an argument list
		/w14546 # function call before comma missing argument list
		/w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
		/w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
		/w14555 # expression has no effect; expected expression with side-effect
		/w14619 # pragma warning: there is no warning number 'number'
		/w14640 # Enable warning on thread un-safe static member initialization
		/w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
		/w14905 # wide string literal cast to 'LPSTR'
		/w14906 # string literal cast to 'LPWSTR'
		/w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
	)

else()
	# command warnings for clang and gcc
	set(compile_opts
		-Werror # treat warnings as errors
		-pedantic # Warn on language extensions
		-Wall -Wextra # reasonable and standard
		-Wpedantic # (all versions of GCC, Clang >= 3.2) warn if non-standard C++ is used
		-Wshadow # warn the user if a variable declaration shadows one from a parent context
		-Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
		-Wold-style-cast # warn for c-style casts
		-Wcast-align # warn for potential performance problem casts
		-Wunused # warn on anything being unused
		-Woverloaded-virtual # warn if you overload (not override) a virtual function
		-Wconversion # warn on type conversions that may lose data
		-Wnull-dereference # (only in GCC >= 6.0) warn if a null dereference is detected
		-Wdouble-promotion # (GCC >= 4.6, Clang >= 3.8) warn if float is implicit promoted to double
		-Wfloat-equal
		-Wcast-qual
		-Wzero-as-null-pointer-constant
		-Wsign-conversion # (Clang all versions, GCC >= 4.3) warn on sign conversions
	)

	if (CMAKE_CXX_COMPILER_ID STREQUAL "GCC")
		list(APPEND compile_opts
			-Wduplicated-cond # (only in GCC >= 6.0) warn if if / else chain has duplicated conditions
			-Wduplicated-branches # (only in GCC >= 7.0) warn if if / else branches have duplicated code
			-Wlogical-op # (only in GCC) warn about logical operations being used where bitwise were probably wanted
			-Wstrict-null-sentinel
		)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# only special branch of Clang currently but it is really helpful
		#list(APPEND compile_opts
		#	-Wlifetime # shows object lifetime issues
		#)
	endif()
endif()

target_compile_options(${target_name} PRIVATE ${compile_opts})

function(copy_content_fn)
	set(options)
	set(oneValueArgs TARGET SRC DST)
	set(multiValueArgs)
	cmake_parse_arguments(copy_content_fn "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	# gather source file paths relative to root
	file(GLOB_RECURSE files_list RELATIVE ${copy_content_fn_SRC} "${copy_content_fn_SRC}/**")

	add_custom_target(${copy_content_fn_TARGET})

	# generate copy commands for each file
	foreach(file_src_rel ${files_list})
		set(file_src_abs ${copy_content_fn_SRC}/${file_src_rel})
		set(file_dst_abs ${copy_content_fn_DST}/${file_src_rel})

		# create desctination directory
		get_filename_component(file_dst_dir ${file_dst_abs} DIRECTORY)
		file(MAKE_DIRECTORY ${file_dst_dir})

		add_custom_command(
			TARGET ${copy_content_fn_TARGET}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${file_src_abs} ${file_dst_abs}
			MAIN_DEPENDENCY ${file_src_abs})
	endforeach()
endfunction()

add_custom_target(copy_content)
add_dependencies(${target_name} copy_content)

copy_content_fn(TARGET copy_textures SRC ${src_textures_dir} DST ${dst_textures_dir})
add_dependencies(copy_content copy_textures)

copy_content_fn(TARGET copy_models SRC ${src_models_dir} DST ${dst_models_dir})
add_dependencies(copy_content copy_models)

copy_content_fn(TARGET copy_shaders SRC ${src_shaders_dir} DST ${dst_shaders_dir})
add_dependencies(copy_content copy_shaders)

