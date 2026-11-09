#!/usr/bin/env python3
"""Record OpenOSINT web demo GIF — 1920x1080, 120 frames @ 150ms."""
import asyncio
import io
from pathlib import Path

from PIL import Image
from playwright.async_api import async_playwright

REPO_ROOT = Path(__file__).parent.parent
MOCK_HTML = REPO_ROOT / "scripts" / "web_demo_mock.html"
OUTPUT_GIF = REPO_ROOT / "assets" / "web-demo.gif"

FRAMES = 120
INTERVAL_MS = 150


async def record() -> None:
    print(f"[*] Loading: {MOCK_HTML}")
    print(f"[*] Output:  {OUTPUT_GIF}")
    print(f"[*] Frames:  {FRAMES} × {INTERVAL_MS}ms = {FRAMES * INTERVAL_MS / 1000}s")

    async with async_playwright() as p:
        browser = await p.chromium.launch(headless=True)
        page = await browser.new_page(viewport={"width": 1920, "height": 1080})

        url = MOCK_HTML.resolve().as_uri()
        await page.goto(url)
        # Let the page settle before starting capture
        await page.wait_for_timeout(300)

        frames: list[bytes] = []
        for i in range(FRAMES):
            await page.wait_for_timeout(INTERVAL_MS)
            data = await page.screenshot(type="png")
            frames.append(data)
            if (i + 1) % 20 == 0:
                print(f"  captured frame {i + 1}/{FRAMES}")

        await browser.close()

    print("[*] Assembling GIF…")
    images = [Image.open(io.BytesIO(f)).convert("RGB") for f in frames]

    # Stamp a unique 2×2 marker at the bottom-right corner of every frame so
    # the GIF encoder cannot merge consecutive identical-looking frames.
    # The corner is always on the dark #0d1117 background (outside chat content),
    # so the per-frame blue-channel increment (1 unit) is imperceptible.
    for i, img in enumerate(images):
        px = img.load()
        blue = min(23 + i, 255)  # base blue from #0d1117; increment by 1 each frame
        for dy in range(2):
            for dx in range(2):
                r, g, _ = px[1918 + dx, 1078 + dy]
                px[1918 + dx, 1078 + dy] = (r, g, blue)

    # Quantize each frame independently to avoid palette-collapse artefacts
    quantized = [
        img.quantize(colors=256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
        for img in images
    ]

    quantized[0].save(
        str(OUTPUT_GIF),
        save_all=True,
        append_images=quantized[1:],
        loop=0,
        duration=INTERVAL_MS,
        optimize=False,
    )

    with Image.open(str(OUTPUT_GIF)) as verify:
        print(f"[+] Saved: {OUTPUT_GIF}")
        print(f"[+] Size:   {verify.size}")
        print(f"[+] Frames: {verify.n_frames}")


if __name__ == "__main__":
    asyncio.run(record())
