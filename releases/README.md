# Releases

Compiled firmware, one folder per version. The same files are attached to the matching
[GitHub Release](https://github.com/djairjr/Programmable_Sequencer/releases).

| file | write at | effect |
| --- | --- | --- |
| `ProgrammableSequencer-X.Y.Z-full.bin` | `0x0` | clean install; **erases the saved settings** |
| `ProgrammableSequencer-X.Y.Z-app.bin` | `0x10000` | app only; keeps the saved settings |
| `SHA256SUMS` | | check with `sha256sum -c SHA256SUMS` |

How to write them to a unit: [../docs/UPDATING.md](../docs/UPDATING.md).

| version | notes |
| --- | --- |
| [1.3.1](1.3.1/) | Fixes the login loop. Latest. Tested on a board. |
