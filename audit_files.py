import os
import hashlib

def get_file_info(filepath):
    size = os.path.getsize(filepath)
    hasher = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while chunk := f.read(65536):
            hasher.update(chunk)
    return size, hasher.hexdigest()

found_files = []
for root, dirs, files in os.walk('.'):
    if 'node_modules' in root or '.git' in root or 'build' in root or '.cache' in root:
        continue
    for file in files:
        ext = os.path.splitext(file)[1].lower()
        if ext in ['.onnx', '.wav', '.flac', '.mp3', '.ogg', '.tar', '.bz2', '.zip']:
            path = os.path.join(root, file)
            size, sha = get_file_info(path)
            found_files.append((path, size, sha))

print(f"Total binary/audio/model files found: {len(found_files)}")
for p, s, sha in found_files:
    print(f"File: {p}\n  Size: {s} bytes\n  SHA-256: {sha}\n")
