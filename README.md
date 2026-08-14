# 🦏 Rhino — ESP32 Battle Tank

**Rhino** is an ESP32-powered battle tank controlled by a FlySky radio, a RoboCore Serial Relay Module, and custom electronics.

This is a project I’ve always dreamed of bringing to life: combining electronics, programming, mechanics, and remote control to build my own functional battle tank.

## 🎮 Why “Rhino”?

The name **Rhino** comes from one of my favorite games: **Grand Theft Auto**.

I grew up playing GTA, and the military tank in the game was called **Rhino**. When I finally decided to build my own RC battle tank, the name was an easy choice.

A little bit of GTA nostalgia turned into a real maker project. 🦏🔥

https://www.youtube.com/shorts/cIAKxjZ56U4

## ⚙️ Hardware

The project uses:

* ESP32
* FlySky transmitter and iBUS receiver
* RoboCore Serial Relay Module
* 4 relay channels
* Two motors/tracks
* Power supply/battery
* Custom mechanical structure
* Wiring and connectors

## 🔌 ESP32 Connections

### FlySky Receiver

```text
FlySky Receiver     ESP32
--------------------------
GND              -> GND
VCC / 5V         -> 5V / VIN
iBUS             -> GPIO 16
```

The ESP32 receives commands from the FlySky receiver using the **iBUS protocol**.

### RoboCore Serial Relay Module

```text
RoboCore Module     ESP32
--------------------------
DATA             -> GPIO 25
CLOCK            -> GPIO 26
GND              -> GND
```

Make sure the **ESP32, FlySky receiver, and relay module share a common GND**.

## 🕹️ Track Controls

The two tracks are controlled independently using relays.

```text
COMMAND       RELAY 1       RELAY 2
------------------------------------
STOP          OFF           OFF
FORWARD       ON            ON
LEFT          ON            OFF
RIGHT         OFF           ON
```

* **Relay 1:** Left track
* **Relay 2:** Right track

Since this version doesn't use an H-bridge, the tracks only need simple ON/OFF control.

## 🎯 Auxiliary Control

**Relay 3** controls an additional mechanism.

Moving the auxiliary analog control activates Relay 3, and releasing it turns the relay off.

```text
Aux control activated -> Relay 3 ON
Aux control released  -> Relay 3 OFF
```

## 🔥 Trigger System

Relay 4 works as a special trigger.

To arm the trigger:

```text
CH5 -> MAX
CH6 -> MAX
```

Then use the same auxiliary control normally assigned to Relay 3.

```text
CH5 MAX
   +
CH6 MAX
   +
AUX activated
   ↓
Relay 4 ON
   ↓
1 second
   ↓
Relay 4 OFF
```

While trigger mode is armed, Relay 3 remains OFF.

The control must be released before another trigger pulse can occur.

## 🛡️ Failsafe

Safety is an important part of the project.

If the radio signal is lost or the transmitter is turned off, the ESP32 automatically commands:

```text
Relay 1 -> OFF
Relay 2 -> OFF
Relay 3 -> OFF
Relay 4 -> OFF
```

This prevents the tank or auxiliary mechanisms from remaining active after losing control communication.

## 📡 Control Channels

The current configuration uses:

```text
CH1 -> Auxiliary control / Trigger
CH2 -> Failsafe
CH3 -> Forward
CH4 -> Left / Right
CH5 -> Trigger arming
CH6 -> Trigger arming
```

## 💻 Software

The firmware is written using the **Arduino framework for ESP32**.

Main libraries:

```cpp
#include <IBusBM.h>
#include <SerialRelay.h>
```

`IBusBM` handles communication with the FlySky receiver, while `SerialRelay` controls the RoboCore Serial Relay Module.

## 🚀 Project Goal

Rhino is more than just an RC vehicle.

The goal is to experiment with:

* ESP32 development
* Radio control
* iBUS communication
* Motors and relays
* Embedded programming
* Mechanical design
* Electronics
* Safety systems
* Custom RC vehicles

It started as a tank I used to drive around in GTA.

Now I'm building my own.

**Welcome to Project Rhino. 🦏**
