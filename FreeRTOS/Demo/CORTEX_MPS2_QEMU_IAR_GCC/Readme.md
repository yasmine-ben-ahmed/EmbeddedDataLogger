# Embedded Data Logger Project

## Overview
This project is a simulated **Embedded Data Logger** using **FreeRTOS** on a Cortex-M microcontroller architecture (via QEMU).  
It reads sensor data (temperature, humidity, and light), processes averages, detects rapid temperature changes, logs the data, and handles commands.

The project is fully implemented with FreeRTOS tasks, queues, and static memory allocation for safe, real-time operation.

---

## Features

- **Sensor Reading**: Simulates temperature, humidity, and light sensors.
- **Data Processing**: Calculates average temperature and humidity over samples.
- **Alerts**: Detects rapid temperature changes and prints alerts in red.
- **Data Logging**: Logs sensor data for analysis.
- **System Monitoring**: Monitors system heap usage and error counts.
- **Command Handling**: Supports simulated commands (`STATUS`, `READ_SENSORS`, `CALIBRATE`).

---

## Project Structure

EmbeddedDataLogger/
├── FreeRTOS/ # FreeRTOS source files
├── Demo/ # Demo project files
├── build/ # Build output (binaries)
├── main.c # Main source code (tasks and scheduler)
├── README.md # This file
└── Makefile / project files


---

## Tasks Overview

| Task Name          | Priority                  | Purpose |
|-------------------|---------------------------|---------|
| SensorReaderTask   | High (SENSOR_TASK_PRIO)  | Read sensors and send data to queue |
| DataProcessorTask  | Medium (PROCESS_TASK_PRIO)| Calculate averages and generate alerts |
| AlertTask          | Low (ALERT_TASK_PRIO)     | Print alert messages |
| DataLoggerTask     | Low (LOGGER_TASK_PRIO)    | Log sensor data |
| SystemMonitorTask  | Medium (MONITOR_TASK_PRIO)| Monitor system health |
| CommandHandlerTask | Medium (COMMAND_TASK_PRIO)| Handle commands from queue |

---

## Dependencies

- [FreeRTOS](https://www.freertos.org/)
- GCC ARM toolchain
- QEMU ARM emulator

---

## How to Run

1. **Build the project** using GCC (IAR or other toolchains can also be used).  
2. **Run in QEMU**:
```bash
qemu-system-arm -machine mps2-an385 -cpu cortex-m3 \
-kernel build/output/RTOSDemo.out \
-monitor none -nographic -serial stdio


Observe the logs in the console:

Sensor readings

Processed averages

Alerts for rapid temperature changes

System monitor messages

Command handling messages

Sample Output

--- SENSOR LOG START ---
| Tick | Temp(°C) | Humidity(%) | Light(lux) |
---------------------------------------------
| 1    | 23.5     | 45.32       | 400        |
[PROCESS] Avg Temp: 23.5°C Avg Hum: 45.32% (Samples: 1)
[ALERT] Rapid temp change: 23.5°C -> 30.2°C
[MONITOR] Tick:3 Heap:72% Errors:1
[COMMAND] Tick:5 Processing:STATUS -> OK (Cmd #1)


Notes

All tasks use static memory allocation for reliability in embedded systems.

Rapid temperature change threshold is set to 5°C by default.

Alerts are displayed with red color in the console using ANSI escape codes.

License

This project is MIT Licensed. You are free to use, modify, and distribute it.

