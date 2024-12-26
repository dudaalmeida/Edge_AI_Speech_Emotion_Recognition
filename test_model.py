import librosa
import numpy as np
from keras.models import load_model
from features_extraction import get_features

# Caminho do modelo
MODEL_PATH = "C:/Users/eduarda.almeida/Desktop/Servidor_Flask_Modelo/model_33.h5"
# Caminho do áudio para teste
AUDIO_PATH = "C:/Users/eduarda.almeida/Downloads/VERBO-Dataset/VERBO-Dataset/Audios/Mixed/tri-f6-ns5.wav"

# Classes
classes = ["Alegria", "Desgosto", "Medo", "Neutro", "Raiva", "Surpresa", "Tristeza"]

# Carregar modelo
model = load_model(MODEL_PATH)

# Função para carregar e processar o áudio
def process_audio(audio_path):
    try:
        x, sr = librosa.load(audio_path, sr=16000)  # Certifique-se de usar a taxa de amostragem correta
        features = get_features(x)  # Extração de features
        features_array = np.array(features)

        print(f"features_array: {features_array.shape}")

        # Calculando os valores mínimos e máximos globais
        #min_val = features_array[0].min(axis=0, keepdims=True)
        #max_val = features_array[0].max(axis=0, keepdims=True)

        #print(f"min_val: {min_val} e max_val: {max_val}")

        # Evitar divisão por zero
        #range_val = max_val - min_val
        #range_val = np.where(range_val == 0, 1, range_val)
        #print(f"range_val: {range_val}")

        # Normalizando para o intervalo [0, 1]
        #features_array_norm = (features_array - min_val) / range_val
        #print(f"features_array_norm: {features_array_norm}")

        # Verificando valores inválidos
        #features_array_norm = np.nan_to_num(features_array_norm)
        #print(f"features_array_norm: {features_array_norm}")

        # Adicionando dimensão extra
        #features_array_reshaped = np.expand_dims(features_array_norm, axis=2)
        features_array_reshaped = np.expand_dims(features_array, axis=2)
        print(f"features_array_reshaped: {features_array_reshaped}")

        return features_array_reshaped
    except Exception as e:
        print(f"Erro ao processar o áudio: {e}")
        return None

# Processar o áudio
processed_audio = process_audio(AUDIO_PATH)

if processed_audio is not None:
    # Fazer a previsão
    predictions = model.predict(processed_audio)
    predicted_class = np.argmax(predictions, axis=1)[0]
    predicted_class_name = classes[predicted_class]

    print(f"Predições: {predictions}")
    print(f"Classe prevista: {predicted_class_name}")
else:
    print("Não foi possível processar o áudio.")
