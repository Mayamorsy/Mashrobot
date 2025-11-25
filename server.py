from flask import Flask, request, send_file
import speech_recognition as sr
import wave
import numpy as np
from gtts import gTTS
import os

app = Flask(__name__)

# Store the latest response file here
RESPONSE_AUDIO_FILE = "response.mp3"

@app.route('/upload', methods=['POST'])
def upload():
    print("\n--- INCOMING AUDIO ---")
    raw_data = request.data
    if not raw_data: return "Error: No data"

    # 1. Diagnostics
    try:
        audio_array = np.frombuffer(raw_data, dtype=np.int16)
        volume_range = np.max(audio_array) - np.min(audio_array)
        if volume_range < 500:
            print("[FAIL] MIC IS DEAD (Silence)")
            return "Error: Silence"
    except: pass

    # 2. Speech to Text
    r = sr.Recognizer()
    text_response = ""
    
    try:
        audio = sr.AudioData(raw_data, 16000, 2)
        print("Recognizing...")
        user_text = r.recognize_google(audio)
        print(f"YOU SAID: {user_text}")
        
        # 3. Logic: Decide what to say back
        user_text = user_text.lower()
        if "hello" in user_text:
            text_response = "Hello there! I am your ESP32 assistant."
        elif "light" in user_text:
            text_response = "I can not control lights yet, but I am learning."
        elif "time" in user_text:
            text_response = "It is time to build more robots."
        else:
            text_response = f"You said {user_text}"

    except sr.UnknownValueError:
        print("Unintelligible")
        text_response = "I could not understand you."
    except Exception as e:
        print(f"Error: {e}")
        text_response = "System error."

    # 4. Text to Speech (Generate MP3)
    print(f"GENERATING TTS: {text_response}")
    try:
        tts = gTTS(text=text_response, lang='en')
        tts.save(RESPONSE_AUDIO_FILE)
        print("Saved response.mp3")
    except Exception as e:
        print(f"TTS Error: {e}")
        return "Error: TTS Failed"

    # 5. Return the URL to play
    # This tells ESP32: "Go download the audio from this link"
    return f"http://{request.host}/play_response"

@app.route('/play_response', methods=['GET'])
def get_audio():
    # ESP32 calls this to download the MP3
    print("ESP32 is downloading the audio response...")
    return send_file(RESPONSE_AUDIO_FILE, mimetype="audio/mpeg")

if __name__ == '__main__':
    # Ensure you use the correct IP
    app.run(host='0.0.0.0', port=5000)