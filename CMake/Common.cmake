macro(m_generate_acg_project _file_name)
	set(acg_project "${_file_name}Acg")
	set(acg_xml "${_file_name}.xml")
	
	project(${acg_project})
	
	set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

	file(GLOB sources "*.xml")
	#message("sources: " ${sources})
	
	get_filename_component(acg_xml_abs "${acg_xml}" ABSOLUTE)
	#message("acg_xml_abs: " ${acg_xml_abs})
	
	set(generated_files "${PROJECT_NAME}.h" "${PROJECT_NAME}.cpp")
	#message("generated_files: " ${generated_files})
	
	file(REMOVE ${generated_files})
	
	add_custom_command(
	    OUTPUT ${generated_files}
	    COMMAND "${CMAKE_SOURCE_DIR}\\External\\Loong SDK\\Bin\\${arch_folder}\\Release\\LoCompiler.exe"
	    ARGS -src "${acg_xml_abs}"
	    DEPENDS "${sources}"
	    COMMENT "Generate ${PROJECT_NAME}"
	)   
	
	add_custom_target(${PROJECT_NAME} DEPENDS ${generated_files} VERBATIM SOURCES ${sources})
endmacro()

macro(m_add_source_group_recursive)
  file(GLOB_RECURSE sources
    "${PROJECT_SOURCE_DIR}/*.cpp"
    "${PROJECT_SOURCE_DIR}/*.h"
  )

  foreach(current_source_file ${sources})
    file(RELATIVE_PATH current_folder ${PROJECT_SOURCE_DIR} ${current_source_file})
    get_filename_component(current_file_name ${current_folder} NAME)
    string(REPLACE ${current_file_name} "" current_folder ${current_folder})
    if(NOT current_folder STREQUAL "")
      string(REGEX REPLACE "/+$" "" current_folder ${current_folder})
      string(REPLACE "/" "\\" current_folder ${current_folder})
      source_group("${current_folder}" FILES ${current_source_file})
    endif()
  endforeach()
endmacro()

macro(m_stop_cmake)
	message(FATAL_ERROR "stop cmake: ${PROJECT_NAME}")
endmacro()

# =================================================
# include header
# =================================================

macro(m_inc_dir_solution)
	include_directories("${CMAKE_SOURCE_DIR}/Inc")
endmacro()


# =================================================
# link dir
# =================================================

macro(m_link_dir_boost)
	link_directories("${CMAKE_SOURCE_DIR}/External/boost/lib/${arch_folder}")
endmacro()


# =================================================
# link lib
# =================================================

macro(m_link_lib_loong_sdk)
	target_link_libraries(${PROJECT_NAME} 
		debug 		"${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Debug/LoCommon.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Release/LoCommon.lib"
		debug 		"${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Debug/LoSeedClient.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Release/LoSeedClient.lib"
		debug 		"${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Debug/LoCore.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Release/LoCore.lib"
	)
endmacro()

macro(m_link_lib_socket)
	target_link_libraries(${PROJECT_NAME} 
		debug 		"${CMAKE_SOURCE_DIR}/External/Socket/Lib/${arch_folder}/Debug/Encrypt.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/Socket/Lib/${arch_folder}/Release/Encrypt.lib"
		debug 		"${CMAKE_SOURCE_DIR}/External/Socket/Lib/${arch_folder}/Debug/Net.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/Socket/Lib/${arch_folder}/Release/Net.lib"
	)
endmacro()

macro(m_link_lib_jsoncpp)
	target_link_libraries(${PROJECT_NAME} 
		debug 		"${CMAKE_SOURCE_DIR}/External/JsonCpp/1.8.1/Lib/${arch_folder}/Debug/jsoncpp.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/JsonCpp/1.8.1/Lib/${arch_folder}/Release/jsoncpp.lib"
	)
endmacro()

macro(m_link_lib_snappy)
	target_link_libraries(${PROJECT_NAME} 
		debug 		"${CMAKE_SOURCE_DIR}/External/Snappy/1.1.7/Lib/${arch_folder}/Debug/snappy.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/Snappy/1.1.7/Lib/${arch_folder}/Release/snappy.lib"
	)
endmacro()

macro(m_link_lib_protobuf)
	target_link_libraries(${PROJECT_NAME}
		debug 		"${CMAKE_SOURCE_DIR}/External/protobuf/3.5.2/Lib/${arch_folder}/Debug/libprotobuf.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/protobuf/3.5.2/Lib/${arch_folder}/Release/libprotobuf.lib"
	)
endmacro()

macro(m_link_lib_slgcody)
	target_link_libraries(${PROJECT_NAME}
		debug 		"${CMAKE_SOURCE_DIR}/External/SlgCody/Lib/${arch_folder}/Debug/slgcody.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/SlgCody/Lib/${arch_folder}/Release/slgcody.lib"
	)
endmacro()

macro(m_link_lib_pnb)
	target_link_libraries(${PROJECT_NAME}
		debug 		"${CMAKE_SOURCE_DIR}/External/pnb/lib/${arch_folder}/pnb.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/pnb/Lib/${arch_folder}/pnb.lib"
	)
endmacro()

macro(m_link_lib_emoney)
	target_link_libraries(${PROJECT_NAME} 
		debug 		"${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Debug/EmoneyChk.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/Loong SDK/Lib/${arch_folder}/Release/EmoneyChk.lib"
	)
endmacro()

macro(m_link_lib_curl)
	target_link_libraries(${PROJECT_NAME} 
		debug 		"${CMAKE_SOURCE_DIR}/External/curl/lib/${arch_folder}/Debug/libcurl.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/curl/lib/${arch_folder}/Release/libcurl.lib"
	)
endmacro()

macro(m_link_lib_db)
	target_link_libraries(${PROJECT_NAME} 
		debug 		"${CMAKE_SOURCE_DIR}/External/DB/Lib/${arch_folder}/Debug/DB.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/DB/Lib/${arch_folder}/Release/DB.lib"
		debug 		"${CMAKE_SOURCE_DIR}/External/DB/Lib/${arch_folder}/Debug/libmysql.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/DB/Lib/${arch_folder}/Release/libmysql.lib"
		debug 		"${CMAKE_SOURCE_DIR}/External/DB/Lib/${arch_folder}/Debug/MySQLConnector.lib"
		optimized "${CMAKE_SOURCE_DIR}/External/DB/Lib/${arch_folder}/Release/MySQLConnector.lib"
	)
endmacro()

#=================================================


macro(m_add_project_pch)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/YuPrecomp.h")
	set_source_files_properties(Precomp.cpp PROPERTIES COMPILE_FLAGS "/YcPrecomp.h")
endmacro()

macro(m_add_project_pch_std)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/Yustdafx.h")
	set_source_files_properties(stdafx.cpp PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
endmacro()

macro(m_add_project_prefix)
	set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "Lp")
endmacro()


#=================================================

macro(m_set_vs_debugger_working_directory)
	set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Run)
endmacro()