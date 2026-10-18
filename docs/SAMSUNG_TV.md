# Samsung TV power control

insomniaTV switches a Samsung Tizen TV off over the network, so no IR hardware is needed.

## How it works

- The TV's REST API (`http://<tv>:8001/api/v2/`) reports `device.PowerState` (`on` / `standby`).
- The remote-control WebSocket (`wss://<tv>:8002/api/v2/channels/samsung.remote.control`) accepts key presses. `KEY_POWER` toggles, so the firmware sends it only while `PowerState` is `on`. `KEY_POWEROFF` left `PowerState` at `on` on the AU7002 tested.
- The first connection shows an allow prompt on the TV and returns a token. The token is stored in `/config/insomnia_tv.json` under `tv_control` and reused for later connections.
- In deep standby the TV drops off the network entirely, so it can't be powered on from here.

## Pairing

Web UI: **Sensors → Add Smart TV**, then **Pair for power control** on the last page. Or **Config → TV Power Control**: enter the TV IP, click **Pair**, and accept the "insomniaTV" prompt on the TV within 30 seconds.

## API

| Route | Purpose |
|-------|---------|
| `GET /api/tv/control` | `host`, `mac`, `paired`, `pair_state` (`idle`/`waiting`/`paired`/`failed`), `error` |
| `POST /api/tv/pair` `{"host":"<ip>"}` | Starts pairing (202); poll `GET /api/tv/control` |
| `POST /api/tv/power-off` | Manual test of the network power-off (202) |

## Sleep cycle

When a TV is paired, the sleep state machine's power-off step uses this path; otherwise it falls back to the IR power code. Add a `samsung_tv` sensor so the fusion sees standby (ping/http keep answering briefly after the screen is off).
