cmake_minimum_required(VERSION 3.26)

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing executable name in this file
# for now, the project name is used as the executable name
set(MAIN_PROJECT_NAME "oop")
set(MAIN_EXECUTABLE_NAME "${MAIN_PROJECT_NAME}")


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

###############################################################################

# external dependencies with FetchContent
include(FetchContent)

set(FETCHCONTENT_QUIET OFF CACHE INTERNAL "Override verbose output" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE INTERNAL "Override update strategy" FORCE)

# NOTE: Also update env vars used for caching in
#     - .github/actions/configure-cmake/action.yml
#     - .github/workflows/cmake.yml
if(UNIX)
    #if(APPLE)
    #    find_package(PcapPlusPlus)
    #else()
    FetchContent_Declare(
        PcapPlusPlus
        GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
        GIT_TAG        v25.05 # <which tag/branch @ <some_date>>
        GIT_SHALLOW    1  # works only with branches or tags, not with arbitrary commit hashes
        GIT_PROGRESS   ON
    )
    # Optimize PcapPlusPlus compilation by disabling tools/examples you don't need
    set(PCAPPP_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(PCAPPP_BUILD_TUTORIALS OFF CACHE INTERNAL "")

    FetchContent_MakeAvailable(PcapPlusPlus)
    #endif()
else()
    # Windows: use pre-built PcapPlusPlus binaries
    if(MINGW)
        set(PCPP_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v25.05/pcapplusplus-25.05-windows-mingw64-x86_64-gcc-15.1.0.zip")
    elseif(MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(PCPP_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v25.05/pcapplusplus-25.05-windows-vs2022-x64-debug.zip")
        else()
            set(PCPP_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v25.05/pcapplusplus-25.05-windows-vs2022-x64-release.zip")
        endif()
    endif()

    # Download and extract pre-built binaries
    FetchContent_Declare(
        pcapplusplus
        DOWNLOAD_ONLY TRUE
        URL "${PCPP_URL}"
    )
    FetchContent_MakeAvailable(pcapplusplus)

    # Set paths to extracted binaries
    set(PCPP_ROOT "${pcapplusplus_SOURCE_DIR}")
    # The archive extracts to a subdirectory named after the archive
    file(GLOB PCPP_SUBDIRS "${pcapplusplus_SOURCE_DIR}/pcapplusplus-*")
    if(PCPP_SUBDIRS)
        list(GET PCPP_SUBDIRS 0 PCPP_SUBDIR)
    else()
        set(PCPP_SUBDIR "${pcapplusplus_SOURCE_DIR}")
    endif()

    # Determine library file extensions
    if(MSVC)
        set(PCPP_LIB_EXT "lib")
        set(PCPP_LIB_PREFIX "")
    else()
        set(PCPP_LIB_EXT "a")
        set(PCPP_LIB_PREFIX "lib")
    endif()

    find_package(Threads REQUIRED)

    # Manually create PcapPlusPlus imported targets (avoid pre-built cmake config which
    # has paths CMake can't verify at configure time)
    add_library(PcapPlusPlus::Common++ STATIC IMPORTED)
    set_target_properties(PcapPlusPlus::Common++ PROPERTIES
        IMPORTED_LOCATION "${PCPP_SUBDIR}/lib/${PCPP_LIB_PREFIX}Common++.${PCPP_LIB_EXT}"
        INTERFACE_INCLUDE_DIRECTORIES "${PCPP_SUBDIR}/include/pcapplusplus;${PCPP_SUBDIR}/include"
        INTERFACE_LINK_LIBRARIES "ws2_32;iphlpapi"
    )

    add_library(PcapPlusPlus::Packet++ STATIC IMPORTED)
    set_target_properties(PcapPlusPlus::Packet++ PROPERTIES
        IMPORTED_LOCATION "${PCPP_SUBDIR}/lib/${PCPP_LIB_PREFIX}Packet++.${PCPP_LIB_EXT}"
        INTERFACE_INCLUDE_DIRECTORIES "${PCPP_SUBDIR}/include/pcapplusplus;${PCPP_SUBDIR}/include"
        INTERFACE_LINK_LIBRARIES "PcapPlusPlus::Common++"
    )

    # PcapPlusPlus::Pcap++ is the main target; it depends on Common++, Packet++, and
    # the underlying PCAP::PCAP / Packet::Packet targets (from npcap SDK or mock)
    add_library(PcapPlusPlus::Pcap++ STATIC IMPORTED)
    set_target_properties(PcapPlusPlus::Pcap++ PROPERTIES
        IMPORTED_LOCATION "${PCPP_SUBDIR}/lib/${PCPP_LIB_PREFIX}Pcap++.${PCPP_LIB_EXT}"
        INTERFACE_INCLUDE_DIRECTORIES "${PCPP_SUBDIR}/include/pcapplusplus;${PCPP_SUBDIR}/include"
        INTERFACE_LINK_LIBRARIES "ws2_32;iphlpapi;Packet::Packet;PcapPlusPlus::Common++;PcapPlusPlus::Packet++;PCAP::PCAP;Threads::Threads"
    )

    # --- Provide PCAP::PCAP and Packet::Packet targets ---
    # These come from the npcap SDK (not redistributable). For CI, use mock libs.
    if(NOT TARGET PCAP::PCAP)
        if(DEFINED ENV{BUILDING_FOR_CI} OR NPCAP_SDK_PATH STREQUAL "")
            # CI build: use mock libraries
            add_subdirectory(${CMAKE_SOURCE_DIR}/cmake/mock_npcap)
            add_library(PCAP::PCAP IMPORTED INTERFACE)
            set_target_properties(PCAP::PCAP PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/cmake/mock_npcap/include"
                INTERFACE_LINK_LIBRARIES "mock_libpcap"
            )
        else()
            # Release build: use real npcap SDK
            set(NPCAP_SDK "$ENV{NPCAP_SDK_PATH}")
            find_path(PCAP_INCLUDE_DIR NAMES pcap/pcap.h PATHS "${NPCAP_SDK}/Include" NO_DEFAULT_PATH)
            find_library(PCAP_LIBRARY NAMES wpcap pcap PATHS "${NPCAP_SDK}/Lib/x64" NO_DEFAULT_PATH)
            if(PCAP_INCLUDE_DIR AND PCAP_LIBRARY)
                add_library(PCAP::PCAP IMPORTED SHARED)
                set_target_properties(PCAP::PCAP PROPERTIES
                    IMPORTED_LOCATION "${PCAP_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${PCAP_INCLUDE_DIR}"
                )
            else()
                message(FATAL_ERROR "npcap SDK not found. Set NPCAP_SDK_PATH or set BUILDING_FOR_CI=1 for mock builds.")
            endif()
        endif()
    endif()

    if(NOT TARGET Packet::Packet)
        if(DEFINED ENV{BUILDING_FOR_CI} OR NPCAP_SDK_PATH STREQUAL "")
            # CI build: use mock libraries
            add_library(Packet::Packet IMPORTED INTERFACE)
            set_target_properties(Packet::Packet PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/cmake/mock_npcap/include"
                INTERFACE_LINK_LIBRARIES "mock_packet"
            )
        else()
            # Release build: use real npcap SDK
            set(NPCAP_SDK "$ENV{NPCAP_SDK_PATH}")
            find_path(PACKET_INCLUDE_DIR NAMES Packet32.h PATHS "${NPCAP_SDK}/Include" NO_DEFAULT_PATH)
            find_library(PACKET_LIBRARY NAMES Packet PATHS "${NPCAP_SDK}/Lib/x64" NO_DEFAULT_PATH)
            if(PACKET_INCLUDE_DIR AND PACKET_LIBRARY)
                add_library(Packet::Packet IMPORTED SHARED)
                set_target_properties(Packet::Packet PROPERTIES
                    IMPORTED_LOCATION "${PACKET_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${PACKET_INCLUDE_DIR}"
                )
            else()
                message(FATAL_ERROR "npcap SDK Packet library not found. Set NPCAP_SDK_PATH or set BUILDING_FOR_CI=1 for mock builds.")
            endif()
        endif()
    endif()

    # Also provide mock_light_pcapng for CI (Pcap++ depends on light_pcapng symbols)
    if(NOT TARGET mock_light_pcapng AND (DEFINED ENV{BUILDING_FOR_CI} OR NPCAP_SDK_PATH STREQUAL ""))
        add_subdirectory(${CMAKE_SOURCE_DIR}/cmake/mock_npcap)
    endif()

endif()

###############################################################################

# external dependencies with find_package

# find_package(Threads REQUIRED)

###############################################################################

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing name here
add_executable(${MAIN_EXECUTABLE_NAME}
    main.cpp
)
target_sources(${MAIN_EXECUTABLE_NAME}
    PRIVATE
        src/Example.cpp
)

# NOTE: Add all defined targets (e.g. executables, libraries, etc. )
# NOTE: RUN_SANITIZERS is optional, if it's not present it will default to true
set_compiler_flags(RUN_SANITIZERS ${USE_ASAN} TARGET_NAMES ${MAIN_EXECUTABLE_NAME})
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
)
# target_include_directories(${MAIN_EXECUTABLE_NAME} SYSTEM PRIVATE ${<SomeLib>_SOURCE_DIR}/include)
# target_link_directories(${MAIN_EXECUTABLE_NAME} PRIVATE ${<SomeLib>_BINARY_DIR}/lib)

if(WIN32)
    # PcapPlusPlus::Pcap++ already depends on PCAP::PCAP, Packet::Packet, ws2_32, iphlpapi, pthread
    target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE
        PcapPlusPlus::Pcap++
    )
else()
    target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE
        Pcap++ Common++
    )
endif()

# Apply platform-specific linkage
if(WIN32)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PCAP_LIBRARY})
endif()

###############################################################################

# copy binaries to "bin" folder; these are uploaded as artifacts on each release
# DESTINATION_DIR is set as "bin" in cmake/Options.cmake:6
install(TARGETS ${MAIN_EXECUTABLE_NAME} DESTINATION ${DESTINATION_DIR})
if(APPLE)
    install(FILES launcher.command DESTINATION ${DESTINATION_DIR})
endif()

copy_files(FILES tastatura.txt DIRECTORY assets COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})
# copy_files(FILES tastatura.txt config.json DIRECTORY images sounds COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})
# copy_files(DIRECTORY images sounds COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})
