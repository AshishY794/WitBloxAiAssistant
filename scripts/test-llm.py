"""Quick English-only LLM test for WitBlox (uses data/.config.yaml)."""

import os
import sys
from pathlib import Path

SERVER_ROOT = Path(__file__).resolve().parent.parent / "xiaozhi-esp32-server" / "main" / "xiaozhi-server"
sys.path.insert(0, str(SERVER_ROOT))
os.chdir(SERVER_ROOT)

from config.settings import load_config
from core.utils.llm import create_instance as create_llm_instance


def main() -> None:
    config = load_config()
    selected = config.get("selected_module", {}).get("LLM")
    if not selected:
        print("ERROR: No LLM selected in selected_module.LLM")
        sys.exit(1)

    llm_config = config.get("LLM", {}).get(selected)
    if not llm_config:
        print(f"ERROR: No config block for LLM.{selected}")
        sys.exit(1)

    api_key = llm_config.get("api_key", "")
    if not api_key or "YOUR" in api_key.upper() or "PASTE" in api_key.upper():
        print(f"ERROR: Set a valid API key for LLM.{selected} in data/.config.yaml")
        sys.exit(1)

    print(f"Testing LLM: {selected}")
    print(f"Model: {llm_config.get('model_name', '(default)')}")
    print("Sending: Hello! Please reply in one short English sentence.")
    print("-" * 50)

    llm_type = llm_config.get("type", selected)
    llm = create_llm_instance(llm_type, llm_config)
    dialogue = [{"role": "user", "content": "Hello! Please reply in one short English sentence."}]
    chunks = []
    for part in llm.response("test", dialogue):
        if isinstance(part, str):
            chunks.append(part)
        elif isinstance(part, tuple) and part[0]:
            chunks.append(part[0])

    reply = "".join(chunks).strip()
    if not reply:
        print("FAILED: Empty response from the LLM.")
        sys.exit(1)

    print("SUCCESS - Gemini replied:")
    print(reply)
    print("-" * 50)


if __name__ == "__main__":
    main()
