cmake_minimum_required(VERSION 3.10)
project(MintCheat VERSION 1.0)

# Create a Shared Library (DLL)
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/Aimbot.cpp
    src/JNIHelper.cpp
    src/OverlayWindow.cpp
    src/Aimbot.h
    src/JNIHelper.h
    src/Mappings_1_21_4.h
    src/OverlayWindow.h
)

# Set C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Define Java JDK path (We found this specific path on the user's system)
set(JAVA_INC "C:/Program Files/Eclipse Adoptium/jdk-21.0.3.9-hotspot/include")
set(JAVA_INC_WIN32 "C:/Program Files/Eclipse Adoptium/jdk-21.0.3.9-hotspot/include/win32")

# Define WebView2 paths
set(WEBVIEW2_INC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(WEBVIEW2_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/WebView2LoaderStatic.lib")

# Add include directories for JNI and WebView2
target_include_directories(${PROJECT_NAME} PRIVATE
    ${JAVA_INC}
    ${JAVA_INC_WIN32}
    ${WEBVIEW2_INC}
)

# Define Java JVM lib for linking JNI functions
set(JAVA_LIB "C:/Program Files/Eclipse Adoptium/jdk-21.0.3.9-hotspot/lib/jvm.lib")

# Link WebView2 static library, JVM lib, and standard Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${WEBVIEW2_LIB}
    ${JAVA_LIB}
    kernel32
    user32
    gdi32
    ole32
    oleaut32
)
