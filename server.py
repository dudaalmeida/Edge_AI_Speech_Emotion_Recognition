from flask import Flask, request, jsonify
import numpy as np
import tensorflow as tf
from keras.models import load_model
from features_extraction import get_features
from preprocess_audio import normalize

app = Flask(__name__)

# Carregar o modelo treinado
model = tf.keras.models.load_model("C:/Users/eduarda.almeida/Downloads/Speech_Emotion_Recognition/modelo_emocoes.h5")

@app.route('/', methods=['POST'])

def process_audio():
    # Recebe os dados de áudio enviados pelo ESP32
    if not request.data:  # Verifica se há dados no corpo da requisição
        return jsonify({"error": "Arquivo de áudio não encontrado"}), 400

    #audio_data = request.data
    audio_data = np.frombuffer(request.data, dtype=np.int16)
    
    audio = normalize(audio_data)
    audio = np.array(audio)

    return f"Tamanho dos dados de áudio recebidos: {audio}"

    for i in range(len(audio)):
        features = get_features(audio[i])

    # Verificar o formato do áudio recebido
    #return f"Tamanho dos dados de áudio recebidos: {audio_data.shape}"
    return f"Tamanho dos dados de áudio recebidos: {features}"

    # Processa o áudio para o formato de entrada do modelo
    audio_data = audio_data.reshape(1, -1, 1)

    # Realiza a inferência
    predictions = model.predict(audio_data)
    emotion_index = np.argmax(predictions)
    confidence = predictions[0][emotion_index]

    return jsonify({
        "emotion_index": int(emotion_index),
        "confidence": float(confidence)
    })

if __name__ == '__main__':
    # Roda o servidor no seu computador
    app.run(host='192.168.0.83', port=5000)
