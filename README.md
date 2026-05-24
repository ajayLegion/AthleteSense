# AthleteSense

## Buliding stage 
An ESP32-powered wearable bio-tracker monitoring real-time heart rate, muscle activity (EMG), and environmental metrics via a custom web dashboard.
Here is a clean, professional, GitHub-ready README.md version of your project. It is structured for clarity, technical credibility, and presentation quality.


---

# 🏃 Wearable Bio-Tracker for Athletes

A multifunctional IoT-based wearable system designed to monitor physiological and environmental parameters in real time. This project enables athletes and trainers to track performance, detect fatigue, and optimize training using a custom web dashboard.

---

## 📌 Overview

The Wearable Bio-Tracker integrates multiple biomedical and environmental sensors with an ESP32 microcontroller to collect, process, and transmit real-time data over Wi-Fi. The system provides actionable insights through a web-based dashboard with live visualization and historical analysis.

---

## 🚀 Features

- ❤️ **Heart Rate Monitoring (ECG)**
  - Uses AD8232 sensor to capture real-time cardiac signals

- 💪 **Muscle Activity Tracking (EMG)**
  - MyoWare sensor measures muscle activation and fatigue

- 🌡️ **Environmental Monitoring**
  - Tracks temperature and humidity (DHT11)

- 📡 **Wireless Data Transmission**
  - ESP32 sends data to server via Wi-Fi (HTTP)

- 📊 **Web Dashboard**
  - Real-time graphs and historical data visualization

- 🎽 **Wearable Design**
  - Compact enclosure mounted on a posture-support harness

---

## 🔧 Hardware Components

| Component | Description |
|----------|------------|
| ESP32 Development Board | Main controller with Wi-Fi |
| AD8232 Module | ECG heart rate sensor |
| MyoWare EMG Sensor | Muscle activity sensor |
| DHT11| Temperature & humidity sensor |
| Li-ion Battery (3.7V)  | Portable power source |
| TP4056 Module | Battery charging module |
| Electrodes & Cables | Signal acquisition |

---

## 🔌 Pin Configuration (ESP32)

| Component | ESP32 Pin | Function |
|----------|----------|----------|
| AD8232 Output | GPIO 34 | Analog input |
| AD8232 LO+ / LO- | GPIO 25, 26 | Lead-off detection |
| MyoWare Output | GPIO 35 | Analog input |
| DHT Data | GPIO 4 | Digital input |

> ⚠️ Note: Pin mapping may vary depending on ESP32 variant.

---

## ⚡ Power Design

Recommended setup:
- 3.7V Li-ion battery
- TP4056 charging module

> Avoid using 12V + linear regulators due to inefficiency and heat.

---
Features

Live data streaming

Historical trend analysis

Alerts (heart rate zones, fatigue detection)



---

🧠 Future Enhancements

🔬 Machine Learning for fatigue prediction

🫁 SpO2 (oxygen level) monitoring

📍 GPS tracking for outdoor training

🧍 Posture detection using IMU (MPU6050)

🔋 Low-power optimization



---

⚠️ Challenges & Considerations

Noise in ECG/EMG signals

Motion artifacts during exercise

ADC resolution limitations

Proper electrode placement

Wearable comfort and durability



---

🛠️ Getting Started

1. Hardware Setup

Connect sensors to ESP32 as per pin configuration

Ensure proper grounding and stable power supply


2. Firmware

Upload ESP32 code using Arduino IDE / PlatformIO


3. Server Setup

Run backend server


4. Dashboard

Launch frontend to visualize real-time data




---

📄 License

All rights reserved. This software is proprietary and cannot be distributed or modified without explicit permission from the owner.


---

👨‍💻 Author

Ajaykumar G


---

⭐ Contributing

Contributions are welcome! Feel free to fork this repo and submit pull requests.


---

🙌 Acknowledgements

Sensor module documentation

ESP32 development ecosystem


---

## National hackthon winner
![c](vriddhee2026.webp)
 

