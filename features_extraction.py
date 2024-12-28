# feature_extraction.py
import librosa
import numpy as np

sr = 16000  # Taxa de amostragem que você está usando para o áudio

def extract_features(data):
    # Zero Crossing Rate
    result = np.array([])
    zcr = np.mean(librosa.feature.zero_crossing_rate(y=data).T, axis=0)
    result = np.hstack((result, zcr))

    # Chroma_stft
    stft = np.abs(librosa.stft(data))
    chroma_stft = np.mean(librosa.feature.chroma_stft(S=stft, sr=sr, n_fft=200).T, axis=0)
    result = np.hstack((result, chroma_stft))

    # MFCC
    mfcc = np.mean(librosa.feature.mfcc(y=data, sr=sr, n_fft=200).T, axis=0)
    result = np.hstack((result, mfcc))

    # MelSpectogram
    mel = np.mean(librosa.feature.melspectrogram(y=data, sr=sr, n_fft=200).T, axis=0)
    result = np.hstack((result, mel))

    # Tonnetz
    tonnetz = np.mean(librosa.feature.tonnetz(y=data, sr=sr).T, axis=0)
    result = np.hstack((result, tonnetz))

    return result

def remove_silence(data, sr=16000, top_db=20):
    # Encontrar intervalos com energia acima do limiar
    intervals = librosa.effects.split(data, top_db=top_db)
    
    # Concatenar os intervalos não silenciosos
    non_silent_data = np.concatenate([data[start:end] for start, end in intervals])
    return non_silent_data

def get_features(data):
    result = []
    
    data = remove_silence(data)

    res3 = extract_features(data)

    result.append(res3)

    return result
