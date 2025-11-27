struct LatencyModel {
    int fiberOneWayUs;       // e.g., 10 µs per km (fiber)
    int packetizationUs;     // serialization delay based on bandwidth
    int jitterUs;            // random +/- jitter
    double fronthaulMbps;    // bandwidth of fronthaul
};