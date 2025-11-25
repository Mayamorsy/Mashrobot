from flask import Flask, request
import speech_recognition as sr
import wave
import numpy as np 

app = Flask(__name__)

@app.route('/upload', methods=['POST'])
def upload():
    print("\n--- INCOMING AUDIO ---")
    
    raw_data = request.data
    if not raw_data:
        return "Error: No data received"

    # --- DIAGNOSTIC STEP ---
    # This analyzes the raw audio bytes to check volume levels
    try:
        # Convert raw bytes to integers
        audio_array = np.frombuffer(raw_data, dtype=np.int16)
        
        max_val = np.max(audio_array)
        min_val = np.min(audio_array)
        volume_range = max_val - min_val
        
        print(f"Audio Stats:")
        print(f"  Max Value: {max_val}")
        print(f"  Min Value: {min_val}")
        print(f"  Volume:    {volume_range}")

        # --- AUTOMATIC DIAGNOSIS ---
        # 500 is the threshold for "Silence" (Dead Mic)
        if volume_range < 500:
            print("[CRITICAL] DIAGNOSIS: DEAD SILENCE DETECTED")
            print("   -> The microphone is sending the Bias Voltage but NO Sound.")
            print("   -> 1. Check if the mic 'OUT' pin is connected to GPIO 1.")
            print("   -> 2. Check if the mic 'VCC' is 3.3V.")
            return "Error: Silence"
            
        elif max_val > 30000 or min_val < -30000:
            print("[WARNING] DIAGNOSIS: CLIPPING / DISTORTION (Too Loud)")
            
        else:
            print("[OK] DIAGNOSIS: Audio levels look GOOD.")
            
        # Save file so you can listen to it
        with wave.open("debug_audio.wav", "wb") as f:
            f.setnchannels(1)
            f.setsampwidth(2)
            f.setframerate(16000)
            f.writeframes(raw_data)
        
    except Exception as e:
        print(f"Diagnostic Error: {e}")

    # --- GOOGLE RECOGNITION ---
    r = sr.Recognizer()
    
    try:
        audio = sr.AudioData(raw_data, 16000, 2)
        print("Sending to Google...")
        text = r.recognize_google(audio)
        print(f"YOU SAID: {text}")
        return f"Success: {text}"
        
    except sr.UnknownValueError:
        print("Google Result: Unintelligible (Could not understand words)")
        return "Error: Unintelligible"
    except Exception as e:
        print(f"Error: {e}")
        return f"Error: {e}"

if __name__ == '__main__':
    # 0.0.0.0 allows the ESP32 to connect to your PC
    app.run(host='0.0.0.0', port=5000)