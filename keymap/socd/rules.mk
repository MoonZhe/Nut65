ENCODER_MAP_ENABLE = yes
VIA_ENABLE = yes
WPM_ENABLE = yes

# Fixed build date (1970-01-01) in version.h. VIA stamps its EEPROM with the
# build date and wipes the saved layout when the stamp doesn't match, so this
# keeps VIA changes across flashes. Set here rather than via SKIP_VERSION,
# which build_keyboard.mk reads before it includes this file.
VERSION_H_FLAGS += --skip-all

# Temporary wake-from-sleep event log: make leku/nut65:socd WAKE_DEBUG=yes
ifeq ($(strip $(WAKE_DEBUG)), yes)
    SRC += wake_debug.c
    OPT_DEFS += -DWAKE_DEBUG
endif
