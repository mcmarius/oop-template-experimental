cmake_minimum_required(VERSION 3.26)

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing executable name in this file
# for now, the project name is used as the executable name
set(MAIN_PROJECT_NAME "oop")
set(MAIN_EXECUTABLE_NAME "${MAIN_PROJECT_NAME}")

# Video player executable
set(VIDEO_PLAYER_EXECUTABLE_NAME "video_player")

# Overlay executable
set(OVERLAY_EXECUTABLE_NAME "overlay")


project(${MAIN_PROJECT_NAME})
# set(CMAKE_PROJECT_VERSION_MAJOR 0)
# set(CMAKE_PROJECT_VERSION_MINOR 0)
# set(CMAKE_PROJECT_VERSION_PATCH 1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Options.cmake)
include(cmake/CompilerFlags.cmake)
include(cmake/CopyHelper.cmake)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

###############################################################################

# external dependencies with FetchContent
include(FetchContent)

set(FETCHCONTENT_QUIET OFF CACHE INTERNAL "Override verbose output" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE INTERNAL "Override update strategy" FORCE)

set(SFML_WARNINGS_AS_ERRORS OFF)

# NOTE: Also update SFML_VERSION and other env vars used for caching in
#     - .github/actions/configure-cmake/action.yml
#     - .github/workflows/cmake.yml
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    # renovate: datasource=github-tags depName=SFML/SFML versioning=loose
    GIT_TAG        3.0.2
    # renovate: depName=SFML/SFML
    # GIT_TAG        0fa201c969e48ecc253581c5841ce73f44d42f49 # 3.0.2
)

set(VLC_VERSION "3.0.22") # Check VideoLAN site for latest 3.x

FetchContent_Declare(
        vlc_sdk
        URL "https://get.videolan.org/vlc/${VLC_VERSION}/win64/vlc-${VLC_VERSION}-win64.7z"
)

FetchContent_Declare(
    libvlcpp
    GIT_REPOSITORY https://github.com/videolan/libvlcpp.git
    GIT_TAG        99541a172deb58278646560f94aeea0706e0294a # master @ 2026-03-05
    GIT_SHALLOW    1  # works only with branches or tags, not with arbitrary commit hashes
    GIT_PROGRESS   ON
)

FetchContent_MakeAvailable(SFML vlc_sdk libvlcpp)

set(LIBVLC_INCLUDE_DIR "${vlc_sdk_SOURCE_DIR}/sdk/include")

###############################################################################

# external dependencies with find_package

find_package(Threads REQUIRED)
if(APPLE)
elseif(UNIX)
    find_package(X11)
endif()

#find_package(LibVLC REQUIRED)

###############################################################################

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing name here
# Main executable (for compatibility with existing code)
add_executable(${MAIN_EXECUTABLE_NAME}
    src/LauncherMain.cpp
)
target_sources(${MAIN_EXECUTABLE_NAME}
    PRIVATE
        $<$<PLATFORM_ID:Darwin>:src/LauncherMain_macOS.cpp>
        $<$<PLATFORM_ID:Windows>:src/LauncherMain_Windows.cpp>
        $<$<PLATFORM_ID:Linux>:src/LauncherMain_Linux.cpp>
)
#target_sources(${MAIN_EXECUTABLE_NAME}
#    PRIVATE
#        src/VideoWindow.cpp
#        src/TransparentWindow.cpp
#        src/SynchronizedWindows.cpp
#        src/Example.cpp
#)

# Video player executable - separate process for video streaming
add_executable(${VIDEO_PLAYER_EXECUTABLE_NAME}
    src/VideoPlayerMain.cpp
)
target_sources(${VIDEO_PLAYER_EXECUTABLE_NAME}
    PRIVATE
        src/IPCController.cpp
        src/VideoWindow.cpp
        src/VideoWindowPlayback.cpp
        src/VideoWindowSources.cpp
        $<$<PLATFORM_ID:Darwin>:src/VideoWindow_macOS.cpp>
        $<$<PLATFORM_ID:Windows>:src/VideoWindow_Windows.cpp>
        $<$<PLATFORM_ID:Linux>:src/VideoWindow_Linux.cpp>
        src/Example.cpp
)

# Overlay executable - separate process for transparent overlay
add_executable(${OVERLAY_EXECUTABLE_NAME}
    src/OverlayMain.cpp
)
target_sources(${OVERLAY_EXECUTABLE_NAME}
    PRIVATE
        src/IPCController.cpp
        src/TransparentWindow.cpp
        $<$<PLATFORM_ID:Darwin>:src/TransparentWindow_macOS.cpp>
        $<$<PLATFORM_ID:Windows>:src/TransparentWindow_Windows.cpp>
        $<$<PLATFORM_ID:Linux>:src/TransparentWindow_Linux.cpp>
        src/TransparentWindowEvents.cpp
        src/TransparentWindowRendering.cpp
        src/TransparentWindowSources.cpp
        src/Example.cpp
)

# Launcher executable - starts video_player and overlay as separate processes
add_dependencies(${MAIN_EXECUTABLE_NAME} ${VIDEO_PLAYER_EXECUTABLE_NAME} ${OVERLAY_EXECUTABLE_NAME})

target_include_directories(${MAIN_EXECUTABLE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_include_directories(${MAIN_EXECUTABLE_NAME} SYSTEM PRIVATE
    ${libvlcpp_SOURCE_DIR}
)
target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE
    SFML::Graphics SFML::Window SFML::System
    Threads::Threads
)

# NOTE: Add all defined targets (e.g. executables, libraries, etc. )
# NOTE: RUN_SANITIZERS is optional, if it's not present it will default to true
set_compiler_flags(RUN_SANITIZERS ${USE_ASAN} TARGET_NAMES ${MAIN_EXECUTABLE_NAME} ${VIDEO_PLAYER_EXECUTABLE_NAME} ${OVERLAY_EXECUTABLE_NAME})
# set_compiler_flags(TARGET_NAMES ${MAIN_EXECUTABLE_NAME} ${FOO} ${BAR})
# where ${FOO} and ${BAR} represent additional executables or libraries
# you want to compile with the set compiler flags

###############################################################################

# use SYSTEM so cppcheck and clang-tidy do not report warnings from these directories
target_include_directories(${MAIN_EXECUTABLE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_include_directories(${MAIN_EXECUTABLE_NAME} SYSTEM PRIVATE
    ext
    ${libvlcpp_SOURCE_DIR}
)

target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE
    SFML::Graphics SFML::Window SFML::System
    ${LIBVLC_LIBRARY} #${LIBVLCCORE_LIBRARY}
    Threads::Threads
)

target_include_directories(${VIDEO_PLAYER_EXECUTABLE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_include_directories(${VIDEO_PLAYER_EXECUTABLE_NAME} SYSTEM PRIVATE
    ext
    ${libvlcpp_SOURCE_DIR}
)

target_link_libraries(${VIDEO_PLAYER_EXECUTABLE_NAME} PRIVATE
    SFML::Graphics SFML::Window SFML::System
    ${LIBVLC_LIBRARY}
    Threads::Threads
)

target_include_directories(${OVERLAY_EXECUTABLE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_include_directories(${OVERLAY_EXECUTABLE_NAME} SYSTEM PRIVATE
    ext
    ${libvlcpp_SOURCE_DIR}
)

target_link_libraries(${OVERLAY_EXECUTABLE_NAME} PRIVATE
    SFML::Graphics SFML::Window SFML::System
    Threads::Threads
)

if(APPLE)
elseif(UNIX)
    target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE X11)
    target_link_libraries(${VIDEO_PLAYER_EXECUTABLE_NAME} PRIVATE X11)
    target_link_libraries(${OVERLAY_EXECUTABLE_NAME} PRIVATE X11)
endif()

###############################################################################

# copy binaries to "bin" folder; these are uploaded as artifacts on each release
# DESTINATION_DIR is set as "bin" in cmake/Options.cmake:6
install(TARGETS ${MAIN_EXECUTABLE_NAME} ${VIDEO_PLAYER_EXECUTABLE_NAME} ${OVERLAY_EXECUTABLE_NAME} DESTINATION ${DESTINATION_DIR})
if(APPLE)
    install(FILES launcher.command DESTINATION ${DESTINATION_DIR})
endif()

# Copy all assets needed by the application
copy_files(FILES tastatura.txt DIRECTORY assets COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})
