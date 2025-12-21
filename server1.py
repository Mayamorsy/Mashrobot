from flask import Flask, request, send_file
import speech_recognition as sr
from gtts import gTTS
import os
import tempfile

app = Flask(__name__)

# --- STATE ---
session = {
    "step": "WAITING_ORDER",
    "current_drink": ""
}

# Cloud servers require using the temporary directory for files
TEMP_DIR = tempfile.gettempdir()
RESPONSE_FILE = os.path.join(TEMP_DIR, "response.mp3")

@app.route('/', methods=['GET'])
def home():
    return "Barista Brain is Online!"

@app.route('/reset', methods=['GET'])
def reset():
    print("\n--- NEW CUSTOMER ---")
    session["step"] = "WAITING_ORDER"
    session["current_drink"] = ""
    return generate_audio("Hello! I am your Barista. What would you like to drink?")

@app.route('/upload', methods=['POST'])
def upload():
    raw_data = request.data
    if not raw_data: return "Error"

    r = sr.Recognizer()
    text = ""
    try:
        # ESP32 sends 16000Hz raw audio
        audio = sr.AudioData(raw_data, 16000, 2)
        text = r.recognize_google(audio).lower()
        print(f"User Said: {text}")
    except:
        return generate_audio("I did not hear you. Please press the button and try again.")

    reply_text = ""
    command_for_arduino = ""

    # Logic
    if session["step"] == "WAITING_ORDER":
        drink = ""
        if "milk tea" in text: drink = "milktea"
        elif "tea" in text:    drink = "tea"
        elif "latte" in text:  drink = "latte"
        elif "coffee" in text: drink = "coffee"
        elif "mocha" in text:  drink = "mocha"
        elif "chocolate" in text: drink = "chocolate"
        elif "cappuccino" in text: drink = "cappuccino"
        
        if drink != "":
            session["current_drink"] = drink
            session["step"] = "WAITING_SUGAR"
            reply_text = f"Okay, {drink}. How many spoons of sugar?"
        else:
            reply_text = "Sorry, please say Tea, Coffee, Latte, or Chocolate."

    elif session["step"] == "WAITING_SUGAR":
        sugar = "0"
        if "one" in text or "1" in text: sugar = "1"
        elif "two" in text or "2" in text: sugar = "2"
        elif "three" in text or "3" in text: sugar = "3"
        elif "four" in text or "4" in text: sugar = "4"
        
        reply_text = f"Coming right up. Making {session['current_drink']} with {sugar} sugars."
        
        # Command Format: CMD:drink:sugar:
        command_for_arduino = f"CMD:{session['current_drink']}:{sugar}:"
        
        session["step"] = "WAITING_ORDER"

    audio_url = generate_audio(reply_text)
    
    if command_for_arduino != "":
        return command_for_arduino + audio_url
    else:
        return audio_url

def generate_audio(text):
    try:
        tts = gTTS(text=text, lang='en')
        tts.save(RESPONSE_FILE)
        # Uses the cloud URL automatically
        return f"{request.host_url}play_response"
    except: return "Error"

@app.route('/play_response', methods=['GET'])
def get_audio():
    return send_file(RESPONSE_FILE, mimetype="audio/mpeg")

if __name__ == '__main__':
    # Cloud assigns a specific port via Environment Variable
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port)