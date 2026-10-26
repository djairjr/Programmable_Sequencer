# Dependencies

Versions the firmware is built and tested with. CI pins the same ones.

| component | version | source |
| --- | --- | --- |
| Arduino-ESP32 core (`esp32:esp32`) | 3.3.12 | Espressif board manager, `https://espressif.github.io/arduino-esp32/package_esp32_index.json` |
| ArduinoJson | 7.4.3 | Arduino Library Manager |
| home-assistant-integration (ArduinoHA) | 2.1.0 | Arduino Library Manager |
| PubSubClient | 2.8 | Arduino Library Manager (dependency of ArduinoHA) |

Board options (FQBN): `esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=custom`.
The custom partition table is `firmware/ProgrammableSequencer/partitions.csv`.

Notes:

- ArduinoHA 2.1.0 has no `text` entity; the sequence entity is published with its own discovery.
- The core enables WiFi modem sleep by default; the firmware turns it off (see `docs/BOARD.md`).
