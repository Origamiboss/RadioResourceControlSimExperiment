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
#include "Utils.hpp"
#include "Attacker.hpp"
#include <chrono>

// #include <pcap.h>
void SimulationType(int optionType);
void ExploitDoSSimulationType(int optionType);
void FuzzingExploitSimulation(int optionType);

int main() {
    std::cout << "=== Functional Splitting RRC Simulator ===\n";

    bool running = true;

    while (running) {
        std::cout << "\n===== MAIN MENU =====\n";
        std::cout << "1. Run Normal Simulation (Option 2)\n";
        std::cout << "2. Run Normal Simulation (Option 6)\n";
        std::cout << "3. Run Normal Simulation (Option 7)\n";
        std::cout << "4. Run DoS Exploit Simulation\n";
        std::cout << "5. Run Fuzzing Exploit Simulation\n";
        std::cout << "0. Exit Program\n";
        std::cout << "Select an option: ";

        int choice;
        std::cin >> choice;

        using clock = std::chrono::high_resolution_clock;

        switch (choice) {
            case 1: {
                auto s = clock::now();
                SimulationType(2);
                auto e = clock::now();
                std::cout << "[TIME] Completed in "
                          << std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count()
                          << " ms\n";
                break;
            }

            case 2: {
                auto s = clock::now();
                SimulationType(6);
                auto e = clock::now();
                std::cout << "[TIME] Completed in "
                          << std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count()
                          << " ms\n";
                break;
            }

            case 3: {
                auto s = clock::now();
                SimulationType(7);
                auto e = clock::now();
                std::cout << "[TIME] Completed in "
                          << std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count()
                          << " ms\n";
                break;
            }

            case 4: {
                std::cout << "Select RRC Option (2, 6, or 7): ";
                int opt;
                std::cin >> opt;

                auto s = clock::now();
                ExploitDoSSimulationType(opt);
                auto e = clock::now();

                std::cout << "[TIME] DoS Simulation completed in "
                          << std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count()
                          << " ms\n";
                break;
            }

            case 5: {
                std::cout << "Select RRC Option (2, 6, or 7): ";
                int opt;
                std::cin >> opt;

                auto s = clock::now();
                FuzzingExploitSimulation(opt);
                auto e = clock::now();

                std::cout << "[TIME] Fuzzing Simulation completed in "
                          << std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count()
                          << " ms\n";
                break;
            }

            case 0:
                running = false;
                std::cout << "Exiting simulator.\n";
                break;

            default:
                std::cout << "Invalid choice. Try again.\n";
        }
    }

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
        

        // (Optional) dummy data
        int dummyPackets = 200;
        // send dummy packets once
        for (int i = 0; i < dummyPackets; i++) {
            ue.sendDummyData();
            ue.checkForPackets();
        }

        // wait until all are received
        while (ue.retrievedDummyPackets() < dummyPackets) {
            ue.checkForPackets();
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield CPU
            ue.sendDummyData();
        }
        


        // ---- Send Connection Complete ----
        ue.sendRrcConnectionComplete();

        while (running) {
            ue.checkForPackets();
            if (ue.getState() == RrcState::RRC_IDLE) {
                running = false;
                break;
            }
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


    // *************** NEW: PACKET DROP REPORT ***************
    std::cout << "\n=== Packet Drop Summary (Option " << optionType << ") ===\n";
    std::cout << "UE -> DU dropped: " << ue_to_du.getDroppedCount() << "\n";
    std::cout << "DU -> UE dropped: " << du_to_ue.getDroppedCount() << "\n";
    std::cout << "DU -> CU dropped: " << du_to_cu.getDroppedCount() << "\n";
    std::cout << "CU -> DU dropped: " << cu_to_du.getDroppedCount() << "\n";
    // ********************************************************

    std::cout << "\nOption Type "<< optionType << " Simulation complete.\n";
}
void ExploitDoSSimulationType(int optionType){
    using clock = std::chrono::steady_clock;

    PacketBuffer ue_to_du, du_to_ue, du_to_cu, cu_to_du;

    std::atomic<bool> running = true;

    UeRrc ue(&du_to_ue, &ue_to_du);
    Attacker attacker(&ue_to_du, 2, &running);

    DistributedUnit du(&ue_to_du, &cu_to_du, &du_to_cu, &du_to_ue, optionType);
    CentralUnit cu(&cu_to_du, &du_to_cu, optionType);

    std::cout << "Starting Exploit Option Type " << optionType << "...\n";

    std::thread attackerThread([&]() {
        attacker.attackTargetBuffer();
    });

    // ---------- UE Thread ----------
    std::thread ueThread([&]() {

        ue.sendRrcConnectionRequest();
        std::cout << "[UE] Sent RRC Request\n";
        ue.checkForPackets();
        bool sentComplete = false;
        

        // (Optional) dummy data
        int dummyPackets = 200;
        // send dummy packets once
        for (int i = 0; i < dummyPackets; i++) {
            ue.sendDummyData();
        }

        // wait until all are received
        while (ue.retrievedDummyPackets() < dummyPackets) {
            ue.checkForPackets();
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield CPU
        }
        


        // ---- Send Connection Complete ----
        ue.sendRrcConnectionComplete();

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
    running = false;
    duThread.join();
    cuThread.join();
    if (attackerThread.joinable()) attackerThread.join();


    // *************** NEW: PACKET DROP REPORT ***************
    std::cout << "\n=== PACKET DROP SUMMARY (EXPLOIT Option " << optionType << ") ===\n";
    std::cout << "UE -> DU dropped: " << ue_to_du.getDroppedCount() << "\n";
    std::cout << "DU -> UE dropped: " << du_to_ue.getDroppedCount() << "\n";
    std::cout << "DU -> CU dropped: " << du_to_cu.getDroppedCount() << "\n";
    std::cout << "CU -> DU dropped: " << cu_to_du.getDroppedCount() << "\n";
    // ********************************************************

    std::cout << "\nExploit Option Type " << optionType << " complete.\n";
}

void FuzzingExploitSimulation(int optionType){
    using clock = std::chrono::steady_clock;

    PacketBuffer ue_to_du, du_to_ue, du_to_cu, cu_to_du;

    std::atomic<bool> running = true;

    UeRrc ue(&du_to_ue, &ue_to_du);

    // Instead of small packets, fuzzing uses bigger packets
    Attacker attacker(&ue_to_du, 2, &running, Attacker::AttackMode::FUZZ);  // 2 bytes fuzzed payload

    DistributedUnit du(&ue_to_du, &cu_to_du, &du_to_cu, &du_to_ue, optionType);
    CentralUnit cu(&cu_to_du, &du_to_cu, optionType);

    std::cout << "Starting FUZZING Exploit Option Type " << optionType << "...\n";

    // ---- Start FUZZING attacker thread ----
    std::thread attackerThread([&]() {
        attacker.attackTargetBuffer();
        
    });

    // ---- UE Thread (same as regular DoS simulation) ----
    // ---------- UE Thread ----------
    std::thread ueThread([&]() {

        ue.sendRrcConnectionRequest();
        std::cout << "[UE] Sent RRC Request\n";
        ue.checkForPackets();
        bool sentComplete = false;
        

        //Wait for the fuzzing attack to suceed

        // wait until all are received
        while (ue.getState() != RrcState::RRC_IDLE) {
            ue.checkForPackets();
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield CPU
        }
        


        // ---- Send Connection Complete ----
        ue.sendRrcConnectionComplete();

        while (running) {
            ue.checkForPackets();
            if (ue.getState() == RrcState::RRC_IDLE) {
                running = false;
                break;
            }
        }

    });

    // DU Thread
    std::thread duThread([&]() {
        while (running) {
            du.checkForPackets();
        }
    });

    // CU Thread
    std::thread cuThread([&]() {
        while (running) {
            cu.checkForPackets();
        }
    });

    // ---- Join Threads ----
    ueThread.join();
    running = false;      // stop fuzzing attacker
    duThread.join();
    cuThread.join();
    if (attackerThread.joinable()) attackerThread.join();

    // ---- Drop Summary ----
    std::cout << "\n=== PACKET DROP SUMMARY (FUZZING Option " << optionType << ") ===\n";
    std::cout << "UE -> DU dropped: " << ue_to_du.getDroppedCount() << "\n";
    std::cout << "DU -> UE dropped: " << du_to_ue.getDroppedCount() << "\n";
    std::cout << "DU -> CU dropped: " << du_to_cu.getDroppedCount() << "\n";
    std::cout << "CU -> DU dropped: " << cu_to_du.getDroppedCount() << "\n";

    std::cout << "\nFUZZING Exploit Option Type " << optionType << " complete.\n";
}