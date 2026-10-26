# Dependencies

Versions the firmware is built and tested with. CI pins the same ones.

| component | version | license | source |
| --- | --- | --- | --- |
| Arduino-ESP32 core (`esp32:esp32`) | 3.3.12 | LGPL-2.1-or-later | Espressif board manager, `https://espressif.github.io/arduino-esp32/package_esp32_index.json` |
| ArduinoJson | 7.4.3 | MIT | Arduino Library Manager |
| home-assistant-integration (ArduinoHA) | 2.1.0 | **AGPL-3.0** | Arduino Library Manager |
| PubSubClient | 2.8 | MIT | Arduino Library Manager (dependency of ArduinoHA) |

Board options (FQBN): `esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=custom`.
The custom partition table is `firmware/ProgrammableSequencer/partitions.csv`.

Notes:

- ArduinoHA 2.1.0 has no `text` entity; the sequence entity is published with its own discovery.
- The core enables WiFi modem sleep by default; the firmware turns it off (see `docs/BOARD.md`).

## Licenses

The source files of this repository are under the [MIT License](LICENSE). The firmware images in `releases/` are
built from that source **plus** the libraries above, so they are also subject to those licenses. In particular
ArduinoHA is licensed under the GNU Affero General Public License v3.0 (license text shipped with the library) and
the Arduino-ESP32 core under the LGPL; the complete corresponding source of the firmware is this repository.
Licenses were read from the installed library and core files on 2026-10-26.
