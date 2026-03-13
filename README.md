# SmartMeterMonitor

This project implements an application for reading electricity meters (SmartMeter) via the Modbus/RTU interface. Additionally, an optional gas meter sensor can be integrated via the S0 interface (GPIO). The captured measurement values are published as JSON records via MQTT to a broker and can thus be used in smart home systems or for databases (e.g., InfluxDB).

The application is primarily designed for use on a ***sysWORXX Pi-AM62x with Smart Metering HAT*** under Linux and is designed for reliable 24/7 continuous operation. Alternatively, the software can also be used on a Raspberry Pi with RS485 HAT or USB RS485 adapter. However, support for the S0 interface is only provided by the *Smart Metering HAT*:

| Controller Board | Meter Adapter | RS485 SmartMeter | S0 Pulse Sensor |
|------------------|---------------|------------------|-----------------|
| sysWORXX Pi-AM62x | Smart Metering HAT | Yes | Yes |
| sysWORXX Pi-AM62x | USB RS485 Adapter | Yes | No |
| Raspberry Pi | RS485 HAT | Yes | No |
| Raspberry Pi | USB RS485 Adapter | Yes | No |

![\[sysWORXX Pi-AM62x + Smart Metering HAT\]](Documentation/sysPi-AM62x_SM-HAT.png)


## Project Overview

### Supported SmartMeters (Modbus/RTU)

| Device | Description |
|--------|-------------|
| Eastron SDM230 | Single-phase electricity meter |
| Eastron SDM630 | Three-phase electricity meter |

The SmartMeters are connected to the controller board (*sysWORXX Pi-AM62x* or *Raspberry Pi*) via an RS485 adapter (Modbus/RTU). The adapter uses a serial interface (e.g., `/dev/ttyS3`, `/dev/ttyUSB0`).

![\[SmartMeter - SDM230\]](Documentation/SDM230.png)

### Supported Gas Meter Sensors (S0/GPIO)

| Device | Description |
|--------|-------------|
| ES-GAS-2 | Gas meter sensor with S0 pulse output |

The S0 pulse output of the gas meter sensor is connected to a GPIO pin of the controller board (e.g., GPIO 0). The *Smart Metering HAT* for the *sysWORXX Pi-AM62x* has a corresponding S0 input for this purpose.

![\[GasMeter Sensor - ES-GAS-2\]](Documentation/ES-GAS-2.png)


## How It Works

1. **Load configuration:** At startup, the configuration file (typically `config.ini`) is read
2. **MQTT connection:** Connection establishment to the configured MQTT broker
3. **Cyclic polling:** At configurable intervals, all SmartMeters are read via Modbus and, if applicable, the counter value of the gas meter
4. **Data transmission:** The measurement values are published as JSON records to the MQTT broker
5. **KeepAlive:** Regular sending of KeepAlive messages to monitor the application


## Measurement Values

### SDM230 (Single-phase)

| Measurement | Unit | JSON Key |
|-------------|------|----------|
| Voltage L1 | V | `"Voltage_L1":` |
| Current L1 | A | `"Current_L1":` |
| Power L1 | W | `"Power_L1":` |
| Frequency | Hz | `"Frequency":` |
| Energy | kWh | `"Energy":` |

### SDM630 (Three-phase)

| Measurement | Unit | JSON Key |
|-------------|------|----------|
| Voltage L1, L2, L3 | V | `"Voltage_L1":`, `"Voltage_L2":`, `"Voltage_L3":` |
| Current L1, L2, L3 | A | `"Current_L1":`, `"Current_L2":`, `"Current_L3":` |
| Power L1, L2, L3 | W | `"Power_L1":`, `"Power_L2":`, `"Power_L3":` |
| Total Current | A | `"Current_Sum":` |
| Total Power | W | `"Power_Sum":` |
| Frequency | Hz | `"Frequency":` |
| Energy | kWh | `"Energy":` |

### Gas Meter Sensor

| Measurement | Unit | JSON Key |
|-------------|------|----------|
| Pulse Counter | Pulses | `"Counter":` |
| Meter Reading | m³ | `"MeterValue":` |
| Power | W | `"Power":` |
| Energy | kWh | `"Energy":` |


## System Architecture

```
+--------------------+
|                    |
|    MQTT Broker     |
|                    |
+--------------------+
          |
          | LAN/WLAN
          |
+--------------------+     +--------------------+     +------------+
|                    |     |                    |     |  SDM230    |
|                    |     |     sysWORXX       |     |  SDM630    |
|    sysWORXX        |-----|  Smart Metering    |-----|            |
|    Pi-AM62x        |     |       HAT          |     +------------+
|                    |     |                    |       Modbus/RTU
|                    |     |                    |         RS485
+--------------------+     |                    |
                           |                    |     +------------+
                           |                    |-----|  ES-GAS-2  |
                           +--------------------+     +------------+
                                                        DI/Pulse
                                                           S0
```


## Configuration

Configuration is done via an INI file (typically `config.ini`). The file is divided into several sections.

### Section [Settings]

This section contains global settings that apply to the entire application.

| Parameter | Required | Description |
|-----------|----------|-------------|
| MbInterface | Yes | Modbus interface (e.g., `/dev/ttyS0`, `/dev/ttyUSB0`)<br>(sysWORXX Pi-AM62x with Smart Metering HAT: `/dev/ttyS3`) |
| Baudrate | Yes | Baud rate of the serial interface (e.g., `9600`) |
| QueryInterval | Yes | Query interval in seconds (e.g., `60`) |
| InterDevPause | No | Pause between device queries in seconds (e.g., `0.25`) |
| MqttBroker | Yes | IP address and port of the MQTT broker (e.g., `192.168.3.200:1883`) |
| ClientName | No | MQTT client name (Default: `SmartMeterMon_<uid>`) |
| UserName | No | MQTT username for authentication |
| Password | No | MQTT password for authentication |
| StatTopic | Yes | MQTT topic for status messages (Bootup, KeepAlive) |
| DataTopic | Yes | MQTT topic template for measurement data<br>may contain placeholders `<label>` and `<id>` (see below) |
| JsonInfoLevel | No | Detail level of JSON output (see below) |

**Placeholders:**
- `<label>` = replaced by the device label
- `<id>` = replaced by the ModbusID (3 digits)
- `<uid>` = replaced by a UUID generated by the application at runtime

**JsonInfoLevel:**
- `0` = Measurements + device type
- `1` = Measurements + device type + timestamp
- `2` = Measurements + device type + timestamp + detailed information (Label, ModbusID, Range)

### Section [DeviceNNN]

A separate section is created for each SmartMeter. The sections must be numbered consecutively (`Device001`, `Device002`, ...).

| Parameter | Required | Description |
|-----------|----------|-------------|
| Type | Yes | Device type (`SDM230` or `SDM630`) |
| ModbusID | Yes | Modbus Slave ID (1-247) |
| Label | No | Description of the measuring point (for topic and JSON) |

### Section [GasMeter]

Optional configuration for a gas meter with S0 interface.

| Parameter | Required | Description |
|-----------|----------|-------------|
| SensorGpio | Yes | GPIO pin for the gas meter/S0 sensor (0-63)<br>(sysWORXX Pi-AM62x with Smart Metering HAT: `0`) |
| Elasticity | No | "Elasticity factor" for calculating the "Energy" value (1-100, Default: 1, description see below) |
| CounterDelta | No | Minimum pulse difference for update (1-10, Default: 1, description see below) |
| VolumeFactor | Yes | Conversion factor pulses → volume [m³]<br>(e.g., `0.01` for "Elster" type gas meters, 1 pulse = 0.01 m³) |
| EnergyFactor | Yes | Conversion factor volume → energy [kWh/m³]<br>(e.g., `0.1062255`, see below) |
| RetainFile | No | File for storing the counter value (persistence)<br>(e.g., `/home/smm/SmartMeterMonitor/GasMeterRetain.dat`, see below) |
| Label | No | Description of the measuring point (for topic and JSON) |

#### RetainFile

The retain file serves both for initial initialization of the counter value and for its cyclic persistent storage. When the application starts, the value entered in the retain file is adopted as the initial counter value. Immediately before commissioning, the current meter reading of the mechanical counter can be entered there. This is then read when the application starts and adopted as the initial counter value. This ensures that the application uses the same value as the mechanical counter.

The retain file has the following format:
```
MeterValue = 1234.56
```

#### EnergyFactor

The `EnergyFactor` specifies the amount of energy that corresponds to one counter pulse. This is calculated as follows:

```
1 Pulse = Calorific Value * Condition Factor * VolumeFactor
```

Where:
- **Calorific Value:** Energy content per m³ of gas (ask your energy supplier or read from invoice), typical: approx. 10..12 kWh/m³
- **Condition Factor:** Factor that takes into account local temperature and pressure conditions (ask your energy supplier or read from invoice), typical: 0.8..1.0
- **VolumeFactor:** Conversion factor pulses to volume [m³] (read from the gas meter nameplate), e.g.: `0.01` for "Elster" type gas meters, 1 pulse = 0.01 m³

**Example:**
```
1 Pulse = 0.01 m³ => 11.5 kWh/m³ * 0.9237 * 0.01 = 0.1062255 kWh/Pulse
```

#### Elasticity and CounterDelta

The two parameters `Elasticity` and `CounterDelta` control the calculation of the current heat output (parameter `Power:` in [W] in the JSON record).

**Default values:**
- Elasticity = 1
- CounterDelta = 1

These default values are sufficient for large consumers such as gas heating systems in single or multi-family homes, as several counter pulses occur per measurement interval anyway.

**Monitoring Small Consumers:**

For smaller devices such as a gas stove, consumption is often so low that no counter pulse occurs despite continuous consumption over several measurement intervals. The gas meter sensor is a reed contact that delivers one pulse per revolution of the counter. With low consumption (e.g., gas stove), this leads to the following effects:

- No pulse is registered for extended periods (Power = 0 or -1)
- When a pulse occurs, the entire energy is assigned to a short time window (the current measurement interval), resulting in an unrealistically high peak power value

**Example with 60-second query interval:**
```
(3600 sec/h ÷ 60 sec) * 0.1 kWh/Pulse = 6 kW peak value
```

**Parameter 'Elasticity'**

Defines the maximum number of query intervals after which a valid power value is calculated and transmitted.

Behavior:
- As long as neither a counter pulse occurs nor the number of intervals defined as Elasticity is reached, `Power:` is set to `-1`. This signals: "No valid value available"
- When the number of intervals defined as Elasticity is reached WITHOUT a counter pulse, `Power:` is set to `0`. This signals: "Actually no consumption"
- When a counter pulse occurs, the power is averaged over the entire elapsed time since the last calculation.

**Important:** The calculation is based on the actual elapsed time, not on a fixed number of intervals. The later a pulse occurs within the Elasticity window, the longer the time base and the lower the calculated average value.

**Parameter 'CounterDelta'**

Defines by how many counter steps the value must increase before a valid power value is transmitted. This parameter enables smoothing over several pulses, independent of the Elasticity configuration.

**Conditions for Recalculation**

A valid power value (`Power:` >= 0) is calculated if AT LEAST ONE of the following conditions is met:
1. The counter value has increased by at least `CounterDelta` - OR -
2. The `Elasticity` number of query intervals has been reached

**Calculation Formula**

Power is calculated as follows:
```
Power [W] = (CounterDiff * EnergyFactor) * (3600 / TimeSpan_sec) * 1000
```

Where:
- **CounterDiff:** Number of pulses since last calculation
- **EnergyFactor:** Conversion factor pulses to kWh (typically about 0.1 kWh/pulse)
- **TimeSpan_sec:** Seconds since last calculation

**Recommended Settings**

For small consumers (e.g., gas stove):
- Elasticity = 3
- CounterDelta = 2

For large consumers (e.g., gas heating):
- Default values are sufficient (Elasticity = 1, CounterDelta = 1)

**Note:** Settings should be oriented to the smallest consumer in the system. For large consumers, the parameters are less relevant as they generate frequent pulses anyway.

**Practical Guidelines**

A typical gas heating system in a two-family house generates about 0.03 m³/min consumption when the burner is active (approx. 20 kW), which corresponds to about 3 pulses per 60-second query interval.


## Example Configuration

```ini
[Settings]
MbInterface = /dev/ttyS3            ; RS485 Modbus/RTU for SmartMeter
Baudrate = 9600
QueryInterval = 60                  ; [sec]
InterDevPause = 0.25                ; [sec]
MqttBroker = 192.168.3.200:1883
StatTopic = SmartMeter/Status       ; this topic refers to the app globally, not to an single device
DataTopic = SmartMeter/Data/<label> ; is individualized for each device
JsonInfoLevel = 2

[Device001]
Type = SDM630
ModbusID = 10
Label = Flat_1

[Device002]
Type = SDM630
ModbusID = 20
Label = Flat_2

[Device003]
Type = SDM230
ModbusID = 21
Label = HeatPump

[GasMeter]
SensorGpio = 0                      ; GPIO Pin for GasMeter/S0-Sensor
Elasticity = 3
CounterDelta = 2
VolumeFactor = 0.01                 ; 1 Pulse = 0.01 m³
EnergyFactor = 0.1062255            ; value got from energy supplier
RetainFile = /home/smm/GasMeter.dat
Label = NaturalGasMeter
```


## Command Line Usage

*SmartMeterMonitor* is a command-line tool that is typically started as a service at system startup. The tool supports the following command-line parameters:

```
SmartMeterMonitor [OPTION]

OPTION:
    -c=<cfg_file>   Path/Name of Configuration File
    -l=<msg_file>   Logs all Messages sent via MQTT to the specified file
                    (the file is opened always in APPEND mode)
    -o              Run in Offline Mode, without MQTT connection
    -v              Run in Verbose Mode (extended logging output)
    --help          Shows Help Screen
```

**Example call:**
```bash
./SmartMeterMonitor -c=config.ini -v
```


## Startup Script

The example script `run_SmartMeterMonitor.sh` performs the following tasks:
- Setting RS485 mode for the serial interface `/dev/ttyS3` (only necessary for *sysWORXX Pi-AM62x* with *Smart Metering HAT*)
- Setting the library path variable `LD_LIBRARY_PATH` for `libmodbus.so` (see section "Building the Application" below)
- Starting the application

```bash
#!/bin/bash
rs485-setup /dev/ttyS3
export LD_LIBRARY_PATH=/home/smm/bin/lib
./SmartMeterMonitor/SmartMeterMonitor -c=config.ini
```


## Device Tree for sysWORXX Pi-AM62x with Smart Metering HAT

The *sysWORXX Pi-AM62x* supports various IO configurations for the 40-pin header. Control of pin muxing is done Linux-typically using the Device Tree, together with Device Tree Overlays (DTBO) that are loaded separately. To provide the necessary IO configuration for the *Smart Metering HAT*, the corresponding *Smart Metering HAT* overlay must be loaded. To do this, execute the following commands as root user:

```bash
sudo dtbo-setup set k3-am625-systec-sysworxx-pi-hat-smart-metering.dtbo
reboot
```

With active *Smart Metering HAT* overlay, the following settings apply for the configuration file (typically `config.ini`):

```ini
[Settings]
MbInterface = /dev/ttyS3            ; RS485 Modbus/RTU Interface for SmartMeter

[GasMeter]
SensorGpio = 0                      ; GPIO Pin for GasMeter/S0-Sensor
```


## MQTT Messages

### Bootup Message

At each start, a bootup message is published to the configured StatTopic (e.g., Topic: `SmartMeter/Status/Bootup`):

```json
{
  "Version": "1.00",
  "TimeStamp": 1704067200,
  "TimeStampFmt": "2025/01/01 - 00:00:00",
  "ClientName": "SmartMeterMon_A1B2C3D4",
  "KeepAlive": 30
}
```

### Measurement Data Message

Measurement data is published to the configured DataTopic (e.g., Topic: `SmartMeter/Data/HeatPump`):

```json
{
  "Type": "SDM230",
  "TimeStamp": 1704067260,
  "TimeStampFmt": "2025/01/01 - 00:01:00",
  "ModbusID": 21,
  "Label": "HeatPump",
  "Voltage_L1": 230.5,
  "Current_L1": 2.35,
  "Power_L1": 541.2,
  "Frequency": 50.01,
  "Energy": 12345.67
}
```


## Building the Application

### Project Structure

The project structure for the *SmartMeterMonitor* application is divided into the following main components:

- `SmartMeterMonitor/` - Application-specific sources
- `libmodbus/` - 'libmodbus' library for Modbus (used as dynamic library)
- `Mqtt/` - 'paho-mqtt-embedded-c' library for MQTT
- `configparser/` - 'configparser' library as INI file parser

```
SmartMeterMonitor/
  |
  +-- libmodbus/                            # <--- Separate Donload!
  |     |
  |     +-- autogen.sh                      # Build Step (1)
  |     +-- configure                       # Build Step (2)
  |     +-- Makefile                        # Build Step (3)
  |     |
  |     +-- ...
  |
  +-- Mqtt/
  |     |
  |     +-- Mqtt_Transport/
  |     |     +-- MqttTransport.h
  |     |     +-- MqttTransport_Posix.c
  |     |
  |     +-- paho_mqtt_embedded_c/
  |           +-- ...
  |
  +-- configparser/
  |     |
  |     +-- ...
  |
  +-- SmartMeterMonitor/
  |     |
  |     +-- Makefile                        # Build Step (4)
  |     |
  |     +-- SmartMeter/
  |     |     +-- SmartMeter.h
  |     |     +-- SDM230.cpp/h
  |     |     +-- SDM630.cpp/h
  |     |
  |     +-- GasMeter/
  |     |     +-- GasMeter.cpp/h
  |     |     +-- GMSensIf.h
  |     |     +-- GMSensIf_sysPi.cpp
  |     |
  |     +-- Main.cpp
  |     +-- CfgReader.cpp/h
  |     +-- LibMqtt.cpp/h
  |     +-- LibSys.cpp/h
  |     +-- Trace.cpp/h
  |
  +-- config.ini                            # Example Configuration
  +-- run_SmartMeterMonitor.sh              # Start Script
```

The libraries for MQTT and Configparser are fully included in the repository and are directly integrated in the source. The Modbus library is **not** part of the repository. It must be downloaded separately from [https://github.com/stephane/libmodbus](https://github.com/stephane/libmodbus) and unpacked into the project tree. The Modbus library is dynamically loaded at runtime (`libmodbus.so`).

The application is built directly on the target system, so the project tree shown above must be completely present on the target system.

### Build Instructions

The following steps describe building the application on a *sysWORXX Pi-AM62x* with *Smart Metering HAT*. This platform uses a robust Yocto Linux for industrial use, where the `/usr` directory is not writable at runtime. Therefore, the libmodbus library must be generated so that both `libmodbus.h` and `libmodbus.so` are installed within the home directory. To do this, the configure script is called with a corresponding `--prefix=` parameter.

The following assumes that a user `smm` with the associated home directory `/home/smm` exists on the *sysWORXX Pi-AM62x*. The *SmartMeterMonitor* application is created in the following path:

```
/home/smm/SmartMeterMonitor
```

First, the sources of the Modbus library not included in the repository must be copied to the `libmodbus` subdirectory. This can be done either directly with the command `git clone https://github.com/stephane/libmodbus.git`, or via download as a ZIP file from https://github.com/stephane/libmodbus. In this case, the archive `libmodbus-master.zip` is placed in the root directory `/home/smm/SmartMeterMonitor` and unpacked as follows:

```bash
cd /home/smm/SmartMeterMonitor
unzip ./libmodbus-master.zip
mv libmodbus-master libmodbus
```

Now the `libmodbus` library can be built on the target:

```bash
cd /home/smm/SmartMeterMonitor/libmodbus
./autogen.sh
./configure --prefix=/home/smm/bin
make
make install
```

The created `libmodbus` library and the associated header file are now located in the following directories:

```
/home/smm/bin/include/modbus  -> modbus.h
/home/smm/bin/lib             -> libmodbus.so -> libmodbus.so.5.1.0
```

The typical installation in `/usr/local/lib` via the command `sudo make install` fails on the *sysWORXX Pi* due to its R/O filesystem.

If the `libmodbus` library was built in a different directory, the paths to the `libmodbus` library and the associated header file must be adjusted in the application's Makefile:

```makefile
MODBUS_H   = /home/smm/bin/include/modbus
MODBUS_LIB = -L/home/smm/bin/lib -lmodbus
```

For GasMeter support, the define `_GASMETER_` must also be set in the Makefile.

After the project tree shown above including the `libmodbus` library is completely present on the target, the *SmartMeterMonitor* application is built with a simple make call:

```bash
cd /home/smm/SmartMeterMonitor/SmartMeterMonitor
make
```

If the installation path for the `libmodbus` library differs, the line

```bash
export LD_LIBRARY_PATH=/home/smm/bin/lib
```

in the script `run_SmartMeterMonitor.sh` may need to be adjusted. The application can then be started with the script.


## Adding Additional SmartMeter Types

To add additional SmartMeter types to the application, the following steps are necessary:

1. Derive a new class as implementation of the abstract base class `SmartMeter` for the SmartMeter type (see `SmartMeter.h`, use classes `SDM230` and `SDM630` as templates)
2. Integrate the newly created class with a dedicated if-branch in the `BuildSmDeviceList()` function
3. Include the source code file of the newly created class in the Makefile


## Implementing Hardware Interface for Alternative Gas Sensor

To implement the hardware interface for an alternative gas meter sensor, the following steps are necessary:

1. Create a new implementation for the functions defined in `GMSensIf.h` in a separate `*.cpp` file (use `GMSensIf_sysPi.cpp` as reference)
2. Replace references to `GMSensIf_sysPi.*` in the Makefile with references to the newly created source code file


## Commissioning Support for Gas Meter

The gas meter sensor consists only of a simple reed contact that is triggered by a magnet in the mechanical counter of the gas meter with each revolution. For error-free counting function, the sensor must be precisely aligned with the counter.

To verify correct function, a "Pulse Analyzer" is implemented in the hardware interface `GMSensIf_sysPi.cpp`. This is activated or deactivated via the line

```cpp
#define _SENSOR_IMPULSE_ANALYZER_
```

at the beginning of the source code file. When the "Pulse Analyzer" is active, each registered counter pulse is logged in a CSV file with timestamp and current counter value:

```
SensorCounterValue;TimeStamp
3956124;26173991
3956125;26206245
3956126;26230844
3956127;26251704
```

The path and name of the CSV file are defined directly within `GMSensIf_sysPi.cpp`:

```cpp
static const char* ANALYZER_FILE_PATH_NAME = "/tmp/SensorImpAnalyzer.log";
```

By comparing the counter value determined by the application with the respective meter reading of the mechanical counter of the gas meter, the correct counting function of the sensor can be verified.


## Third-Party Components Used

### Modbus/RTU

The "libmodbus" library is used for RS485-based Modbus/RTU communication with the SmartMeters:

[https://github.com/stephane/libmodbus](https://github.com/stephane/libmodbus)

### MQTT Client

The "Paho MQTT Embedded/C" library is used for the MQTT client implementation:

[https://github.com/eclipse/paho.mqtt.embedded-c](https://github.com/eclipse/paho.mqtt.embedded-c)

### INI File Parser

The "configparser" library is used for reading configuration settings from the INI file (typically `config.ini`):

[https://github.com/dzilles/configparser](https://github.com/dzilles/configparser)

The sources contained in this repository implement an extended functionality of the library for this application. The modifications are marked by corresponding comments.


