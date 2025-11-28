cmake_minimum_required(VERSION 3.26)
project(Analyzer)

set(CMAKE_CXX_STANDARD 23)

# -------------------------
# PCAP detection
# -------------------------
if(WIN32)
    set(PCAP_ROOT "C:/Program Files/Npcap" CACHE PATH "Npcap install directory")

    find_path(PCAP_INCLUDE_DIR
        NAMES pcap.h
        PATHS "${PCAP_ROOT}/Include"
    )

    find_library(PCAP_LIBRARY
        NAMES wpcap Packet
        PATHS "${PCAP_ROOT}/Lib/x64"
    )

    if(NOT PCAP_INCLUDE_DIR OR NOT PCAP_LIBRARY)
        message(FATAL_ERROR "Npcap not found! Install from https://npcap.com/")
    endif()

else()  # Linux or macOS
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PCAP REQUIRED libpcap)

    set(PCAP_INCLUDE_DIR ${PCAP_INCLUDE_DIRS})
    set(PCAP_LIBRARY ${PCAP_LIBRARIES})
endif()

# -------------------------
# Sources
# -------------------------
add_executable(Analyzer
    main.cpp
    UeRrc.cpp
    NetworkRrc.cpp
    PcapLogger.cpp
    PDcp.cpp
    Utils.cpp
    Attacker.cpp
    CentralUnit.cpp
    DistributedUnit.cpp
)

# -------------------------
# Include directories
# -------------------------
target_include_directories(Analyzer PRIVATE
    include
    ${PCAP_INCLUDE_DIR}
)

# -------------------------
# Link Libraries
# -------------------------
target_link_libraries(Analyzer PRIVATE
    ${PCAP_LIBRARY}
)
