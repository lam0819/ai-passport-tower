# 摩天大廈 SKYRISE

An original SimTower-style tower management sim (homage — no Maxis / OPeNBooK assets or names) for the [FoloToy AI Passport](https://github.com/lam0819/ai-passport) (ESP32-C3, 320×240, 3 keys). Build offices, flats, hotels and Hong Kong-flavoured shops; run lifts; chase ★ ratings up to **TOWER**, then dig 100 basement floors for the **CORE** challenge.

## Play

- **Scroll** with UP / DOWN; **OK** opens the build / action menu.
- Place floors and facilities (shops by type, multi-floor cinema / coaster / parking / pool / …), dig basements, rush construction with in-game money.
- Speed shop: buy **2×** / **4×** clocks with tower money only (no real money).
- Two save slots + autosave; day/night music toggle; HK events (typhoon, black rain, Mid-Autumn, Christmas Santa, …) that never soft-lock you.
- Stairs still work if lifts lose power — pay to repair or wait it out.

## Build & flash (`ota_1`, next to the assistant)

```bash
. $IDF_PATH/export.sh                       # ESP-IDF v6
cp ../ai-passport-muse/esp32/dev_signing_key.pem .
idf.py set-target esp32c3 && idf.py build && idf.py size
tools/flash.sh -p /dev/cu.usbmodem* --boot  # app only — never idf.py flash
```

`tools/flash.sh` writes the signed app to `ota_1` at `0x400000` and optionally switches the boot slot. The assistant owns the bootloader, partition table and NVS — do not overwrite them with `idf.py flash`.

Save: NVS namespace `tower`, blobs `s0` / `s1` (two slots) + `slot` (last-used) + `snd` (sound). Legacy single `save` migrates to slot 0 once.

## Browser prototype

```bash
tools/build_proto.sh          # → prototype/index.html
tools/proto_test.sh           # headless Chrome checks
```

## Host harness (firmware C on PC)

```bash
tools/host/run.sh             # simulation + paint tests, screenshots → docs/
```

## Design

See [PLAN.md](PLAN.md) (Cantonese + English): floors, stars, underground challenge, events, memory budget.

## License

Apache-2.0. Original game; not affiliated with Maxis or Electronic Arts.
