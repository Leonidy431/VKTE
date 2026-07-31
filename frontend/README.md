# VKTE Frontend — Combiner-HUD (Path A)

Single-file, dependency-free HUD frontend. Implements Path A from
`docs/RESEARCH_HUD_GLASS_TOP3.md` (the 48-parameter-matrix winner): a
high-brightness display reflected off a combiner/windshield, rendered in
mirror mode so the reflection reads correctly to the driver.

## Run

No build step. Any static file server works:

```bash
cd frontend/src
python3 -m http.server 8080
# open http://<cm4-host>:8080/index.html on the projector's display
```

On first run it prompts for the backend URL and API key (stored in
`localStorage`); click the ⚙ button any time to change them.

## What it does

- Connects to `ws://<backend>/ws/telemetry` with automatic reconnect
  (exponential backoff, capped at 10s, jittered) — closes the Sprint 4
  backlog item "WebSocket reconnect logic".
- Renders land / marine / debug telemetry views on a full-screen canvas.
- `Mirror` toggle applies `transform: scaleX(-1)` to the render surface —
  flip this on when physically projecting onto a combiner/windshield, off
  when viewing the screen directly (bring-up/debug on a monitor).
- Button controls duplicated as both keyboard bindings (`M`, `+`/`-`, `B`,
  `Esc`) and large touch buttons, per the HMI research in
  `docs/RESEARCH_HUD_GLASS_TOP3.md` — physical/keyboard-first, touch is the
  parking-mode fallback, not the primary interaction.
- Talks to the existing `/api/v1/*` REST endpoints with the same Bearer
  auth the backend already enforces (see `backend/app/main.py`).

## Not yet done (see BACKLOG.md)

- Real physical button pad (USB/BT HID) — currently keyboard bindings only,
  a real pad just needs to emit the same key events.
- Kiosk-mode auto-launch on CM4 boot (systemd unit / chromium --kiosk).
- On-vehicle brightness/ghost-image validation (BACKLOG Фаза 6).
