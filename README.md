# VoidRecon
 <img width="2281" height="501" alt="VoidRecon" src="https://github.com/user-attachments/assets/67ae50fb-6ab7-40da-aec2-cf8b88e918f8" />
 <p align="center">
  <b>A suite of RF offensive and defensive tools for the ESP32(Atmega32u4 in future)</b>
 </p>


---

## Features
<img width="1920" height="1080" alt="render-1783769825014" src="https://github.com/user-attachments/assets/6120951a-2162-4bd6-90d4-06b33e4da682" />

- Packet Sniffer: Receive and log RF packets.
- Frequency Analyzer: Scan and detect active channels in Sub-GHz spectrum.
- RSSI Monitoring: Display signal strength and link quality (LQI).
- RAW RF Capture/Replay: Record raw RF samples (`rxraw`, `recraw`) and replay (`playraw`).
- Fixed-Code Remote Capture/Replay: Capture and replay fixed-code signals (`recsig`, `playsig`).
- Packet Recording/Playback: Buffer, list, and replay packets (`rec`, `show`, `play`).
- Multi-Modulation Support: 2-FSK, GFSK, ASK/OOK, 4-FSK, MSK.
- Configurable Radio Parameters: Frequency, channel, data rate, deviation, bandwidth, sync, CRC, encoding, and more.
- Jamming Mode: Continuous transmission on selected band (`jam`).
- Brute-Force Mode: Timing-based signal brute forcing (`brute`).
- RF Chat Mode: Peer-to-peer RF communication (`chat`).


## ScreenShot
<img width="1118" height="571" alt="Screenshot 2026-07-26 053217" src="https://github.com/user-attachments/assets/d0fb3d6c-7fc1-476f-89f5-94b0e31f3746" />

## Planned Improvements
### Edge-Based Recording (In Development)

-  **Flipper compatibility** - Edge timings format
-  **Interrupt-driven capture** - Accurate timing
-  **`.sub` file import/export** - Flipper Zero compatibility

## Getting Started
Download the [latest release](https://github.com/wrait8/VoidRecon/releases/latest) of the firmware.
Check out the project [wiki](https://github.com/wrait8/VoidRecon/wiki) for a full overview of the VoidRecon.
