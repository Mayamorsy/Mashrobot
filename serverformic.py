from flask import Flask, request
import speech_recognition as sr

app = Flask(__name__)

# --- STATE ---
session = {
    "step": "WAITING_ORDER", 
    "current_drink": ""      
}

@app.route('/upload', methods=['POST'])
def upload():
    raw_data = request.data
    if not raw_data: return "ERROR"

    # 1. Speech to Text
    r = sr.Recognizer()
    text = ""
    try:
        # ESP32 sends 16000Hz raw audio
        audio = sr.AudioData(raw_data, 16000, 2)
        text = r.recognize_google(audio).lower()
        print(f"User said: {text}")
    except:
        return "RETRY" # Tell ESP32 to flash led or something

    # 2. Logic
    response_command = "WAIT" # Default: do nothing yet

    # --- STEP 1: LISTEN FOR ORDER ---
    if session["step"] == "WAITING_ORDER":
        if "tea" in text:      session["current_drink"] = "tea"
        elif "coffee" in text: session["current_drink"] = "coffee"
        elif "cocoa" in text:  session["current_drink"] = "cocoa"
        
        # If we found a drink, ask for sugar next
        if session["current_drink"] != "":
            session["step"] = "WAITING_SUGAR"
            print(f"Got {session['current_drink']}, waiting for sugar...")
            return "WAIT" # Still collecting info
        else:
            return "RETRY"

    # --- STEP 2: LISTEN FOR SUGAR ---
    elif session["step"] == "WAITING_SUGAR":
        sugar_count = 0
        if "one" in text or "1" in text: sugar_count = 1
        elif "two" in text or "2" in text: sugar_count = 2
        elif "three" in text or "3" in text: sugar_count = 3
        elif "zero" in text or "no" in text: sugar_count = 0
        
        # FINAL COMMAND
        drink = session["current_drink"]
        
        # Reset for next customer
        session["step"] = "WAITING_ORDER"
        session["current_drink"] = ""
        
        # Send command format: CMD:drink:sugar
        # Example: CMD:coffee:2
        print(f"Sending Command: CMD:{drink}:{sugar_count}")
        return f"CMD:{drink}:{sugar_count}"

    return "WAIT"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)