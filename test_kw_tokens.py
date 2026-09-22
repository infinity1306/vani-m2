with open("models/sherpa-onnx-streaming-zipformer-en-20M-2023-02-17/tokens.txt", "r", encoding="utf-8") as f:
    tokens = [line.strip().split()[0] for line in f if line.strip() and len(line.strip().split()) == 2]

token_set = set(tokens)

def check_keyword(kw_line):
    toks = kw_line.split()
    all_ok = True
    for t in toks:
        if t not in token_set:
            print(f"Token '{t}' NOT found in tokens.txt")
            all_ok = False
    if all_ok:
        print(f"Keyword '{kw_line}' is VALID! Tokens: {[tokens.index(t) for t in toks]}")

print("Checking 'V A N I':")
check_keyword("V A N I")

print("Checking 'V AN I':")
check_keyword("V AN I")

print("Checking 'H E Y':")
check_keyword("H E Y")
