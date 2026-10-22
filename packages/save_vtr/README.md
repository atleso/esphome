# save_vtr packages

Pull in the full VTR300 setup with a few lines of YAML, so you don't have to
list every entity yourself.

| File | Contents |
| --- | --- |
| `core.yaml` | `external_components`, UART, Modbus, Modbus controller, climate, all sensors, all binary sensors, `user_mode` / `system_status` / `alarm_summary` text sensors |
| `alarm_details.yaml` | the 32 per-alarm detail text sensors (optional) |
| `controls.yaml` | switch / number / select / button settings entities (optional, **registers not yet verified on a real unit**) |

## Usage

```yaml
esphome:
  name: vtr300
  friendly_name: vtr300

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
api:
  encryption:
    key: !secret api_key
ota:
  - platform: esphome

# network: wifi: or ethernet: as usual

packages:
  save_vtr:
    url: https://github.com/atleso/esphome
    ref: main
    refresh: 1d
    files:
      - path: packages/save_vtr/core.yaml
        vars:
          save_vtr_ref: main    # keep equal to ref above
          update_interval: 5s
      - packages/save_vtr/alarm_details.yaml   # optional
      # - packages/save_vtr/controls.yaml      # optional, unverified registers
```

## Variables (`core.yaml`)

| Variable | Default |
| --- | --- |
| `save_vtr_ref` | `main` (git ref used for the component itself) |
| `tx_pin` / `rx_pin` | `GPIO16` / `GPIO17` |
| `baud_rate` | `115200` |
| `modbus_address` | `5` |
| `update_interval` | `10s` (UI values) |
| `alarm_update_interval` | `300s` (alarm detail scan) |
| `max_airflow` | `300` (m³/h at 100 %) |
| `climate_name` | `SAVE Climate` |

The package defines the ids `save_vtr_uart`, `save_vtr_modbus` and
`save_vtr_controller`, so don't define your own `uart:` / `modbus:` for the
same bus. Entities you don't want can be disabled in Home Assistant. To leave
one out of the firmware completely, define it yourself instead of using that
package file, or copy the package locally and edit it.
