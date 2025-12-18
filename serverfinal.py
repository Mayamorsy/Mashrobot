from flask import Flask, request, send_file
import speech_recognition as sr
from gtts import gTTS
import os

app = Flask(__name__)

# --- CONVERSATION STATE ---
# This dictionary remembers what is happening
session = {
    "step": "WAITING_ORDER", # Can be: WAITING_ORDER, WAITING_SUGAR
    "current_drink": ""      # Stores "tea", "coffee", etc.
}

RESPONSE_FILE = "response.mp3"

@app.route('/reset', methods=['GET'])
def reset():
    """Called when ESP32 starts. Resets the brain."""
    print("\n--- NEW CUSTOMER ---")
    session["step"] = "WAITING_ORDER"
    session["current_drink"] = ""
    return generate_audio("Hello! I am your Barista. What would you like to drink?")

@app.route('/upload', methods=['POST'])
def upload():
    print("\n--- INCOMING VOICE ---")
    raw_data = request.data
    if not raw_data: return "Error"

    # 1. Speech to Text
    r = sr.Recognizer()
    text = ""
    try:
        # ESP32 sends 16000Hz raw audio
        audio = sr.AudioData(raw_data, 16000, 2)
        text = r.recognize_google(audio).lower()
        print(f"User said: {text}")
    except:
        return generate_audio("I didn't catch that. Please press the button and try again.")

    # 2. Conversation Logic
    reply_text = ""
    command_for_arduino = ""

    # --- STEP 1: LISTEN FOR ORDER ---
    if session["step"] == "WAITING_ORDER":
        # Identify the drink
        found_drink = ""
        if "milk tea" in text: found_drink = "milktea"
        elif "tea" in text:    found_drink = "tea"
        elif "latte" in text:  found_drink = "latte"
        elif "coffee" in text: found_drink = "coffee"
        elif "mocha" in text:  found_drink = "mocha"
        elif "chocolate" in text: found_drink = "chocolate"
        elif "cappuccino" in text: found_drink = "cappuccino"
        
        if found_drink != "":
            # Save the drink and move to next step
            session["current_drink"] = found_drink
            session["step"] = "WAITING_SUGAR"
            reply_text = f"Okay, {found_drink}. How many spoons of sugar?"
        else:
            reply_text = "Sorry, we don't serve that. We have Coffee, Tea, Latte, Cappuccino and Hot Chocolate."

    # --- STEP 2: LISTEN FOR SUGAR ---
    elif session["step"] == "WAITING_SUGAR":
        # Identify the number
        sugar_count = "0"
        if "one" in text or "1" in text: sugar_count = "1"
        elif "two" in text or "2" in text: sugar_count = "2"
        elif "three" in text or "3" in text: sugar_count = "3"
        elif "four" in text or "4" in text: sugar_count = "4"
        elif "zero" in text or "no" in text: sugar_count = "0"
        
        # Confirm and Build Command
        drink = session["current_drink"]
        reply_text = f"Perfect. Making {drink} with {sugar_count} sugars. Please place your cup."
        
        # Format: CMD:recipe:sugar: (e.g., CMD:tea:2:)
        # NOTE: The last colon is important for the ESP32 parser to separate command from URL
        command_for_arduino = f"CMD:{drink}:{sugar_count}:"
        
        # Reset for next customer
        session["step"] = "WAITING_ORDER"
        session["current_drink"] = ""

    # 3. Generate Audio and Return
    audio_url = generate_audio(reply_text)
    
    # Check if we have a command to send
    if command_for_arduino != "":
        # If order is complete, send: CMD:tea:2:http://...
        return command_for_arduino + audio_url
    else:
        # If just chatting (incomplete order), send only: http://...
        return audio_url

def generate_audio(text):
    print(f"Barista: {text}")
    try:
        tts = gTTS(text=text, lang='en')
        tts.save(RESPONSE_FILE)
        return f"http://{request.host}/play_response"
    except Exception as e:
        print(e)
        return "Error"

@app.route('/play_response', methods=['GET'])
def get_audio():
    return send_file(RESPONSE_FILE, mimetype="audio/mpeg")

if __name__ == '__main__':
    # 0.0.0.0 allows external connections (ESP32)
    app.run(host='0.0.0.0', port=5000)