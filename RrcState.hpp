#ifndef RRCSTATE_HPP
#define RRCSTATE_HPP

enum class RrcState {
    // UE has no connection
    RRC_IDLE,

    // UE has sent RRCConnectionRequest, waiting for Setup
    RRC_REQUEST_SENT,

    // UE received RRCConnectionSetup from CU
    RRC_SETUP_RECEIVED,

    // UE has sent RRCConnectionComplete, waiting for Release or SRB establishment
    RRC_CONNECTED,

    // CU has sent RRCRelease
    RRC_RELEASE_RECEIVED,

    // Final state after release processing
    RRC_RELEASED
};

#endif 
