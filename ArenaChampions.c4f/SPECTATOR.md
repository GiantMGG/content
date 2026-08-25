# Spectator & Replay Support — Arena Champions

## What works today

### Live spectating via `/observe`

A tournament organiser can launch a dedicated server with the `/observe` flag to
join as a live spectator without consuming an active player slot:

```bash
clonk /observe /lobby:60 ArenaChampions.c4f/Frostlance.c4s
```

The `/observe` flag is parsed at `src/C4Game.cpp:2738` and sets `fObserve = true`
and `NetworkActive = true`. The observer-state getter/setter lives on
`C4ClientCore`:

- `src/C4Client.cpp:34` — `fObserver(false)` default constructor.
- `src/C4Client.h:58` — `isObserver()` getter.
- `src/C4Client.h:63` — `SetObserver(bool)` setter; `SetObserver(true)` clears
  `fActivated` so the observer client does not appear as an active player.

The observer sees the game world but has no crew and cannot issue commands.
Disconnecting the observer requires no game-state rollback — the observer is a
passive viewer.

### Watchable replays via `Film=1`

All five scenarios set `Film=1` in `Scenario.txt [Head]`. This enables
record-mode for watchable replays. The recorded replay view is locked to follow
the first player and controls are hidden during playback. The `Film` flag is
referenced alongside the `fObserve`/`fLobby` defaults at `src/C4Game.cpp:1514`.

Replay files are written to the player's replay directory after the match.
Replays are forward-compatible within a minor version.

---

## What is missing (the `replay-browser-scrubbing` gap)

The roadmap sentence "Includes spectator mode support" is satisfied honestly by
the existing `/observe` flag (live spectating without consuming an active slot)
and `Film=1` (watchable replays). However, the following are **not** in scope for
this pack and are explicitly deferred:

1. **Replay browser with timeline scrubbing.** A proper replay *browser* with
   timeline scrubbing, playback speed control, and free-camera mode touches
   `src/C4Record.*`, `src/C4Replay.*`, and `src/C4Gui.*`. This is the
   `replay-browser-scrubbing` roadmap item, currently **not started**.

2. **Polished in-game spectator UI overlay.** The existing `/observe` flag
   provides the slot-free join, but the UI around it is bare. A polished
   spectator overlay (team scores, player viewports, free-camera mode) is a
   separate concern.

---

## Recommended path forward

Ship `arena-champions-spectator-mode` as a follow-up roadmap item blocked on
`replay-browser-scrubbing`. In the meantime, tournament organisers should use
`/observe` for live spectating and `Film=1` replays for VOD review.

### Tournament organiser quick-start

1. **Live spectating:** Host the match with `clonk /lobby:60
   ArenaChampions.c4f/Frostlance.c4s`. Have spectators join with `clonk
   /observe /join:<host-ip>`. The observer joins without consuming an active
   slot.

2. **VOD review:** After the match, locate the replay file in the host's replay
   directory. Load the replay in the replay viewer — the view is locked to
   follow the first player and controls are hidden during playback.
