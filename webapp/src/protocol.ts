/* Mirrors the UUIDs in src/app/ble/gesture_config_service.h. The wire format itself is no
 * longer hardcoded here — it's described by the device's own Schema characteristic and decoded
 * generically by schema.ts, so this file never needs updating when firmware's config fields
 * change. */

export const SERVICE_UUID = "5b854902-7ba7-4ce6-aa35-f734c828a00e";
export const CONFIG_CHAR_UUID = "db1147e1-d5c9-41a0-be76-cd36e7d85572";
export const SCHEMA_CHAR_UUID = "fbdce744-27cf-46c3-a21b-84953dcd14da";
