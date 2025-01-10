FROM python:3.10-slim

WORKDIR /Servidor_Flask_Modelo

COPY requirements.txt .

RUN apt-get update && apt-get install -y \
    libsndfile1 \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir -r requirements.txt

COPY . .

CMD ["python", "server.py"]
