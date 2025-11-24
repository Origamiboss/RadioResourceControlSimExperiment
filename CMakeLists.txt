cmake_minimum_required(VERSION 3.26)
project(Analyzer)

set(CMAKE_CXX_STANDARD 23)

if(WIN32)
    set(PCAP_ROOT "C:/Program Files/Npcap" CACHE PATH "Npcap install directory")
    find_path(PCAP_INCLUDE_DIR pcap.h PATHS "${PCAP_ROOT}/Include")
    find_library(PCAP_LIBRARY NAMES wpcap PATHS "${PCAP_ROOT}/Lib/x64")
else()
    # Linux/Mac: use pkg-config instead of find_package(PCAP)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PCAP REQUIRED libpcap)

    set(PCAP_INCLUDE_DIR ${PCAP_INCLUDE_DIRS})
    set(PCAP_LIBRARY ${PCAP_LIBRARIES})
endif()

# List only .cpp files here
add_executable(Analyzer
    main.cpp
    UeRrc.cpp
    NetworkRrc.cpp
    PcapLogger.cpp
    PDcp.cpp
    Utils.cpp
)

# Include directories for headers
target_include_directories(Analyzer PRIVATE include ${PCAP_INCLUDE_DIR})

# Link PCAP
target_link_libraries(Analyzer PRIVATE ${PCAP_LIBRARY})

