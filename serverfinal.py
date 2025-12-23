from flask import Flask, request
import speech_recognition as sr

app = Flask(__name__)

session = {"state": "WAITING_ORDER", "drink": None}

def stt_from_pcm16(raw_data: bytes):
    r = sr.Recognizer()
    try:
        audio = sr.AudioData(raw_data, 16000, 2)
        text = r.recognize_google(audio).lower()
        return text
    except Exception as e:
        print("STT error:", repr(e))
        return None

def reply(heard_text: str, action: str):
    # Ensure no weird characters or hidden spaces are sent
    response_string = f"HEARD:{heard_text}|{action}"
    print(f"Sending to ESP32: {response_string}") # Add this to see what server sends
    return response_string

@app.route("/reset", methods=["GET"])
def reset():
    session["state"] = "WAITING_ORDER"
    session["drink"] = None
    print("RESET -> WAITING_ORDER")
    return reply("reset", "PLAY:WELCOME")

@app.route("/upload", methods=["POST"])
def upload():
    raw_data = request.data
    if not raw_data:
        print("No audio received")
        return reply("", "PLAY:WELCOME")

    text = stt_from_pcm16(raw_data)

    # If STT failed
    if not text:
        if session["state"] == "WAITING_ORDER":
            return reply("", "PLAY:WELCOME")
        else:
            return reply("", "PLAY:SUGARQUESTION")

    print("User said:", text)

    # ----- WAITING_ORDER -----
    if session["state"] == "WAITING_ORDER":
        drink = None
        if "tea" in text: drink = "tea"
        elif "coffee" in text: drink = "coffee"
        elif "cocoa" in text: drink = "cocoa"

        if drink:
            session["drink"] = drink
            session["state"] = "WAITING_SUGAR"
            print("Drink:", drink, "-> WAITING_SUGAR")
            return reply(text, "PLAY:SUGARQUESTION")
        else:
            return reply(text, "PLAY:WELCOME")

    # ----- WAITING_SUGAR -----
    if session["state"] == "WAITING_SUGAR":
        sugar = None
        if "one" in text or "1" in text: sugar = 1
        elif "two" in text or "2" in text: sugar = 2
        elif "three" in text or "3" in text: sugar = 3
        elif "zero" in text or "no" in text: sugar = 0

        if sugar is not None:
            drink = session["drink"] or "unknown"
            print(f"Final: drink={drink}, sugar={sugar}")

            session["state"] = "WAITING_ORDER"
            session["drink"] = None

            return reply(text, "PLAY:STARTORDER")
        else:
            return reply(text, "PLAY:SUGARQUESTION")

    # fallback
    session["state"] = "WAITING_ORDER"
    session["drink"] = None
    return reply(text, "PLAY:WELCOME")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
