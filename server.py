from flask import Flask, request
import speech_recognition as sr

app = Flask(__name__)

@app.route('/upload', methods=['POST'])
def upload():
    print("Receiving audio...")
    
    # 1. Get the raw bytes from ESP32
    raw_data = request.data
    
    # 2. Initialize Recognizer
    r = sr.Recognizer()
    
    try:
        # 3. Create an AudioData object directly from raw bytes
        # 16000 = Sample Rate (Must match Arduino)
        # 2     = Sample Width (2 bytes = 16-bit audio)
        audio = sr.AudioData(raw_data, 16000, 2)
        
        # 4. Send to Google
        print("Processing...")
        text = r.recognize_google(audio)
        print(f"YOU SAID: {text}")
        return f"Success: {text}"
        
    except sr.UnknownValueError:
        print("Google could not understand audio")
        return "Error: Unintelligible"
    except sr.RequestError as e:
        print(f"Google error: {e}")
        return "Error: Connection"
    except Exception as e:
        print(f"General Error: {e}")
        return f"Error: {e}"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)