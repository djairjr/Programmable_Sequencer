# Updating a unit over USB

The unit is updated with a USB-C cable from a computer. There is no over-the-air update yet.
Every release carries these files (see `SHA256SUMS`):

| file | write at | effect |
| --- | --- | --- |
| `ProgrammableSequencer-X.Y.Z-full.bin` | `0x0` | **clean install**: bootloader, partitions and app; erases the saved settings (sequence, network, page password) |
| `ProgrammableSequencer-X.Y.Z-app.bin` | `0x10000` | app only; **keeps** the saved settings |

Use `full` for a unit whose settings are wrong or unknown (it comes up like a new unit and raises its own
WiFi AP). Use `app` to keep the settings. Do not mix them in the same session.

## 1. Before you start

1. Stop the sequence from the page (STOP). The unit restarts and the lights go off for a few seconds.
2. Connect the board to the computer with a **data** USB-C cable.
3. Check the file with `sha256sum -c SHA256SUMS` (Linux/macOS) or `certutil -hashfile <file> SHA256` (Windows).

## 2. Download mode

Hold **BOOT**, press and release **RESET**, release **BOOT**. A new serial port appears (`COMx` on Windows,
`/dev/ttyACM0` on Linux).

## 3. Write the file

### Espressif Flash Download Tool (Windows, 3.9.5 or newer)

1. ChipType **ESP32-S3**, WorkMode **Develop**, LoadMode **UART**.
2. Tick one row, pick the file and set its address (`0x0` for `full`, `0x10000` for `app`).
3. SPI SPEED **80 MHz**, SPI MODE **DIO**, FLASH SIZE **16 MB**, leave "compress" on.
4. Select the COM port, baud 460800 (use 115200 if it fails). Press **START** and wait for **FINISH**.
5. **Do not press ERASE** and do not add other files. Press RESET on the board.

### esptool (any system, needs Python)

```
pip install esptool
esptool --chip esp32s3 --port COMx --baud 460800 write_flash \
  --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 ProgrammableSequencer-X.Y.Z-full.bin
```

For an app-only update use `0x10000` and the `-app.bin` file.

## 4. Check it worked

1. With no network the unit raises the WiFi `Sequencer-XXXX` within about 10 s (default password `sequencer1234!`) and the
   page opens at `http://192.168.4.1`. If it is on your network, use its IP or `http://quadros.local`.
2. Log in and open `/api/state`: `"version"` must read the version you wrote.
3. After a `full` install, set the sequence and the network again. With autostart on, the sequence starts by itself.

## 5. If something goes wrong

- The port does not appear or the write fails: repeat the download-mode steps and change the cable.
- The unit misbehaves after the update: write the previous release's `full` file the same way and report the
  version shown in `/api/state`.
- When asking for help send: the version, a photo of the tool's window and, if you can, the serial monitor output
  (115200 baud) after typing `status`.
