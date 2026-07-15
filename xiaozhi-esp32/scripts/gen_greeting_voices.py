#!/usr/bin/env python3
"""Generate EN/HI male/female greeting OGG (Opus) files for local presence greet."""

import asyncio
import subprocess
import sys
from pathlib import Path

try:
    import edge_tts
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "edge-tts", "-q"])
    import edge_tts

OUT_DIR = Path(__file__).resolve().parents[1] / "main" / "assets" / "common"

# Voice matrix requested by product:
# - English: male + female
# - Hindi: male + female
# Default playback in firmware = Hindi male
VOICES = [
    ("greeting_en_female", "en-US-JennyNeural", "Hi, how can I help you today?"),
    ("greeting_en_male", "en-US-GuyNeural", "Hi, how can I help you today?"),
    ("greeting_hi_female", "hi-IN-SwaraNeural", "नमस्ते, आज मैं आपकी कैसे मदद कर सकती हूँ?"),
    ("greeting_hi_male", "hi-IN-MadhurNeural", "नमस्ते, आज मैं आपकी कैसे मदद कर सकता हूँ?"),
]


async def synthesize(name: str, voice: str, text: str, tmp_mp3: Path) -> None:
    communicate = edge_tts.Communicate(text, voice)
    await communicate.save(str(tmp_mp3))


def to_ogg(mp3: Path, ogg: Path) -> None:
    subprocess.check_call(
        [
            "ffmpeg",
            "-y",
            "-i",
            str(mp3),
            "-c:a",
            "libopus",
            "-b:a",
            "24k",
            "-ar",
            "16000",
            "-ac",
            "1",
            str(ogg),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


async def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, voice, text in VOICES:
        mp3 = OUT_DIR / f"{name}.mp3"
        ogg = OUT_DIR / f"{name}.ogg"
        print(f"Generating {name} ({voice})...")
        await synthesize(name, voice, text, mp3)
        to_ogg(mp3, ogg)
        mp3.unlink(missing_ok=True)
        print(f"  -> {ogg} ({ogg.stat().st_size} bytes)")

    print("Done. Firmware default playback = greeting_hi_male.ogg")


if __name__ == "__main__":
    asyncio.run(main())
