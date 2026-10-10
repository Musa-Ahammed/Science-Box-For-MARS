# Current test sequence

[Back to the project overview](../README.md)

The [control sketch](../firmware/ScienceBoxMega.ino) runs these stages:

| Stage | Current behavior |
| --- | --- |
| Idle | A LOW sample trigger or START input begins a test after an 80 ms delay. |
| Homing | Runs the motor toward the Hall reference; an 8-second timeout stops it and enters lockout. |
| Mixing | Reverses the motor every second for 8 seconds. |
| Reading | Reads raw moisture and pH inputs and prints them to USB serial and `Serial1`. |
| Analyzing | Prints `HABITABILITY=UNCONFIGURED_CALIBRATION` to USB serial. |
| Reporting | Prints a status message to USB serial and `RESULT_READY` to `Serial2`. |
| Return home | Runs toward the Hall reference; reaching it or an 8-second timeout advances to completion. |
| Complete | Turns on the LED, sounds the buzzer, marks the cartridge used, and enters lockout. |
| Lockout | Stops the motor and waits for the cartridge-reset input. |

## Details to check before testing

- The 80 ms start delay does not recheck the trigger, so stable-input validation is not implemented.
- A return-home timeout still reports completion; completion alone does not confirm the tray is home.
- Used-cartridge status is held in memory and clears on a power cycle. Replace the cartridge before resetting or restarting.
- LCD messages, temperature/humidity readings, RGB readings, calibrated results, and full Wi-Fi reporting are not implemented.

Mixing times and motor settings are prototype values. Confirm them against the mechanism and chosen test method.
