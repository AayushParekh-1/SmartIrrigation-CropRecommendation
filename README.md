# 🌱 Smart Irrigation & Crop Advisor

An IoT and web-based solution that automates irrigation based on real-time soil and weather conditions, while strategically recommending crops suitable for the current environmental metrics.

## 📌 Overview

The **Smart Irrigation & Crop Advisor** integrates a hardware node (ESP32) and a web dashboard (Flask + MongoDB). 
The hardware node constantly monitors temperature, humidity, and soil moisture to trigger an irrigation system autonomously. Simultaneously, it sends this telemetry data to a centralized Flask server over HTTP. The backend evaluates these metrics against a dataset of crops to recommend the five most suitable crops for the current environment. 

## 🏗 Architecture & Technologies

The system is broken down into three major components:
- **Hardware/IoT Node:** Collects real-time environmental data and automates localized irrigation.
- **Backend API & Database:** Ingests sensor data, computes recommendations, and exposes APIs.
- **Frontend Dashboard:** A web interface providing data visualization and manual overrides.

### Tech Stack
- **Microcontroller:** ESP32
- **Sensors:** DHT11 (Temperature & Humidity), Analog Soil Moisture Sensor
- **Output & Feedback:** 16x2 I2C LCD Display, LED (representing a water pump)
- **Backend:** Python, Flask server
- **Database:** MongoDB (via PyMongo)
- **Frontend:** HTML, CSS, JavaScript (via Flask templates)
- **Dataset:** `crop_data.csv` comprising crop growth metrics.

## 📡 How Data Flow Works

1. **Environmental Sensing:** The ESP32 continuously reads the temperature, humidity, and soil moisture levels.
2. **Local Automation:** If the soil moisture drops below the hardcoded threshold (40%), the ESP32 switches ON the Water Pump (LED indicator).
3. **Data Transmission:** The ESP32 formats the readings into a JSON payload and transmits it via an `HTTP POST` request to the Flask server's `/data` endpoint.
4. **Data Ingestion:** The Flask backend receives the payload, appends a UTC timestamp, and stores it directly into a MongoDB cluster.
5. **Crop Recommendation Pipeline:** 
   - A recommendation algorithm compares the incoming temperature, humidity, and moisture data against the optimal ranges stored in `crop_data.csv`. 
   - It computes a compatibility score based on weighted variances.
6. **Frontend Interaction:**
   - The user visits the web dashboard, triggering `GET` requests to `/api/readings` (fetching the latest 60 readings) and `/api/recommend` (fetching top 5 compatible crops).
   - *Manual Override:* Users can submit custom metrics via the `/api/manual` endpoint on the dashboard to test and record custom states and get theoretical crop recommendations.

## 📂 Project Structure

```text
Smart_Irrigation+CropAdvisor/
│
├── ESP-32Code/
│   └── ESP-32Code.ino            # Complete C++ code for ESP32, sensors & Wifi comms
│
└── irrigation_dashboard/
    ├── app.py                    # Main Flask application, API routes, and recommendation logic
    ├── crop_data.csv             # Dataset detailing optimal environmental limits parameter for crops
    ├── .env                      # Environment Variables configuration (MongoDB URI)
    └── templates/                
        └── dashboard.html        # Front-end dashboard UI 
```

## 🚀 Setup & Installation

### 1. Backend & Web Dashboard

1. **Navigate to the dashboard directory:**
   ```bash
   cd irrigation_dashboard
   ```
2. **Install Python Dependencies:**
   Make sure you have Python installed. You may want to use a virtual environment.
   ```bash
   pip install Flask pymongo python-dotenv
   ```
3. **Configure Environment Variables:**
   Create or modify the `.env` file in the `irrigation_dashboard` directory and add your MongoDB connection string:
   ```env
   MONGO_URI=mongodb+srv://<username>:<password>@cluster...
   ```
4. **Run the Flask Application:**
   ```bash
   python app.py
   ```
   *The server will be hosted on `http://0.0.0.0:5001/` by default.*

### 2. ESP32 Setup

1. Open the `ESP-32Code.ino` in the **Arduino IDE**.
2. Install the necessary libraries via the Library Manager:
   - `HTTPClient`
   - `DHT sensor library` by Adafruit
   - `LiquidCrystal I2C`
   - `ArduinoJson`
3. Configure the specific variables inside the code:
   - Update `ssid` and `password` for your WiFi network.
   - Update `serverURL` to the Local IP address of the machine running your Flask server (e.g., `http://192.168.1.10:5001/data`).
4. **Hardware Wiring:**
   - DHT11 -> `GPIO 4`
   - Soil Moisture Sensor -> `GPIO 34` (Analog input)
   - Pump/LED Indicator -> `GPIO 2`
   - I2C Screen -> Standard I2C Pins (SDA, SCL)
5. **Upload the code** to your ESP32 board.

## 🔮 Future Enhancements
- Deploy the Web Dashboard to an active domain using platforms like Render or Heroku.
- Send alerts or notifications when soil moisture hits a critically low limit.
- Integrate time-series forecasting or ML modeling on collected historical data to predict long-term crop performance. 
- Implement User Authentication on the dashboard.
