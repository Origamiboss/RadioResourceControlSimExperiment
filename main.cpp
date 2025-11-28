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
#include "Attacker.hpp"
#include <chrono>

// #include <pcap.h>
void SimulationType(int optionType);
void ExploitSimulationType(int optionType);

int main() {
    std::cout << "=== Functional Splitting RRC Simulator ===\n";

    using clock = std::chrono::high_resolution_clock;
    /*
    // --- Option 2 ---
    auto start2 = clock::now();
    SimulationType(2);
    auto end2 = clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    std::cout << "Option 2 completed in: " << duration2 << " ms\n\n";

    // --- Option 6 ---
    auto start6 = clock::now();
    SimulationType(6);
    auto end6 = clock::now();
    auto duration6 = std::chrono::duration_cast<std::chrono::milliseconds>(end6 - start6).count();
    std::cout << "Option 6 completed in: " << duration6 << " ms\n\n";

    // --- Option 7 ---
    auto start7 = clock::now();
    SimulationType(7);
    auto end7 = clock::now();
    auto duration7 = std::chrono::duration_cast<std::chrono::milliseconds>(end7 - start7).count();
    std::cout << "Option 7 completed in: " << duration7 << " ms\n\n";
    */
    auto startex = clock::now();
    ExploitSimulationType(2);
    auto endex = clock::now();
    auto durationex = std::chrono::duration_cast<std::chrono::milliseconds>(endex - startex).count();

    std::cout << "=== All simulations complete ===\n";
    //std::cout << "Option 2 Time: " << duration2 << " ms\n";
    //std::cout << "Option 6 Time: " << duration6 << " ms\n";
    //std::cout << "Option 7 Time: " << duration7 << " ms\n";
    std::cout << "Exploit Option 2 Time: " << durationex << " ms\n";

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
        for(int i = 0; i < 200; i++){
            ue.sendDummyData();
        }
        ue.checkForPackets();

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
        }
    });

    // ---------- CU Thread ----------
    std::thread cuThread([&]() {

        while (running) {

            cu.checkForPackets();

        }
    });
    ueThread.join();
    duThread.join();
    cuThread.join();

    // UE processes setup

    std::cout << "\n Option Type "<< optionType << " Simulation complete. Check Logs/ for details.\n";
}
void ExploitSimulationType(int optionType){
    PacketBuffer ue_to_du, du_to_ue, du_to_cu, cu_to_du;

    std::atomic<bool> running = true;

    UeRrc ue(&du_to_ue, &ue_to_du);
    Attacker attacker(&ue_to_du, 2, &running);

    DistributedUnit du(&ue_to_du, &cu_to_du, &du_to_cu, &du_to_ue, optionType);
    CentralUnit cu(&cu_to_du, &du_to_cu, optionType);

    std::cout << "Starting Exploit Option Type " << optionType << "...\n";

    std::thread attackerThread([&]() {
        attacker.DoSAttack();
    });

    std::thread ueThread([&]() {
        ue.sendRrcConnectionRequest();

        bool sentComplete = false;
        while (!sentComplete && running) {
            ue.checkForPackets();
            if (ue.getState() == RrcState::RRC_CONNECTED) {
                ue.sendRrcConnectionComplete();
                sentComplete = true;
            }
        }

        for(int i = 0; i < 200; i++){
            ue.sendDummyData();
        }

        while (running) {
            ue.checkForPackets();
            if (ue.getState() == RrcState::RRC_IDLE) {
                running = false;
                break;
            }
        }
    });

    std::thread duThread([&]() {
        while (running) {
            du.checkForPackets();
        }
    });

    std::thread cuThread([&]() {
        while (running) {
            cu.checkForPackets();
        }
    });

    ueThread.join();
    running = false;           // <-- TELL ATTACKER TO STOP
    duThread.join();
    cuThread.join();

    if (attackerThread.joinable()) attackerThread.join();  // <-- CRUCIAL

    std::cout << "\nExploit Option Type " << optionType << " complete.\n";
}
