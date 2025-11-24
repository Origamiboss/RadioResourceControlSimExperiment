cmake_minimum_required(VERSION 3.26)
project(Analyzer)

set(CMAKE_CXX_STANDARD 23)

if(WIN32)
    set(PCAP_ROOT "C:/Program Files/Npcap" CACHE PATH "Npcap install directory")
    find_path(PCAP_INCLUDE_DIR pcap.h PATHS "${PCAP_ROOT}/Include")
    find_library(PCAP_LIBRARY NAMES wpcap PATHS "${PCAP_ROOT}/Lib/x64")
else()
    find_package(PCAP REQUIRED)  # Linux/Mac
    pkg_check_modules(PCAP REQUIRED libpcap)

    set(PCAP_INCLUDE_DIR ${PCAP_INCLUDE_DIRS})
    set(PCAP_LIBRARY ${PCAP_LIBRARIES})
endif()

add_executable(Analyzer
        main.cpp
        UeRrc.hpp UeRrc.cpp
        NetworkRrc.hpp NetworkRrc.cpp
        PcapLogger.hpp
        PcapLogger.cpp
        src/PDcp.hpp src/PDcp.cpp
)


target_include_directories(Analyzer PRIVATE ${PCAP_INCLUDE_DIR})
target_link_libraries(Analyzer PRIVATE ${PCAP_LIBRARY})

