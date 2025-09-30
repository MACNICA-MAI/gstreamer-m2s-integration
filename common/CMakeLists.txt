cmake_minimum_required(VERSION 3.9)
project(common_m2s_lib
		VERSION 1.0.0
		DESCRIPTION "common_lib"
		LANGUAGES C)
		
# The default M2S root directory
set(M2S_TOP ${PROJECT_SOURCE_DIR}/..)

add_library(common_m2s STATIC tr_offset.c)

target_include_directories(common_m2s PRIVATE
							${M2S_TOP}/library/include
							)
set_target_properties(common_m2s
    PROPERTIES
    VERSION ${PROJECT_VERSION})
