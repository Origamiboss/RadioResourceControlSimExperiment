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

int main() {
    std::cout << "=== Functional Splitting RRC Simulator ===\n";

    using clock = std::chrono::high_resolution_clock;
    
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
    
    auto startex2 = clock::now();
    ExploitDoSSimulationType(2);
    auto endex2 = clock::now();
    auto durationex2 = std::chrono::duration_cast<std::chrono::milliseconds>(endex2 - startex2).count();

    auto startex7 = clock::now();
    ExploitDoSSimulationType(7);
    auto endex7 = clock::now();
    auto durationex7 = std::chrono::duration_cast<std::chrono::milliseconds>(endex7 - startex7).count();

    auto startFuzz = clock::now();
    FuzzingExploitSimulation(7);  // or 2 / 6 / etc
    auto endFuzz = clock::now();

    std::cout << "=== All simulations complete ===\n";
    std::cout << "Option 2 Time: " << duration2 << " ms\n";
    std::cout << "Option 6 Time: " << duration6 << " ms\n";
    std::cout << "Option 7 Time: " << duration7 << " ms\n";

    std::cout << "Exploit Option 2 Time: " << durationex2 << " ms\n";
    std::cout << "Exploit Option 2 Time: " << durationex7 << " ms\n";

    std::cout << "Fuzzing Time: " 
     << std::chrono::duration_cast<std::chrono::milliseconds>(endFuzz - startFuzz).count()
     << " ms\n";
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
        attacker.DoSAttackStep();
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
            if (elapsedMs(tStart) > UeRrc::T300_MS) {

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

void FuzzingExploitSimulation(int optionType){
    using clock = std::chrono::steady_clock;

    PacketBuffer ue_to_du, du_to_ue, du_to_cu, cu_to_du;

    std::atomic<bool> running = true;

    UeRrc ue(&du_to_ue, &ue_to_du);

    // Instead of small packets, fuzzing uses bigger packets
    Attacker attacker(&ue_to_du, 32, &running);  // 32 bytes fuzzed payload

    DistributedUnit du(&ue_to_du, &cu_to_du, &du_to_cu, &du_to_ue, optionType);
    CentralUnit cu(&cu_to_du, &du_to_cu, optionType);

    std::cout << "Starting FUZZING Exploit Option Type " << optionType << "...\n";

    // ---- Start FUZZING attacker thread ----
    std::thread attackerThread([&]() {
        attacker.FuzzAttackStep();
    });

    // ---- UE Thread (same as regular DoS simulation) ----
    std::thread ueThread([&]() {
        ue.sendRrcConnectionRequest();
        auto tStart = clock::now();
        int retries = 0;
        bool connected = false;

        while (running && !connected) {
            ue.checkForPackets();

            if (ue.getState() == RrcState::RRC_CONNECTED) {
                connected = true;
                break;
            }

            if (elapsedMs(tStart) > UeRrc::T300_MS) {
                if (retries >= UeRrc::MAX_RRC_RETRIES) {
                    std::cout << "[UE] FUZZING prevented RRC setup — giving up.\n";
                    running = false;
                    break;
                }

                retries++;
                std::cout << "[UE] T300 timeout (fuzzing overload) — retry " << retries << "\n";
                ue.sendRrcConnectionRequest();
                tStart = clock::now();
            }
        }

        if (!connected) {
            running = false;
            return;
        }

        ue.sendRrcConnectionComplete();

        for (int i = 0; i < 200; i++)
            ue.sendDummyData();

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