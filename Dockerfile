FROM python:3.10-slim

WORKDIR /Servidor_Flask_Modelo

COPY requirements.txt .

RUN apt-get update && apt-get install -y \
    libsndfile1 \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir -r requirements.txt

COPY . .

CMD ["python", "server.py"]
FROM python:3.10-slim

# Diretório de trabalho
WORKDIR /Servidor_Flask_Modelo

# Copie o arquivo de requisitos
COPY requirements.txt .

# Instala as dependências do sistema operacional e do Python
RUN apt-get update && apt-get install -y \
    libsndfile1 \
    nginx \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir -r requirements.txt

# Copie o código da aplicação para dentro do contêiner
COPY . .

# Copia o arquivo de configuração do Nginx para o diretório padrão
COPY nginx.conf /etc/nginx/sites-available/default

# Exponha a porta 80 para o tráfego HTTP
EXPOSE 80

# Comando para iniciar o Nginx e o Gunicorn simultaneamente
CMD service nginx start && gunicorn -b 0.0.0.0:5000 server:app
