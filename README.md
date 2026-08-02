# SubRabbit
<img width="2281" height="624" alt="subrabbit" src="https://github.com/user-attachments/assets/3ca3a929-8e79-4bcc-80e3-ecd3812bacbf" />

 <p align="center">
  <b>A suite of RF offensive and defensive tools for the ESP32(Atmega32u4 in future)</b>
 </p>


---

## Features
<img width="2160" height="1216" alt="subrabbit" src="https://github.com/user-attachments/assets/b3c39d85-df86-41d6-af86-523378f217de" />

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
<img width="1115" height="576" alt="Screenshot 2026-08-02 230142" src="https://github.com/user-attachments/assets/601a14f0-86f6-44cd-8f24-60de2f62e08d" />


## Planned Improvements
### Edge-Based Recording (In Development)

-  **Flipper compatibility** - Edge timings format
-  **Interrupt-driven capture** - Accurate timing
-  **`.sub` file import/export** - Flipper Zero compatibility

## Getting Started
Download the [latest release](https://github.com/wrait8/SubRabbit/releases/latest) of the firmware.
Check out the project [wiki](https://github.com/wrait8/SubRabbit/wiki) for a full overview of the VoidRecon.
