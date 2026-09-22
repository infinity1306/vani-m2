with open("models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/tokens.txt", "r", encoding="utf-8") as f:
    lines = [line.strip() for line in f if line.strip()]

token_map = {}
for line in lines:
    parts = line.split()
    if len(parts) == 2:
        token_map[parts[0]] = int(parts[1])

print("Total tokens:", len(token_map))
for sym in [" V", "V", "A", "N", "I", "AN", "ANI", "VA", "VAN", " H", "H", "E", "Y", "HE", "HEY"]:
    print(f"'{sym}' in token_map: {sym in token_map} (ID: {token_map.get(sym)})")
