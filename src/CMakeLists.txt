cmake_minimum_required(VERSION 3.4)

project(	"CloudPlusCoin" 
			VERSION 0.1.0
			DESCRIPTION "CloudCoin C++ API")
set(PROJECT_SHORTNAME "cpc")



# --- soruce
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

# --- target
add_library(${PROJECT_SHORTNAME} SHARED ${SOURCES})
add_library(rot::${PROJECT_SHORTNAME} ALIAS ${PROJECT_SHORTNAME})

# --- values
set_target_properties(${PROJECT_SHORTNAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAG} -pedantic-errors"
)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# --- dependencies
find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)
pkg_check_modules(UUID REQUIRED uuid)

target_link_libraries(${PROJECT_SHORTNAME} PUBLIC
	${JANSSON_LIBRARIES}
	CURL::libcurl
	${UUID_LIBRARIES})

target_include_directories(${PROJECT_SHORTNAME} PUBLIC
	${PROJECT_SOURCE_DIR}/include 
	${UUID_INCLUDE_DIRS} 
	${JANSSON_INCLUDE_DIRS})



if (BUILD_GUIEXAMPLE)
	set(ADDITION_NAME "guiexample")

	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	file(GLOB SOURCES 
		${PROJECT_SOURCE_DIR}/example/gui/*.cpp
		${PROJECT_SOURCE_DIR}/example/gui/*.qrc
	)
	
	find_package(Qt5 COMPONENTS Widgets Qml Quick REQUIRED)

	add_executable(${ADDITION_NAME} ${SOURCES})

	target_link_libraries(${ADDITION_NAME}
		Qt5::Qml
		Qt5::Quick
		Qt5::Widgets
	)
endif ()

if (BUILD_TEST)
endif ()
