from ..common.light_control_base import BrightnessEngineBase

SWITCH = "switch1"

class LightControl(BrightnessEngineBase):
    LIGHTS = ["light.test_lights"]
    SWITCH_ENTITY = "event.smart_switch_action"
    EVENTS = {
        f"{SWITCH}_long_press": "start_ramp",
        f"{SWITCH}_long_release": "stop_ramp",
        f"{SWITCH}_tap_1": "toggle_light",
        f"{SWITCH}_tap_2": "cycle_temp",
    }
