import librosa
import numpy as np
from keras.models import load_model
from features_extraction import get_features

def process_audio(audio_path):
    try:
        # Carregando o áudio
        x, sr = librosa.load(audio_path, sr=16000)  # Certifique-se de usar a taxa de amostragem correta

        # Exibir estatísticas do áudio
        print(f"Amplitude máxima: {np.max(x)}")
        print(f"Amplitude mínima: {np.min(x)}")

        # Extração de features
        features = get_features(x)
        features_array = np.array(features)

        # Normalização
        #features_array_normalized = normalize(features_array)

        # Adicionar dimensão para compatibilidade com o modelo
        features_array_reshaped = np.expand_dims(features_array, axis=2)

        # Exibir informações de depuração
        #print(f"Features antes da normalização: {features}")
        #print(f"Features após a normalização: {features_array_normalized}")
        #print(f"Shape após expansão de dimensão: {features_array_reshaped.shape}")

        return features_array_reshaped
    except Exception as e:
        print(f"Erro ao processar o áudio: {e}")
        return None

def normalize(data):
    # Normalização Z-Score
    mean_val = np.mean(data, axis=0, keepdims=True)
    std_val = np.std(data, axis=0, keepdims=True)

    # Evitar divisões por zero
    std_val = np.where(std_val == 0, 1, std_val)
    data_normalized = (data - mean_val) / std_val

    # Substituir valores NaN por zero
    data_normalized = np.nan_to_num(data_normalized)

    return data_normalized
