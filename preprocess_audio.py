import numpy as np

# Função para normalizar o áudio para a faixa de -1 a 1
def normalize(audio):
    max_val = np.max(np.abs(audio))
    if max_val == 0:
        return audio  # Evitar divisão por zero, retorna o áudio sem alterações
    return audio / max_val