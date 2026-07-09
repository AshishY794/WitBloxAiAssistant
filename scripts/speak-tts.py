"""
Interactive TTS: type your own text and pick a voice (English, Hindi, and more).
Uses Microsoft Edge TTS (free, needs internet).

Usage (Anaconda Prompt):
  scripts\\speak-tts.bat
"""

import asyncio
import os
import sys
import uuid
from datetime import date
from pathlib import Path

import edge_tts

# Popular WitBlox voices (Edge TTS)
CURATED_VOICES = [
    ("en-US-JennyNeural", "English (US)", "Female"),
    ("en-US-AriaNeural", "English (US)", "Female"),
    ("en-GB-SoniaNeural", "English (UK)", "Female"),
    ("en-IN-NeerjaNeural", "English (India)", "Female"),
    ("en-US-GuyNeural", "English (US)", "Male"),
    ("hi-IN-SwaraNeural", "Hindi (India)", "Female"),
    ("hi-IN-MadhurNeural", "Hindi (India)", "Male"),
    ("zh-CN-XiaoxiaoNeural", "Chinese", "Female"),
]

OUTPUT_DIR = (
    Path(__file__).resolve().parent.parent
    / "xiaozhi-esp32-server"
    / "main"
    / "xiaozhi-server"
    / "tmp"
)


def play_file(path: Path) -> None:
    if sys.platform == "win32":
        os.startfile(str(path))  # noqa: S606
    else:
        import subprocess

        subprocess.run(["xdg-open", str(path)], check=False)


async def fetch_all_voices():
    voices = await edge_tts.list_voices()
    return sorted(voices, key=lambda v: (v["Locale"], v["ShortName"]))


def print_curated_menu() -> None:
    print("\n--- Popular voices ---")
    for i, (name, lang, gender) in enumerate(CURATED_VOICES, 1):
        print(f"  {i:2}. {name}  ({lang}, {gender})")
    print(f"  {len(CURATED_VOICES) + 1:2}. Browse ALL Edge voices (long list)")
    print("   0. Exit")


async def print_all_voices(filter_lang: str | None = None) -> list[dict]:
    all_v = await fetch_all_voices()
    if filter_lang:
        prefix = {"english": "en", "hindi": "hi", "chinese": "zh"}.get(filter_lang.lower())
        if prefix:
            all_v = [v for v in all_v if v["Locale"].lower().startswith(prefix)]
    print(f"\n--- All voices ({len(all_v)} shown) ---")
    for i, v in enumerate(all_v, 1):
        print(
            f"  {i:3}. {v['ShortName']:28} "
            f"{v['Locale']:10} {v['Gender']}"
        )
    return all_v


async def synthesize(voice: str, text: str) -> Path:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUTPUT_DIR / f"speak-{date.today()}@{uuid.uuid4().hex}.mp3"
    communicate = edge_tts.Communicate(text, voice=voice)
    await communicate.save(str(out))
    return out


async def pick_from_all() -> str | None:
    print("\nFilter: [1] English  [2] Hindi  [3] Chinese  [4] Show everything")
    f = input("Filter (Enter = everything): ").strip()
    filt = None
    if f == "1":
        filt = "english"
    elif f == "2":
        filt = "hindi"
    elif f == "3":
        filt = "chinese"
    voices = await print_all_voices(filt)
    if not voices:
        print("No voices found.")
        return None
    choice = input("\nPick a voice number (0 = cancel): ").strip()
    if choice == "0":
        return None
    try:
        n = int(choice)
        if 1 <= n <= len(voices):
            return voices[n - 1]["ShortName"]
    except ValueError:
        pass
    print("Invalid number.")
    return await pick_from_all()


async def main_loop() -> None:
    print("=" * 50)
    print("  WitBlox Interactive TTS")
    print("  Type text -> pick voice -> hear speech")
    print("  Needs internet (Microsoft Edge TTS)")
    print("=" * 50)

    while True:
        print_curated_menu()
        choice = input("\nPick a voice number: ").strip()
        if choice == "0":
            print("Goodbye.")
            break
        try:
            n = int(choice)
        except ValueError:
            print("Invalid number. Try again.")
            continue

        voice = None
        if 1 <= n <= len(CURATED_VOICES):
            voice = CURATED_VOICES[n - 1][0]
        elif n == len(CURATED_VOICES) + 1:
            voice = await pick_from_all()
        else:
            print("Invalid number. Try again.")
            continue

        if not voice:
            continue

        print(f"\nSelected voice: {voice}")
        text = input("Enter text to speak (Enter = default): ").strip()
        if not text:
            text = "Hello! I am WitBlox AI. Nice to meet you."

        print("Generating audio...")
        try:
            path = await synthesize(voice, text)
        except Exception as exc:
            print(f"FAILED: {exc}")
            continue

        if not path.exists() or path.stat().st_size == 0:
            print("FAILED: empty audio file.")
            continue

        print(f"SUCCESS - saved: {path}")
        print("Playing...")
        play_file(path)

        again = input("\nSpeak again? [Y/n]: ").strip().lower()
        if again == "n":
            print("Done.")
            break


if __name__ == "__main__":
    try:
        asyncio.run(main_loop())
    except KeyboardInterrupt:
        print("\nStopped.")
