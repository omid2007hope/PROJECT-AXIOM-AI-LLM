import time
from pathlib import Path
from huggingface_hub import hf_hub_download

print("[Axiom Model Fetcher] Starting download of Qwen/Qwen2.5-Coder-1.5B-Instruct...")
out_dir = Path("models/qwen_hf")
out_dir.mkdir(parents=True, exist_ok=True)

t0 = time.time()
path = hf_hub_download(
    repo_id="Qwen/Qwen2.5-Coder-1.5B-Instruct",
    filename="model.safetensors",
    local_dir=str(out_dir)
)
t1 = time.time()
print(f"[Axiom Model Fetcher] Download complete: {path} in {t1 - t0:.1f}s")

