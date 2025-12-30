# Medallion ESPHome Firmware

ESPHome firmware for the Waveshare ESP32-S3-Touch-AMOLED-1.75-B "Medallion" voice recorder device.

## Features

- **OTA Updates**: Push firmware updates directly from Home Assistant
- **Home Assistant Integration**: Full integration with sensors, buttons, and services
- **Voice Recording**: Record audio to SD card via ES8311 codec
- **HTTP Upload**: Upload recordings to your backend API server
- **AMOLED Display**: 466x466 round AMOLED display support
- **Touch Input**: Capacitive touch via CST92xx controller
- **Power Management**: AXP2101 PMIC with battery monitoring

## Hardware

This firmware is designed for the Waveshare ESP32-S3-Touch-AMOLED-1.75-B which includes:

| Component | Details |
|-----------|---------|
| MCU | ESP32-S3 with PSRAM |
| Display | CO5300 466x466 QSPI AMOLED |
| Touch | CST92xx Capacitive Touch |
| PMIC | AXP2101 Power Management |
| Audio | ES8311 Codec with I2S |
| Storage | MicroSD Card (SPI) |
| IO Expander | TCA9554 (optional) |

## Installation

### Prerequisites

1. **ESPHome**: Install ESPHome on your system or use the Home Assistant ESPHome add-on
2. **Home Assistant**: For full integration and OTA updates
3. **USB-C Cable**: For initial flash

### First-Time Setup

1. **Copy the secrets template**:
   ```bash
   cd esphome
   cp secrets.yaml.example secrets.yaml
   ```

2. **Edit `secrets.yaml`** with your credentials:
   ```yaml
   wifi_ssid: "YourWiFiNetwork"
   wifi_password: "YourWiFiPassword"
   fallback_password: "medallion123"
   api_encryption_key: "YOUR_32_BYTE_BASE64_KEY"
   ota_password: "your_ota_password"
   web_password: "admin"
   upload_url: "http://192.168.1.119:8000/upload"
   ```

   Generate an encryption key with:
   ```bash
   esphome generate-key
   ```

3. **Initial Flash via USB**:
   ```bash
   cd esphome
   esphome run medallion.yaml
   ```

4. **Add to Home Assistant**:
   - Go to Settings → Devices & Services → Add Integration
   - Search for "ESPHome"
   - Enter the device IP address
   - Use the API encryption key when prompted

### OTA Updates

After initial setup, you can update the firmware directly from:
- **Home Assistant**: Navigate to ESPHome integration → device → Update
- **ESPHome Dashboard**: Click "Upload" on the device
- **Command Line**: `esphome run medallion.yaml` (will use OTA if device is online)

## Home Assistant Integration

### Entities

The device exposes these entities to Home Assistant:

**Sensors:**
- Battery Voltage
- Battery Level (%)
- VBUS Voltage
- WiFi Signal Strength
- Uptime

**Text Sensors:**
- IP Address
- Connected SSID
- ESPHome Version
- Recording Status

**Binary Sensors:**
- Recording State

**Buttons:**
- Start Recording
- Stop Recording
- Upload Recording
- Restart

**Numbers:**
- Display Brightness (0-255)

### Services

You can also call these services from automations:

```yaml
# Start recording
service: esphome.medallion_start_recording

# Stop recording
service: esphome.medallion_stop_recording

# Upload the last recording
service: esphome.medallion_upload_recording
```

### Example Automation

```yaml
automation:
  - alias: "Medallion: Upload after recording"
    trigger:
      - platform: state
        entity_id: binary_sensor.medallion_voice_recorder_recording
        from: "on"
        to: "off"
    action:
      - delay: "00:00:02"
      - service: esphome.medallion_upload_recording
```

## Custom Components

This firmware includes custom ESPHome components for the Waveshare hardware:

| Component | Purpose |
|-----------|---------|
| `axp2101` | AXP2101 PMIC power management |
| `co5300_qspi` | CO5300 QSPI AMOLED display |
| `es8311` | ES8311 audio codec with I2S |
| `cst92xx` | CST92xx capacitive touch |
| `medallion_voice` | Voice recording and upload logic |

These are located in the `custom_components/` directory and are automatically loaded.

## Pin Configuration

| Function | GPIO |
|----------|------|
| I2C SDA | GPIO15 |
| I2C SCL | GPIO14 |
| PMIC IRQ | GPIO11 |
| Display CS | GPIO12 |
| Display SCLK | GPIO38 |
| Display D0 | GPIO4 |
| Display D1 | GPIO5 |
| Display D2 | GPIO6 |
| Display D3 | GPIO7 |
| Display RST | GPIO39 |
| Touch INT | GPIO11 |
| Touch RST | GPIO40 |
| Audio MCLK | GPIO42 |
| Audio BCLK | GPIO9 |
| Audio WS | GPIO45 |
| Audio DOUT | GPIO10 |
| Audio DIN | GPIO8 |
| PA Enable | GPIO46 |
| SD CS | GPIO41 |
| SD CLK | GPIO2 |
| SD MOSI | GPIO1 |
| SD MISO | GPIO3 |

## Upload Server

The device uploads WAV files via HTTP multipart/form-data POST to your configured URL.

Expected endpoint:
```
POST /upload
Content-Type: multipart/form-data

file: <WAV file>
```

The server should return HTTP 200 or 201 on success.

## Troubleshooting

### Device won't connect to WiFi
- Check `secrets.yaml` credentials
- Ensure the device is in range of your WiFi network
- Connect to the fallback hotspot "medallion Fallback Hotspot" to configure

### SD card not detected
- Ensure card is FAT32 or exFAT formatted
- Try a different SD card
- Check SD card is fully inserted

### No audio recording
- Verify ES8311 codec is detected (check logs)
- Ensure SD card is mounted
- Check microphone hardware connection

### OTA updates fail
- Verify device is on the same network
- Check API encryption key matches
- Try reducing upload chunk size

### Display not working
- Check PMIC voltages are configured correctly
- Verify display reset sequence in logs
- Ensure BLDO1 is enabled (powers OLED)

## Development

To modify the custom components:

1. Edit the Python and C++ files in `custom_components/`
2. Run `esphome compile medallion.yaml` to verify
3. Run `esphome run medallion.yaml` to deploy

## License

This project is provided as-is for the Medallion hardware platform.

## Credits

- ESPHome project: https://esphome.io
- Arduino GFX Library: https://github.com/moononournation/Arduino_GFX
- XPowersLib: https://github.com/lewisxhe/XPowersLib
- SdFat: https://github.com/greiman/SdFat
