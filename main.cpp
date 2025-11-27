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
void SimulationType(int optionType);


int main() {
    std::cout << "=== Functional Splitting RRC Simulator ===\n";
    SimulationType(2);
    SimulationType(6);
    SimulationType(7);


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
    return 0;
}

void SimulationType(int optionType){
    PacketBuffer ue_to_du, du_to_ue, du_to_cu, cu_to_du;

    UeRrc ue(&du_to_ue, &ue_to_du);


    DistributedUnit du(&ue_to_du, &cu_to_du, &du_to_cu, &du_to_ue, optionType);
    CentralUnit cu(&cu_to_du, &du_to_cu, optionType);

    std::atomic<bool> running = true;
    std::cout << "Starting Option Type " << optionType << " Simulation...\n";
    

    // ---------- UE Thread ----------
    std::thread ueThread([&]() {

        ue.sendRrcConnectionRequest();
        std::cout << "[UE] Sent RRC Request\n";
        ue.checkForPackets();
        bool sentComplete = false;
        while(!sentComplete){
            ue.checkForPackets();
            // After receiving the setup message
            if (ue.getState() == RrcState::RRC_CONNECTED) {
                ue.sendRrcConnectionComplete();
                std::cout << "[UE] Sent RRC Complete\n";
                sentComplete = true;
            }

        }
        for(int i = 0; i < 1000; i++){
            ue.sendDummyData();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            ue.checkForPackets();
        }
        

        // After receiving the setup message
        if (ue.getState() == RrcState::RRC_CONNECTED) {
            ue.sendRrcConnectionComplete();
            std::cout << "[UE] Sent RRC Complete\n";
        }
        while (running) {
            ue.checkForPackets();

            // When CU sends RRC Release, UE goes idle
            if (ue.getState() == RrcState::RRC_IDLE) {
                std::cout << "[UE] Release received, ending\n";
                running = false;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // After receiving the release message
        if (ue.getState() == RrcState::RRC_IDLE) {
            std::cout << "[UE] Release received, ending\n";
            running = false;
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

    std::cout << "\n Option Type "<< optionType << " Simulation complete. Check Logs/ for details.\n";
}
