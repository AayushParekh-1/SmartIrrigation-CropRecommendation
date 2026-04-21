from flask import Flask, request, jsonify, render_template
from pymongo import MongoClient
import datetime, csv, os
from dotenv import load_dotenv

app = Flask(__name__)
load_dotenv()

MONGO_URI = os.getenv("MONGO_URI")
client = MongoClient(MONGO_URI)

db = client["irrigation"]
collection = db["readings"]


# Load crop dataset
def load_crop_data():
    crops = []
    csv_path = os.path.join(os.path.dirname(__file__), "crop_data.csv")

    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            crops.append({
                "crop": row["crop"],
                "temp_min": float(row["temp_min"]),
                "temp_max": float(row["temp_max"]),
                "humidity_min": float(row["humidity_min"]),
                "humidity_max": float(row["humidity_max"]),
                "moisture_min": float(row["moisture_min"]),
                "moisture_max": float(row["moisture_max"]),
                "season": row["season"],
                "w_temp": float(row["w_temp"]),
                "w_humidity": float(row["w_humidity"]),
                "w_moisture": float(row["w_moisture"])
            })

    return crops


CROP_DATA = load_crop_data()


# UPDATED RECOMMENDATION LOGIC ONLY
def get_crop_recommendation(temp, humidity, moisture):
    scores = {}

    for c in CROP_DATA:
        crop = c["crop"]

        t_mid = (c["temp_min"] + c["temp_max"]) / 2
        h_mid = (c["humidity_min"] + c["humidity_max"]) / 2
        m_mid = (c["moisture_min"] + c["moisture_max"]) / 2

        score = 100 - (
            abs(temp - t_mid) * c["w_temp"] +
            abs(humidity - h_mid) * c["w_humidity"] +
            abs(moisture - m_mid) * c["w_moisture"]
        )

        if crop not in scores or score > scores[crop]:
            scores[crop] = round(score, 2)

    best = sorted(scores.items(), key=lambda x: x[1], reverse=True)

    return [x[0] for x in best[:5]]


# ESP32 POST
@app.route("/data", methods=["POST"])
def receive_data():
    d = request.get_json()
    d["timestamp"] = datetime.datetime.utcnow()
    collection.insert_one(d)
    return jsonify({"status": "ok"}), 200


# Manual Input
@app.route("/api/manual", methods=["POST"])
def manual_input():
    d = request.get_json()

    temp = float(d["temperature"])
    humidity = float(d["humidity"])
    moisture = float(d["moisture"])

    crops = get_crop_recommendation(temp, humidity, moisture)

    collection.insert_one({
        "temperature": temp,
        "humidity": humidity,
        "moisture": moisture,
        "pump": "ON" if moisture < 40 else "OFF",
        "timestamp": datetime.datetime.utcnow(),
        "source": "manual"
    })

    return jsonify({
        "crops": crops,
        "temp": temp,
        "humidity": humidity,
        "moisture": moisture
    })


@app.route("/api/readings")
def api_readings():
    rows = list(
        collection.find({}, {"_id": 0})
        .sort("timestamp", -1)
        .limit(60)
    )

    rows.reverse()

    for r in rows:
        r["timestamp"] = r["timestamp"].strftime("%Y-%m-%d %H:%M:%S")

    return jsonify(rows)


@app.route("/api/recommend")
def api_recommend():
    row = collection.find_one({}, {"_id": 0}, sort=[("timestamp", -1)])

    if not row:
        return jsonify({
            "crops": ["No data yet"],
            "temp": 0,
            "humidity": 0,
            "moisture": 0
        })

    t = row["temperature"]
    h = row["humidity"]
    m = row["moisture"]

    return jsonify({
        "crops": get_crop_recommendation(t, h, m),
        "temp": t,
        "humidity": h,
        "moisture": m
    })


@app.route("/")
def dashboard():
    return render_template("dashboard.html")


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001, debug=True)