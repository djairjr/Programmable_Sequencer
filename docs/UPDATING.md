# Updating a unit over USB with a web flasher

You do not need to install anything: the firmware is written from the browser with Espressif's web flasher,
**https://espressif.github.io/esptool-js/**. The unit still has to be connected to the computer with a USB-C
cable, because there is no over-the-air update yet.

## What you need

- A computer with **Chrome or Edge**. The flasher page says it is not supported on Safari.
- A USB-C **data** cable (a charge-only cable will not work).
- One firmware file from [`releases/`](../releases/) (or from the
  [GitHub Release](https://github.com/djairjr/Programmable_Sequencer/releases/latest)):

| file | write at | effect |
| --- | --- | --- |
| `ProgrammableSequencer-X.Y.Z-app.bin` | `0x10000` | **recommended**: app only, keeps the saved settings (sequence, network, page password) |
| `ProgrammableSequencer-X.Y.Z-full.bin` | `0x0` | clean install: bootloader, partitions and app; **erases the saved settings**; use it for a blank or misbehaving unit |

Write **one** of them, not both.

## 1. Before you start

1. Stop the sequence from the page (Stop). The unit restarts and the loads go off for a few seconds.
2. Close any other program that uses the board's serial port (Arduino IDE, a serial monitor): only one program
   can use it at a time.
3. Optional: check the file with `sha256sum -c SHA256SUMS` (Linux/macOS) or `certutil -hashfile <file> SHA256`
   (Windows) and compare with the line in `SHA256SUMS`.

## 2. Put the board in download mode

Connect the board to the computer, then: hold **BOOT**, press and release **RESET**, release **BOOT**.

## 3. Write the file

1. Open https://espressif.github.io/esptool-js/ in Chrome or Edge.
2. Leave **WebUSB (CH340)** unchecked.
3. Pick the **Baudrate** `460800` (use `115200` if the write fails).
4. Click **Connect**. The browser asks which serial port to use: choose the one that appeared when you plugged
   the board in. If unsure, unplug the board, open the list, note the entries, plug the board again and pick
   the new one.
5. After connecting, the page shows **Flash Mode**, **Flash Frequency** and **Flash Size**. Leave them as they are
   (`keep`).
6. In the **Flash Address** box type the address from the table above (`0x10000` for `app`, `0x0` for `full`) and
   use **File** to choose the `.bin`. Use only this one row; do not **Add File**.
7. Click **Program** and wait until the console reports that it is done. Do not unplug the board meanwhile.
8. **Do not click Erase Flash.** It erases the whole chip, including the settings, and with an `app` image it
   would also erase the bootloader, so the unit would not start until a `full` image is written.
9. Click **Disconnect**, then press **RESET** on the board (or unplug and plug it again).

## 4. Check it worked

1. With no network the unit raises the WiFi `Sequencer-XXXX` within about 10 s (default password
   `sequencer1234!`) and the page opens at `http://192.168.4.1`. If it is on your network, use its IP or
   `http://sequencer.local`.
2. Log in and open `/api/state`: `"version"` must read the version you wrote.
3. After a `full` install, set the sequence and the network again. With autostart on, the sequence starts by itself.

## 5. If something goes wrong

- **Connect fails or no port is listed:** repeat the download-mode steps, try `115200`, a different USB port or a
  different cable. On Linux the user also needs permission on the serial device (usually the `dialout` group).
- **The write stops halfway:** do not use the unit; repeat from step 2 with the same file. A `full` image always
  brings a unit back to a known state.
- **Asking for help:** send the version from `/api/state`, a screenshot of the flasher console and, if you can,
  the serial monitor output (115200 baud) after typing `status`.

## Alternative: command line

```
pip install esptool
esptool --chip esp32s3 --port COMx --baud 460800 write_flash \
  --flash_mode dio --flash_freq 80m --flash_size 16MB 0x10000 ProgrammableSequencer-X.Y.Z-app.bin
```

Use `0x0` and the `-full.bin` file for a clean install.
