# save_vtr

ESPHome component for the **Systemair SAVE VTR** ventilation unit over Modbus
(RS-485). Exposes the unit as a Home Assistant climate entity plus a full set
of telemetry, alarm and settings entities.

Requires **ESPHome 2026.9.0 or newer** (it is built on
`modbus_controller::ControllerDevice`, which replaced the deprecated
`ModbusCommandItem` API).

## Home Assistant

- `climate` entity with current temperature (supply air), current humidity
  (when the PDM RHS sensor register returns a value), target temperature and
  the unit's user modes as **custom fan modes** (`AUTO-VENT`, `MANUAL`,
  `CROWDED`, `REFRESH`, `FIREPLACE`, `AWAY`, `HOLIDAY`, `COOKERHOOD`).
- `climate` action is derived on-device from the heat demand register and the
  airflow percentages (`heating` / `fan` / `idle`).
- All sensors carry proper `device_class` / `state_class` /
  `entity_category`, so long-term statistics and device pages in Home
  Assistant work out of the box.

## Polling model

| Register group | Cadence |
| --- | --- |
| Temperatures, setpoint, user mode, heat demand, airflow, RPM, output utilisation | `update_interval` (climate platform, default `10s`) |
| Summary alarm flags (filter alarm, output alarm, alarm type A/B/C) | `update_interval` |
| Detail alarm registers (32 x 0-3 states) + filter warning counter | `alarm_update_interval` (default `300s`) |

The two-cadence split answers "poll alarms less frequently than the things
that are in the UI": the 32 detail alarm registers only cost bus time every
`alarm_update_interval`, while the 5 summary flags ride along with the fast
poll at no extra cost. **When a summary flag changes, the component
immediately rescans the detail registers** (rate-limited to 30 s), so a new
alarm is fully described within one update cycle instead of waiting for the
slow interval.

Reads are batched per contiguous register range and every read completes
asynchronously (response / exception / timeout / drop), so values publish as
they arrive - there is no snapshot timer and no stale-publish window. A new
poll cycle is never stacked on one still in flight.

## Alarm correlation (backend logic)

- `active_alarm_count` (sensor): number of active alarm flags.
- `any_alarm_active` (binary sensor): true when any summary flag or detail
  register is active.
- `filter_warning_active` (binary sensor): filter warning detail register or
  filter alarm flag active.
- `system_status` (text sensor): `OK` / `Warning` (waiting or cleared-error
  states present) / `Alarm` / `Unknown` (before first scan).
- `alarm_summary` (text sensor): comma-separated names of the active alarms,
  e.g. `Filter, EAF RPM` - or `None`.
- The 32 individual alarm text sensors only publish on **state change**,
  keeping MQTT/API/history quiet when nothing happens.

## Register map

Register classification follows the addresses already committed in this
repository. **Only the registers in "Verified" have been seen working on a
real unit** (via this repo's history); the "Extension" group follows the
Systemair SAVE Modbus specification and is **not yet verified against a
physical VTR300** - check against your unit before relying on them.

| Register | Type | Entity | Status |
| --- | --- | --- | --- |
| 1160 | input | current user mode | Verified |
| 1161 | holding | user mode request | Verified (see note) |
| 2000 | holding | target temperature (°C x 10) | Verified |
| 12101 | holding | outdoor air temperature | Verified |
| 12102 | holding | supply air temperature | Verified |
| 12135 | holding | PDM RHS sensor (%) | Verified (PR #5) |
| 12543 | holding | extract air temperature | Verified |
| 2148 | input | heat demand (%) | Verified |
| 12400/12401 | input | supply/extract fan RPM | Verified |
| 14000/14001 | input | supply/extract airflow (%) | Verified |
| 14102 | input | heat exchanger utilisation (%) | Verified (PR #5) |
| 7006 | input | filter alarm was detected | Verified (PR #5) |
| 14002 | input | output alarm | Verified (PR #5) |
| 15001-15543 (32 regs) | input | detail alarm states (0-3) | Verified (PR #5) |
| 15548 | input | filter warning counter | Verified (PR #5) |
| 15900-15902 | input | alarm type A/B/C | Verified (PR #5) |
| 1100-1104 | holding | user mode durations | **Extension - unverified** |
| 2505 | holding | ECO function on/off | **Extension - unverified** |
| 3001 | holding | moisture transfer on/off | **Extension - unverified** |
| 7000 | holding | filter replacement period (months) | **Extension - unverified** |
| 14100/14101 | input | heater/cooler utilisation (Y0/Y1) | **Extension - unverified** |

`thermal_efficiency` is computed locally from the three temperatures
(`eta = (T_supply - T_outdoor) / (T_extract - T_outdoor)`), it reads no
register.

### Note on user mode values

Register 1160 decodes as `0=AUTO-VENT ... 7=COOKERHOOD` (as before). The
write path now uses the **same** value table for 1161; the previous code wrote
a `+1`-shifted value (requesting `AUTO-VENT` sent `1`, which the read side
decodes as `MANUAL`). `COOKERHOOD` (7) is treated as read-only. Please verify
a mode round-trip (`AUTO-VENT` -> readback) on a real unit after upgrading.

## Setup

The quickest setup is the ready-made package in
[`packages/save_vtr`](../../packages/save_vtr/README.md): one `packages:`
entry gives you the bus, climate and every entity. Manual setup:

```yaml
external_components:
  - source: github://atleso/esphome
    components: [save_vtr]

uart:
  id: mb_uart
  tx_pin: GPIO16
  rx_pin: GPIO17
  baud_rate: 115200

modbus:
  id: mb1
  uart_id: mb_uart

modbus_controller:
  - id: save_modbus
    modbus_id: mb1
    address: 5

climate:
  - platform: save_vtr
    name: "SAVE Climate"
    modbus_id: save_modbus
    update_interval: 10s        # UI values
    alarm_update_interval: 300s # alarm detail scan
    max_airflow: 300            # m3/h at 100%, VTR300 default

sensor:
  - platform: save_vtr
    heat_demand:
      name: "Heat Demand"
    ...
```

See `config_examples/save_vtr/save_vtr.yml` for the full example.