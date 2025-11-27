/**
 * @file main.cpp
 * @brief Main simulation of LTE RRC protocol exchange
 *
 * Simulates the complete RRC connection lifecycle between UE and Network:
 * 1. RRC Connection Request
 * 2. RRC Connection Setup
 * 3. RRC Connection Complete
 * 4. RRC Release
 */


#include <iostream>
#include <thread>
#include "UeRrc.hpp"
#include "NetworkRrc.hpp"
#include "CentralUnit.hpp"
#include "DistributedUnit.hpp"
#include "PacketBuffer.hpp"
// #include <pcap.h>

int main() {

    PacketBuffer ueBuffer, duBuffer, cuBuffer;

    UeRrc ue(&ueBuffer, &duBuffer);
    DistributedUnit du(&duBuffer, &ueBuffer, &cuBuffer);
    CentralUnit cu(&cuBuffer, &duBuffer);

    std::atomic<bool> running = true;

    std::cout << "=== LTE RRC Simulator ===\n";

    // ---------- UE Thread ----------
    std::thread ueThread([&]() {

        ue.sendRrcConnectionRequest();
        std::cout << "[UE] Sent RRC Request\n";

        bool sentComplete = false;

        while (running) {
            ue.checkForPackets();

            // After receiving the setup message
            if (ue.getState() == RrcState::RRC_CONNECTED && !sentComplete) {
                ue.sendRrcConnectionComplete();
                std::cout << "[UE] Sent RRC Complete\n";
                sentComplete = true;
            }

            // After receiving the release message
            if (ue.getState() == RrcState::RRC_IDLE && sentComplete) {
                std::cout << "[UE] Release received, ending\n";
                running = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });


    // ---------- DU Thread ----------
    std::thread duThread([&]() {
        while (running) {
            du.checkForPackets();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // ---------- CU Thread ----------
    std::thread cuThread([&]() {

        while (running) {

            cu.checkForPackets();

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    ueThread.join();
    duThread.join();
    cuThread.join();

    // UE processes setup
    std::this_thread::sleep_for(std::chrono::seconds(3));

    

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\nSimulation complete. Check Logs/ for details.\n";
    return 0;


    //    char errbuf[PCAP_ERRBUF_SIZE];
//    pcap_if_t* all_devices;
//
//    if (pcap_findalldevs(&all_devices, errbuf) == -1)
//    {
//        std::cerr << "npcap not found!" << errbuf << std::endl;
//    }
//    else
//    {
//        std::cout << "npcap installed successfully" << std::endl;
//        pcap_freealldevs(all_devices);
//    }

}
