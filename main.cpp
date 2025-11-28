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
            if (ue.getState() == RrcState::RRC_CONNECTED) {
                ue.sendRrcConnectionComplete();
                std::cout << "[UE] Sent RRC Complete\n";
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
        int retries = 0;
        bool connected = false;

        // ---- Send initial RRC Request ----
        ue.sendRrcConnectionRequest();
        auto tStart = clock::now();

        while (running && !connected) {

            ue.checkForPackets();

            // Check if DU/CU responded with RRC Setup
            if (ue.getState() == RrcState::RRC_CONNECTED) {
                connected = true;
                break;
            }

            // Timer expired?
            if (Utils.elapsedMs(tStart) > UeRrc::T300_MS) {

                if (retries >= UeRrc::MAX_RRC_RETRIES) {
                    std::cout << "[UE] T300 expired: MAX RETRIES REACHED — giving up.\n";
                    running = false;   // End simulation (UE fails to connect)
                    return;
                }

                retries++;
                std::cout << "[UE] T300 timeout — retransmitting RRC Connection Request (retry "
                          << retries << ")\n";

                ue.sendRrcConnectionRequest();
                tStart = clock::now();
            }
        }

        if (!running) return;  // stop if attacker crushed UE

        // ---- Send Connection Complete ----
        ue.sendRrcConnectionComplete();

        // (Optional) dummy data
        for (int i = 0; i < 200; i++) {
            ue.sendDummyData();
        }

        // ---- Wait for RRC Release ----
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
