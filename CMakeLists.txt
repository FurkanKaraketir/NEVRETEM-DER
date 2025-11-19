cmake_minimum_required(VERSION 3.16)
project(StudentManager VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler-specific flags
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
elseif(MINGW)
    # MinGW-specific flags
    add_compile_options(-Wall -Wextra)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

# Fetch QXlsx for Excel support
include(FetchContent)
FetchContent_Declare(
    QXlsx
    GIT_REPOSITORY https://github.com/QtExcel/QXlsx.git
    GIT_TAG master
    SOURCE_SUBDIR QXlsx
)

# Don't build QXlsx examples
set(QXLSX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Use modern FetchContent API
FetchContent_MakeAvailable(QXlsx)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/studentdialog.cpp
    src/student.cpp
    src/firestoreservice.cpp
    src/firebasestorageservice.cpp
    src/firebaseauthservice.cpp
    src/logindialog.cpp
    src/statisticsdialog.cpp
    src/thememanager.cpp
)

set(HEADERS
    src/mainwindow.h
    src/studentdialog.h
    src/student.h
    src/firestoreservice.h
    src/firebasestorageservice.h
    src/firebaseauthservice.h
    src/logindialog.h
    src/statisticsdialog.h
    src/thememanager.h
)

set(UI_FILES
    src/mainwindow.ui
    src/studentdialog.ui
)

qt_add_executable(StudentManager ${SOURCES} ${HEADERS})
qt_add_resources(StudentManager "resources" FILES resources/style.qss)
qt_add_resources(StudentManager "icons" PREFIX "/" FILES src/logo.jpg)

target_link_libraries(StudentManager 
    PRIVATE 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network
    QXlsx
)

# Include QXlsx headers
target_include_directories(StudentManager PRIVATE ${qxlsx_SOURCE_DIR}/QXlsx/header)

# Set output directory
set_target_properties(StudentManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy universities.json to the build directory
configure_file(${CMAKE_SOURCE_DIR}/src/universities.json ${CMAKE_BINARY_DIR}/bin/universities.json COPYONLY)

# Windows deployment configuration
if(WIN32)
    # Set executable properties for Windows
    set_target_properties(StudentManager PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "StudentManager"
    )
    
    # Find Qt installation path for deployment
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
    set(QT_WINDEPLOYQT_EXECUTABLE "${QT_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
    
    # Add custom target for Windows deployment
    if(EXISTS ${QT_WINDEPLOYQT_EXECUTABLE})
        add_custom_target(deploy_windows
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/deploy_windows
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/deploy_windows
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:StudentManager> ${CMAKE_BINARY_DIR}/deploy_windows/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/config.example.ini ${CMAKE_BINARY_DIR}/deploy_windows/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/universities.json ${CMAKE_BINARY_DIR}/deploy_windows/
            COMMAND ${QT_WINDEPLOYQT_EXECUTABLE} --verbose 2 --dir ${CMAKE_BINARY_DIR}/deploy_windows --libdir ${CMAKE_BINARY_DIR}/deploy_windows --plugindir ${CMAKE_BINARY_DIR}/deploy_windows/plugins --no-translations --no-system-d3d-compiler --no-opengl-sw ${CMAKE_BINARY_DIR}/deploy_windows/StudentManager.exe
            DEPENDS StudentManager
            COMMENT "Deploying Qt libraries for Windows distribution"
        )
        
        message(STATUS "Windows deployment target 'deploy_windows' available")
        message(STATUS "Run 'cmake --build . --target deploy_windows' to create deployment package")
    else()
        message(WARNING "windeployqt not found. Windows deployment target not available.")
    endif()
endif()