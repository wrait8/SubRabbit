# SubRabbit
<div align="center">
<img width="2281" height="624" alt="subrabbit (2)" src="https://github.com/user-attachments/assets/2fc8a925-5033-47c2-9387-f1e82c04a322" />
  <p>
  <b>A suite of RF offensive and defensive tools</b>
 </p>
<!-- Badges -->
<a href="https://github.com/wrait8/SubRabbit" title="Go to GitHub repo"><img src="https://img.shields.io/static/v1?label=wrait8&message=SubRabbit&color=magenta&logo=github" alt="wrait8 - SubRabbit"></a>
<a href="https://github.com/wrait8/SubRabbit"><img src="https://img.shields.io/github/stars/wrait8/SubRabbit?style=social" alt="stars - SubRabbit"></a>
<a href="https://github.com/wrait8/SubRabbit"><img src="https://img.shields.io/github/forks/wrait8/SubRabbit?style=social" alt="forks - SubRabbit"></a>
</div>

## Features
<p align="center"><img alt="SubRabbit_Render" src="https://github.com/user-attachments/assets/2d75b4aa-5771-415c-89cb-edb284a3dce2" ></p>

- Packet Sniffer: Receive RF packets (`rxraw`).
- Frequency Analyzer: Scan and detect active channels in Sub-GHz spectrum (`analyze`).
- RSSI Monitoring: Display signal strength and link quality (LQI).
- RAW RF Capture/Replay: Record raw RF samples (`rxraw`, `recraw`) and replay (`playraw`).
- Fixed-Code Remote Capture/Replay: Capture and replay fixed-code signals (`recsig`, `playsig`).
- Packet Recording/Playback: Buffer, list, and replay packets (`rec`, `show`, `play`).
- Multi-Modulation Support: 2-FSK, GFSK, ASK/OOK, 4-FSK, MSK.
- Configurable Radio Parameters: Frequency, channel, data rate, deviation, bandwidth, sync, CRC, encoding, and more.
- Jamming Mode: Continuous transmission on selected band (`jam`).
- Brute-Force Mode: Timing-based signal brute forcing (`brute`).

## ScreenShot
<img width="1110" height="587" alt="image" src="https://github.com/user-attachments/assets/9c60d6c4-d79c-4bff-97ff-41df03262f81" />

## Planned Improvements
### Edge-Based Recording (In Development)

-  **Flipper compatibility** - Edge timings format
-  **Interrupt-driven capture** - Accurate timing
-  **`.sub` file import/export** - Flipper Zero compatibility

## Getting Started
Download the [latest release](https://github.com/wrait8/SubRabbit/releases/latest) of the firmware.
Check out the project [wiki](https://github.com/wrait8/SubRabbit/wiki) for a full overview of the VoidRecon.
