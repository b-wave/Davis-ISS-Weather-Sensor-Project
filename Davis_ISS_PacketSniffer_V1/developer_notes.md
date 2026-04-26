# Developer Notes  
**Davis ISS Packet Sniffer — Internal Engineering Notes**  
**REV: 042526 — VERS: 1.0**

These notes capture subtle engineering traps encountered during development.  
They exist to save future developers (and Future‑Steve) from rediscovering them.

---

## 1. Davis ISS CRC Gotcha (Critical)

The ISS packet is 8 bytes total, but CRC is computed over **only the first 6 bytes**.

Correct:

```cpp
uint16_t calc = radio.crc16_ccitt(packet, 6);
uint16_t recv = (packet[6] << 8) | packet[7];
bool davisCrcOk = (calc == recv) && (calc != 0)
```
## 2. Arduino Sketch Folder Gotcha
#Arduino requires:

I used sketch folder per project
- The main file must match the folder name
- The .ino files are auto‑merged at compile time
- The contains all libraries used so IDE does not try using arduino/library/

**This means:**

- You cannot rename .ino files arbitrarily
- You cannot nest .ino files in subfolders
- Arduino will silently reorder tabs
-- I had a lot of RFM69 versions from various sources -  this protected the sketch folder

## 3. GitHub Sidebar + .ino Visibility Trick
GitHub sometimes hides .ino files in the sidebar.

**Workaround**
- Rename: MySketch.ino → MySketch.ino.txt
- Upload the .txt file
- Rename back: MySketch.ino.txt → MySketch.ino

**Side effect*
- Arduino IDE will place the .ino tab at the end
- No functional impact

Edge Sidebar Metadata Paste Corruption (Important)
When pasting code from Arduino IDE into Copilot chat, the Edge sidebar may inject metadata like:

```cpp
# User's Edge browser tabs metadata...
edge_all_open_tabs = [
   ...
]
```
- This is not in your .ino file.
  - It is injected by the Edge sidebar clipboard handler.
- Symptoms
  - Copilot sees “extra code”
    - GitHub shows unexpected Markdown headers
      - Pasted code doesn’t match the real file

- Why it happens
  -  Edge sidebar attaches browsing context to help Copilot
  - Arduino IDE has no RAW view
  - The metadata becomes part of the clipboard

- Safe workaround
  - Use the .ino → .txt → .ino trick when sharing code.
