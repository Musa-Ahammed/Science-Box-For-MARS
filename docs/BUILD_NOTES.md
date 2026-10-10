# Build notes

[Back to the project overview](../README.md)

## Mechanical layout

- Keep the funnel fixed and the four-section sample cartridge removable.
- Separate the dry-soil chamber from the pre-watered pH tube and two reagent tubes.
- Keep electronics in a dry compartment with a removable splash shield.
- Shield the color-sensor station from ambient light and keep tube placement consistent.
- Check tray travel, motor direction, and the Hall home sensor before loading samples. The break-beam detects sample arrival, not sample mass.

See the [mechanical concept](../mechanical/Design.png) and [parts list](../hardware/BOM.csv).

## Firmware pin assignments

| Function | Pin in the sketch |
| --- | --- |
| Sample trigger / START / cartridge reset | 2 / 3 / 4 |
| Motor PWM / IN1 / IN2 | 5 / 6 / 7 |
| Hall home sensor | 8 |
| LED / buzzer | 9 / 10 |
| Temperature/humidity | 22, reserved; reading not implemented |
| Moisture / pH input | A0 / A1 |
| Bluetooth | `Serial1`, 9600 baud |
| Wi-Fi module interface | `Serial2`, 115200 baud |
| USB serial | 115200 baud |

The trigger, buttons, and home sensor are active LOW with internal pull-ups. Compare the table with the [schematic image](../hardware/Sch-SB.jpg): its pH/moisture, trigger, and motor labels differ from the sketch.

## Setup status

The design lists a 5 V logic supply and a separate 3.3 V regulator for the ESP-01. Verify module supply requirements, signal levels, and motor-driver wiring before power-up. PCB current capacity has not been verified.

The sketch uses `Wire`; LCD, temperature/humidity, and RGB drivers remain to be added. The ESP-01 network bridge, tube-positioning method, and sensor calibration are also pending. Follow the [test sequence notes](TEST_SEQUENCE.md) when checking motion and lockout behavior.
