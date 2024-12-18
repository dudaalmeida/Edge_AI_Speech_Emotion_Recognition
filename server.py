from flask import Flask, request, jsonify
import os
import wave
import librosa
import librosa.display
import soundfile
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from features_extraction import get_features
from keras.models import load_model
import numpy as np


app = Flask(__name__)

# Pasta para armazenar os fragmentos temporários
TEMP_FOLDER = "temp_audio/"
FINAL_AUDIO = "final_audio.wav"

if not os.path.exists(TEMP_FOLDER):
    os.makedirs(TEMP_FOLDER)

fragment_count = 0  # Contador de fragmentos

classes = ["Alegria", "Desgosto", "Medo", "Neutro", "Raiva", "Surpresa", "Tristeza"]

model = load_model("C:/Users/eduarda.almeida/Desktop/Servidor_Flask_Modelo/modelo_emocoes.h5") 

@app.route("/upload", methods=["POST"])
def upload_audio():
    global fragment_count

    # Verifica se a solicitação contém JSON com a flag de término
    if request.is_json:
        data = request.get_json()
        if data.get("end"):
            print("Flag de término detectada. Compilando o arquivo final...")

            # Compilação do áudio final
            with wave.open(FINAL_AUDIO, "wb") as output_wave:
                output_wave.setnchannels(1)
                output_wave.setsampwidth(2)
                output_wave.setframerate(16000)

                for i in range(fragment_count):
                    fragment_path = os.path.join(TEMP_FOLDER, f"fragment_{i}.wav")
                    if os.path.exists(fragment_path):
                        with open(fragment_path, "rb") as frag_file:
                            output_wave.writeframes(frag_file.read())
                        os.remove(fragment_path)  # Remove fragmento compilado

            print(f"Áudio final salvo como '{FINAL_AUDIO}'")
            fragment_count = 0  # Reinicia contador

            audio_arrays = []

            x, sr = librosa.load("C:/Users/eduarda.almeida/Desktop/Servidor_Flask_Modelo/final_audio.wav", sr=44100)
            audio_arrays.append(x)

            #audio_arrays= np.ndarray([audio_arrays])

            #print(model.input_shape)

            X = []

            feature=get_features(x);
            for j in feature:
                X.append(j)
            print(X)

            X_array = np.array(X)
            X_reshape = np.expand_dims(X_array,axis=2)

            print(X_reshape.shape)

            predictions = model.predict(X_reshape)

            print(predictions)

            predicted_class = np.argmax(predictions, axis=1)[0]
            predicted_class_name = classes[predicted_class]

            print(f"Predição: Classe {predicted_class_name}")

            return jsonify({"status": "success", "message": "Áudio compilado", "class": f"{predicted_class_name}"})

    # Caso seja um fragmento de áudio comum
    file_path = os.path.join(TEMP_FOLDER, f"fragment_{fragment_count}.wav")
    with open(file_path, "wb") as f:
        f.write(request.data)
    fragment_count += 1

    print(f"Fragmento {fragment_count} recebido e salvo.")

    #print(audio_arrays)

    return jsonify({"status": "success", "message": "Fragmento recebido"}), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)