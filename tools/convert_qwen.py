import json
import struct
import sys
from pathlib import Path

def export_qwen_to_axiom(model_dir: str, out_weights: str, out_vocab: str):
    from safetensors import safe_open
    import torch

    model_dir_path = Path(model_dir)
    safetensors_path = model_dir_path / "model.safetensors"
    tokenizer_path = model_dir_path / "tokenizer.json"

    print(f"[Axiom Converter] Packing weights from {safetensors_path}...")
    f = safe_open(str(safetensors_path), framework="pt", device="cpu")

    # Header: magic(0x4D4F4958="MOIX"), version(2), num_layers(28), hidden_dim(1536),
    # num_q_heads(12), num_kv_heads(2), head_dim(128), ffn_dim(8960), vocab_size(151936),
    # max_seq_len(4096), rms_norm_eps(1e-6), rope_theta(1000000.0) + 16 bytes reserved
    header = struct.pack(
        "<10I2f16x",
        0x4D4F4958, 2, 28, 1536, 12, 2, 128, 8960, 151936, 4096,
        1e-6, 1000000.0
    )

    def write_bf16(out_file, tensor_name: str):
        t = f.get_tensor(tensor_name).to(torch.bfloat16).contiguous()
        out_file.write(t.view(torch.uint16).numpy().tobytes())

    Path(out_weights).parent.mkdir(parents=True, exist_ok=True)
    Path(out_vocab).parent.mkdir(parents=True, exist_ok=True)

    with open(out_weights, "wb") as out:
        out.write(header)
        write_bf16(out, "model.embed_tokens.weight")
        for l in range(28):
            p = f"model.layers.{l}"
            write_bf16(out, f"{p}.input_layernorm.weight")
            write_bf16(out, f"{p}.self_attn.q_proj.weight")
            write_bf16(out, f"{p}.self_attn.q_proj.bias")
            write_bf16(out, f"{p}.self_attn.k_proj.weight")
            write_bf16(out, f"{p}.self_attn.k_proj.bias")
            write_bf16(out, f"{p}.self_attn.v_proj.weight")
            write_bf16(out, f"{p}.self_attn.v_proj.bias")
            write_bf16(out, f"{p}.self_attn.o_proj.weight")
            write_bf16(out, f"{p}.post_attention_layernorm.weight")
            write_bf16(out, f"{p}.mlp.gate_proj.weight")
            write_bf16(out, f"{p}.mlp.up_proj.weight")
            write_bf16(out, f"{p}.mlp.down_proj.weight")
        write_bf16(out, "model.norm.weight")
        if "lm_head.weight" in f.keys():
            write_bf16(out, "lm_head.weight")
        else:
            write_bf16(out, "model.embed_tokens.weight")

    if tokenizer_path.exists():
        print(f"[Axiom Converter] Exporting vocabulary from {tokenizer_path}...")
        tok_data = json.loads(tokenizer_path.read_text(encoding="utf-8"))
        vocab = tok_data.get("model", {}).get("vocab", {})
        for added in tok_data.get("added_tokens", []):
            vocab[added["content"]] = added["id"]

        with open(out_vocab, "wb") as vf:
            vf.write(struct.pack("<I", len(vocab)))
            for token_str, token_id in vocab.items():
                raw_str = token_str.replace("Ġ", " ").replace("Ċ", "\n").encode("utf-8")
                vf.write(struct.pack("<II", int(token_id), len(raw_str)))
                vf.write(raw_str)

    print("[Axiom Converter] Conversion complete.")

if __name__ == "__main__":
    src_dir = sys.argv[1] if len(sys.argv) > 1 else "./Qwen2.5-Coder-1.5B-Instruct"
    export_qwen_to_axiom(src_dir, "models/qwen1_5b.axiom", "models/qwen_vocab.axiom")

