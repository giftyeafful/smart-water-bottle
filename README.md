## Smart Water Bottle

An ESP32-based embedded systems project designed to monitor water consumption and encourage consistent hydration through automated reminders.

The system tracks changes in the bottle's water level to estimate the amount consumed and displays real-time measurements on an OLED screen. If no water consumption is detected for 30 minutes, an LED and buzzer activate for 3 seconds to remind the user to drink water. The reminder countdown automatically restarts whenever drinking is detected.

The project currently features a working Wokwi simulation using a potentiometer to simulate water-level changes. The physical hardware design incorporates a load cell and HX711 amplifier for weight-based water consumption monitoring.
