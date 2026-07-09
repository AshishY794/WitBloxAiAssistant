"""
Interactive STT (speech-to-text) test using FunASR / SenseVoiceSmall.
Record from microphone or use a WAV file.

Usage (Anaconda Prompt):
  scripts\\test-stt.bat

First run loads the model (~30-60 seconds). Needs ~2GB+ free RAM.
"""

import asyncio
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SERVER_ROOT = Path(__file__).resolve().parent.parent / "xiaozhi-esp32-server" / "main" / "xiaozhi-server"
sys.path.insert(0, str(SERVER_ROOT))
os.chdir(SERVER_ROOT)

from config.settings import load_config
from core.utils.asr import create_instance as create_asr_instance

SAMPLE_RATE = 16000
CHANNELS = 1
RECORD_SECONDS = 5
OUTPUT_DIR = SERVER_ROOT / "tmp"


def ensure_ffmpeg() -> str:
    exe = shutil.which("ffmpeg")
    if not exe:
        raise RuntimeError("ffmpeg not found. Run: conda install ffmpeg -y")
    return exe


def discover_microphones(ffmpeg: str) -> list[str]:
    proc = subprocess.run(
        [ffmpeg, "-list_devices", "true", "-f", "dshow", "-i", "dummy"],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    names = []
    for line in proc.stderr.splitlines():
        match = re.search(r'"([^"]+)"\s*\(audio\)', line, re.IGNORECASE)
        if match:
            names.append(match.group(1))
    return names


def list_microphones(ffmpeg: str) -> None:
    print("\nDetecting microphones (ffmpeg)...")
    names = discover_microphones(ffmpeg)
    if not names:
        print("No microphones found.")
        return
    for i, name in enumerate(names, 1):
        print(f"  {i}. {name}")


def record_wav(ffmpeg: str, seconds: int = RECORD_SECONDS) -> Path:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUTPUT_DIR / "stt-record.wav"
    mics = discover_microphones(ffmpeg)
    default_mic = mics[0] if mics else "Microphone"
    if mics:
        print("\nAvailable microphones:")
        for i, name in enumerate(mics, 1):
            print(f"  {i}. {name}")

    prompt = f'\nMicrophone (Enter = "{default_mic}"): '
    device = input(prompt).strip()
    if not device:
        device = default_mic
    elif device.isdigit() and mics:
        idx = int(device)
        if 1 <= idx <= len(mics):
            device = mics[idx - 1]
    audio_in = f"audio={device}"

    print(f"\nRecording {seconds} seconds... SPEAK NOW!")
    cmd = [
        ffmpeg,
        "-y",
        "-f",
        "dshow",
        "-i",
        audio_in,
        "-ar",
        str(SAMPLE_RATE),
        "-ac",
        str(CHANNELS),
        "-t",
        str(seconds),
        str(out),
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if proc.returncode != 0:
        print(proc.stderr)
        raise RuntimeError(
            "Recording failed. Run option 3 to list microphones, "
            "then type the exact name in quotes if it has spaces."
        )
    return out


def wav_to_pcm(wav_path: Path, ffmpeg: str) -> bytes:
    cmd = [
        ffmpeg,
        "-i",
        str(wav_path),
        "-ar",
        str(SAMPLE_RATE),
        "-ac",
        str(CHANNELS),
        "-f",
        "s16le",
        "pipe:1",
    ]
    proc = subprocess.run(cmd, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError(f"Could not read audio: {proc.stderr.decode(errors='replace')}")
    return proc.stdout


def format_result(text) -> str:
    if text is None:
        return ""
    if isinstance(text, dict):
        parts = []
        if text.get("language"):
            parts.append(f"Language: {text['language']}")
        if text.get("emotion"):
            parts.append(f"Emotion: {text['emotion']}")
        if text.get("content"):
            parts.append(f"Text: {text['content']}")
        return "\n".join(parts) if parts else str(text)
    return str(text)


async def transcribe(asr, pcm: bytes) -> str:
    if len(pcm) < 1600:
        raise RuntimeError("Audio too short. Speak louder or check the microphone.")
    text, _ = await asr.speech_to_text_wrapper([pcm], "stt-test", "pcm")
    return format_result(text)


async def load_asr():
    config = load_config()
    selected = config.get("selected_module", {}).get("ASR", "FunASR")
    asr_config = config.get("ASR", {}).get(selected)
    if not asr_config:
        raise RuntimeError(f"No ASR config for {selected} in config files.")
    asr_type = asr_config.get("type", "fun_local")
    print(f"\nLoading STT model ({selected}, type={asr_type})...")
    print("This can take 30-60 seconds on first run. Please wait.")
    asr = create_asr_instance(asr_type, asr_config, delete_audio_file=True)
    asr.audio_format = "pcm"
    print("Model loaded.\n")
    return asr


async def main_loop() -> None:
    print("=" * 50)
    print("  WitBlox STT Test (Speech to Text)")
    print("  Engine: FunASR / SenseVoiceSmall (local)")
    print("=" * 50)

    ffmpeg = ensure_ffmpeg()
    asr = await load_asr()

    while True:
        print("\nOptions:")
        print("  1. Record from microphone (5 seconds)")
        print("  2. Transcribe a WAV file")
        print("  3. List microphones")
        print("  0. Exit")
        choice = input("\nChoice: ").strip()

        if choice == "0":
            print("Done.")
            break
        if choice == "3":
            list_microphones(ffmpeg)
            continue
        if choice not in ("1", "2"):
            print("Invalid choice.")
            continue

        try:
            if choice == "1":
                wav_path = record_wav(ffmpeg)
            else:
                path_str = input("Path to WAV file: ").strip().strip('"')
                wav_path = Path(path_str)
                if not wav_path.exists():
                    print("File not found.")
                    continue

            print("Transcribing...")
            pcm = wav_to_pcm(wav_path, ffmpeg)
            result = await transcribe(asr, pcm)
            print("-" * 50)
            if result.strip():
                print("SUCCESS - Recognition result:")
                print(result)
            else:
                print("No speech detected. Try again and speak clearly.")
            print("-" * 50)
        except Exception as exc:
            print(f"FAILED: {exc}")

        again = input("\nTest again? [Y/n]: ").strip().lower()
        if again == "n":
            break


if __name__ == "__main__":
    try:
        asyncio.run(main_loop())
    except KeyboardInterrupt:
        print("\nStopped.")
