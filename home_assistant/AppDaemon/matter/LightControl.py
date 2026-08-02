from ..common.light_control_base import BrightnessEngineBase

class LightControl(BrightnessEngineBase):
    LIGHTS = ["light.test_lights"]
    SWITCH_ENTITY = "event.smart_switch_button_1"
    EVENTS = {
        "long_press": "start_ramp",
        "long_release": "stop_ramp",
        "multi_press_2": "cycle_temp",
    }
