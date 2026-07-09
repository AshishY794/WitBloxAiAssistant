"""English-only TTS test for WitBlox (uses data/.config.yaml, EdgeTTS by default)."""

import asyncio
import os
import subprocess
import sys
import threading
from pathlib import Path

SERVER_ROOT = Path(__file__).resolve().parent.parent / "xiaozhi-esp32-server" / "main" / "xiaozhi-server"
sys.path.insert(0, str(SERVER_ROOT))
os.chdir(SERVER_ROOT)

from config.settings import load_config
from core.utils.tts import create_instance as create_tts_instance


class MockConn:
    sample_rate = 16000
    audio_format = "pcm"
    stop_event = threading.Event()
    client_abort = False
    headers = {}


async def run_test() -> int:
    config = load_config()
    selected = config.get("selected_module", {}).get("TTS")
    if not selected:
        print("ERROR: No TTS selected in selected_module.TTS")
        return 1

    tts_config = config.get("TTS", {}).get(selected)
    if not tts_config:
        print(f"ERROR: No config block for TTS.{selected}")
        return 1

    tts_type = tts_config.get("type", selected)
    test_text = (
        "Hello! I am WitBlox AI. "
        "If you can hear this, text to speech is working."
    )

    print(f"Testing TTS: {selected} (type: {tts_type})")
    print(f"Voice: {tts_config.get('voice', '(default)')}")
    print(f"Text: {test_text}")
    print("-" * 50)

    tts = create_tts_instance(tts_type, tts_config, delete_audio_file=False)
    tts.conn = MockConn()

    out_file = tts.generate_filename()
    await tts.text_to_speak(test_text, out_file)

    if not out_file or not os.path.exists(out_file) or os.path.getsize(out_file) == 0:
        print("FAILED: No audio file was created.")
        return 1

    abs_path = os.path.abspath(out_file)
    print("SUCCESS - Audio file created:")
    print(abs_path)
    print("-" * 50)
    print("Opening audio in your default player...")

    if sys.platform == "win32":
        os.startfile(abs_path)  # noqa: S606
    else:
        subprocess.run(["xdg-open", abs_path], check=False)

    return 0


def main() -> None:
    code = asyncio.run(run_test())
    if code != 0:
        sys.exit(code)
    print("TTS test PASSED.")


if __name__ == "__main__":
    main()
