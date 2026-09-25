# Third-party audio ledger

All sounds in this file are released under **CC0** (Creative Commons Zero
1.0 Universal, https://creativecommons.org/publicdomain/zero/1.0/) public
domain dedications. This file serves as the license record for all
third-party audio in `content/` — every shipped sound file is listed below
with its upstream source, license, and checksum.

## Sound files

| Destination path (in content/) | Upstream filename | Source pack + URL | License | sha256 (shipped .wav) | Date |
|---|---|---|---|---|---|
| Agriculture.c4d/Structures.c4d/Windmill.c4d/MillGrind.wav | Audio/creak2.ogg | Kenney RPG Audio pack (`kenney_rpg-audio.zip`), https://kenney.nl/assets/rpg-audio | CC0 | e9fdc51c5d1321fdf358c6835801ea8bd38a3d196546b35a9f85d04e4881a798 | 2026-09-22 |
| Agriculture.c4d/Vegetation.c4d/Wheat.c4d/HarvestChop.wav | Audio/chop.ogg | Kenney RPG Audio pack (`kenney_rpg-audio.zip`), https://kenney.nl/assets/rpg-audio | CC0 | d2eed1e0d6d313a0e3ff4dd6087fad96694e5f5f81bcff1f994b1fd4e61f334b | 2026-09-22 |
| Agriculture.c4d/Tools.c4d/FishTrap.c4d/FishCatch.wav | splash_01.ogg | "40 CC0 water / splash / slime SFX" by rubberduck (`water-splash-slime-sfx.zip`), https://opengameart.org/content/40-cc0-water-splash-slime-sfx | CC0 | aec614804e8d13c683e73198c9f4887e5c1c44a01148b6a24201ba899664ec22 | 2026-09-22 |
| Missions.c4f/FirstLight.c4s/Goal_Homestead.c4d/MarketBell.wav | Audio/impactBell_heavy_001.ogg | Kenney Impact Sounds pack (`kenney_impact-sounds.zip`), https://kenney.nl/assets/impact-sounds | CC0 | 47dc9c41d8e0c92ee0a272a6a2c2ea3ff4fc018340c8b2de42f8c45ccf294de9 | 2026-09-22 |
| Missions.c4f/FirstLight.c4s/Goal_Homestead.c4d/LedgerChime.wav | Audio/confirmation_001.ogg | Kenney Interface Sounds pack (`kenney_interface-sounds.zip`), https://kenney.nl/assets/interface-sounds | CC0 | 45df1829f45b81890562a019e316623781c6cf6ef53b272ef9d5991885ba9e2f | 2026-09-22 |
| ArenaChampions.c4f/Objects.c4d/KillTarget.c4d/KillPing.wav | Audio/tick_001.ogg | Kenney Interface Sounds pack (`kenney_interface-sounds.zip`), https://kenney.nl/assets/interface-sounds | CC0 | d2028308a9f9e9bf63d7aa4d3c90a7f6ff0d7d141f85012785846248b9f214a4 | 2026-09-25 |
| ArenaChampions.c4f/Objects.c4d/Respawner.c4d/RespawnChime.wav | Audio/pluck_001.ogg | Kenney Interface Sounds pack (`kenney_interface-sounds.zip`), https://kenney.nl/assets/interface-sounds | CC0 | 81a4b05a72cede822fe26b1e57b110ecf3ee578e9e29d03aab1e53bc5955e9ee | 2026-09-25 |
| ArenaChampions.c4f/Objects.c4d/TeamChest.c4d/ChestOpen.wav | Audio/handleCoins.ogg | Kenney RPG Audio pack (`kenney_rpg-audio.zip`), https://kenney.nl/assets/rpg-audio | CC0 | 1e41e1566b43fbb9e80fa974de52e96840f3d047430ada1298cf3b41afe2180f | 2026-09-25 |
| ArenaChampions.c4f/Objects.c4d/Boundary.c4d/TideAlert.wav | Audio/bong_001.ogg | Kenney Interface Sounds pack (`kenney_interface-sounds.zip`), https://kenney.nl/assets/interface-sounds | CC0 | f065fefe2483ebc362eeba926488a2e24b85ce4331999cdc1d41beaeea49acde | 2026-09-25 |
| ArenaChampions.c4f/Objects.c4d/KillTarget.c4d/VictorySting.wav | Audio/confirmation_001.ogg | Kenney Interface Sounds pack (`kenney_interface-sounds.zip`), https://kenney.nl/assets/interface-sounds | CC0 | 45df1829f45b81890562a019e316623781c6cf6ef53b272ef9d5991885ba9e2f | 2026-09-25 |

License notes:

- Kenney asset packs state "CC0 licensed!" / "Creative Commons CC0" on every
  asset page (e.g. https://kenney.nl/assets/rpg-audio).
- The OpenGameArt pack page lists license **CC0**
  (http://creativecommons.org/publicdomain/zero/1.0/) under "License(s)".

Conversion: every file was converted to **WAV PCM 16-bit mono 22050 Hz**
(classic Clonk sound spec) and peak-normalized to a common peak of
**-1.5 dBFS** so no sound blasts relative to the others.

Exact ffmpeg commands (two steps per file, the `volumedetect` peak measured
on the plain resample, then a linear gain so the final peak is -1.5 dBFS):

```
# Step 1 — decode upstream .ogg, resample to mono 22050 Hz 16-bit PCM:
ffmpeg -y -i <upstream>.ogg -ac 1 -ar 22050 -sample_fmt s16 <name>.tmp.wav

# Step 2 — measure the peak:
ffmpeg -i <name>.tmp.wav -af volumedetect -f null -
#   MillGrind:       max_volume = -8.6 dB
#   HarvestChop:     max_volume = -0.3 dB
#   FishCatch:       max_volume = -0.2 dB
#   MarketBell:      max_volume = -1.2 dB
#   LedgerChime:     max_volume = -0.9 dB
#   KillPing:        max_volume = -4.2 dB   (2026-09-25 arena batch)
#   RespawnChime:    max_volume = -2.2 dB
#   ChestOpen:       max_volume = -1.4 dB
#   TideAlert:       max_volume = -0.9 dB
#   VictorySting:    max_volume = -0.9 dB

# Step 3 — apply gain so the final peak lands on -1.5 dBFS
#   (gain = -1.5 dB - measured peak):
ffmpeg -y -i <name>.tmp.wav -af "volume=+7.1dB" <name>.wav   # MillGrind
ffmpeg -y -i <name>.tmp.wav -af "volume=-1.2dB" <name>.wav   # HarvestChop
ffmpeg -y -i <name>.tmp.wav -af "volume=-1.3dB" <name>.wav   # FishCatch
ffmpeg -y -i <name>.tmp.wav -af "volume=-0.3dB" <name>.wav   # MarketBell
ffmpeg -y -i <name>.tmp.wav -af "volume=-0.6dB" <name>.wav   # LedgerChime
ffmpeg -y -i <name>.tmp.wav -af "volume=+2.7dB" <name>.wav   # KillPing
ffmpeg -y -i <name>.tmp.wav -af "volume=+0.7dB" <name>.wav   # RespawnChime
ffmpeg -y -i <name>.tmp.wav -af "volume=-0.1dB" <name>.wav   # ChestOpen
ffmpeg -y -i <name>.tmp.wav -af "volume=-0.6dB" <name>.wav   # TideAlert
ffmpeg -y -i <name>.tmp.wav -af "volume=-0.6dB" <name>.wav   # VictorySting
```

Verified output format (ffprobe, all files):
`codec_name=pcm_s16le, sample_rate=22050, channels=1`, peak = -1.5 dB.

Upstream downloads:

- `kenney_rpg-audio.zip`: https://kenney.nl/media/pages/assets/rpg-audio/8e99002d76-1677590336/kenney_rpg-audio.zip
- `kenney_impact-sounds.zip`: https://kenney.nl/media/pages/assets/impact-sounds/87b4ddecda-1677589768/kenney_impact-sounds.zip
- `kenney_interface-sounds.zip`: https://kenney.nl/media/pages/assets/interface-sounds/fa43c1dd4d-1677589452/kenney_interface-sounds.zip
- `water-splash-slime-sfx.zip`: https://opengameart.org/sites/default/files/water-splash-slime-sfx.zip
