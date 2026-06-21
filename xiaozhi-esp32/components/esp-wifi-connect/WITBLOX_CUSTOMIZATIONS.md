# Witblox Customizations - ESP WiFi Connect Component

This is a **LOCAL COPY** of the `78/esp-wifi-connect` component with Witblox branding customizations.

## Why Local Component?

The component is stored locally in `components/esp-wifi-connect` instead of `managed_components/` to preserve customizations. If it was in `managed_components/`, running `idf.py fullclean` would delete it and re-download the original from the ESP Component Registry.

## Customizations Made

### 1. **WiFi Configuration Page** (`assets/wifi_configuration.html`)
- ✅ Added Witblox navbar header with logo
- ✅ Removed language selector (hidden)
- ✅ Removed Advanced tab (hidden)
- ✅ Added footer with link: "WitBlox - Build like an engineer" → https://witblox.com
- ✅ All text in English only
- ✅ Clean, simple interface

### 2. **Success Page** (`assets/wifi_configuration_done.html`)
- ✅ Added Witblox navbar header with logo
- ✅ Removed Chinese text ("配置成功！")
- ✅ English-only success message
- ✅ Added footer with Witblox link
- ✅ Clean, modern design

### 3. **Logo** (`assets/witblox.png`)
- ✅ Added Witblox logo image
- ✅ Embedded in firmware via CMakeLists.txt

### 4. **Server Code** (`wifi_configuration_ap.cc`)
- ✅ Added HTTP handler to serve `/witblox.png`
- ✅ Logo is embedded in binary and served from flash

### 5. **Build Configuration** (`CMakeLists.txt`)
- ✅ Added `witblox.png` to `EMBED_FILES` for both IDF v5 and v6

## Component Configuration

### Integration with Main Component

The local component is properly linked in `main/CMakeLists.txt`:
```cmake
PRIV_REQUIRES
    ...
    nvs_flash           # Required for settings storage
    esp-wifi-connect    # Local WiFi component with Witblox customizations
    ...
```

### Component Registry

The component is **commented out** in `main/idf_component.yml`:
```yaml
# 78/esp-wifi-connect: ~3.1.3  # Using local component instead
```

This prevents the component manager from downloading and overwriting the local version.

## Building

The build system automatically detects local components in the `components/` folder and uses them instead of downloading from the registry.

```bash
# These commands will use your local customized component:
idf.py build
idf.py flash

# Even after fullclean, your customizations are preserved:
idf.py fullclean
idf.py build
```

## Original Component

- **Name:** 78/esp-wifi-connect
- **Version:** 3.1.5 (base version before customizations)
- **Registry:** https://components.espressif.com/components/78/esp-wifi-connect

## Notes

- Do NOT delete the `components/esp-wifi-connect` folder
- All customizations are in this local copy
- The `managed_components/` folder can be safely deleted - it will be regenerated for other components

---

**Customized for Witblox AI Assistant**  
Last updated: June 19, 2026
