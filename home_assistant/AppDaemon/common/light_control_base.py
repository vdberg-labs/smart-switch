import time

import appdaemon.plugins.hass.hassapi as hass


class Ramp:
    def __init__(self, start_pct=0, direction=1, period=4.0):
        self.period = period
        # Rising half is phase 0.0->0.5, falling half is 0.5->1.0.
        # Seed the clock so we begin at start_pct moving in `direction`.
        frac = start_pct / 100
        if direction >= 1:
            phase = frac * 0.5
        else:
            phase = 1 - (frac * 0.5)
        self.start = time.time() - phase * self.period

    def state(self):
        """Returns (brightness_pct, current_direction)."""
        t = (time.time() - self.start) % self.period
        phase = t / self.period
        if phase < 0.5:
            return int(phase * 2 * 100), 1
        return int((1 - phase) * 2 * 100), -1


class BrightnessEngineBase(hass.Hass):
    LIGHTS = []
    TEMP_STEPS = [2700, 3000, 3500, 4000, 6500]
    RAMP_PERIOD = 4.0
    LOOP_INTERVAL = 0.1
    SWITCH_ENTITY = None
    EVENTS = {}

    def initialize(self):
        self.log("Brightness Engine started")
        self.ramp = None
        self.direction = 1  # 1 = up, -1 = down; remembered across holds
        self.temp_index = 1  # start at 3000K
        self.listen_state(self.handle_event, self.SWITCH_ENTITY)
        self.run_every(self.loop, "now", self.LOOP_INTERVAL)

    def handle_event(self, entity, attribute, old, new, kwargs):
        event_type = self.get_state(entity, attribute="event_type")
        self.log(f"Event: {event_type}")

        handler_name = self.EVENTS.get(event_type)
        if handler_name is not None:
            getattr(self, handler_name)()

    def current_brightness_pct(self):
        b = self.get_state(self.LIGHTS[0], attribute="brightness")
        if b is None:
            return 0
        return round(int(b) / 255 * 100)

    def start_ramp(self):
        start_pct = self.current_brightness_pct()
        # Flip direction so each new hold continues the opposite way.
        self.direction = -self.direction
        direction_label = "up" if self.direction == 1 else "down"
        self.log(f"start ramp from {start_pct}% {direction_label}")
        self.ramp = Ramp(start_pct=start_pct, direction=self.direction, period=self.RAMP_PERIOD)

    def stop_ramp(self):
        self.log("stop ramp")
        # Remember whichever direction the sweep was actually moving at release.
        if self.ramp is not None:
            _, self.direction = self.ramp.state()
        self.ramp = None

    def toggle_light(self):
        self.toggle(self.LIGHTS[0])

    def cycle_temp(self):
        self.temp_index = (self.temp_index + 1) % len(self.TEMP_STEPS)
        kelvin = self.TEMP_STEPS[self.temp_index]
        self.log(f"color temp -> {kelvin}K")
        for light in self.LIGHTS:
            self.call_service(
                "light/turn_on",
                entity_id=light,
                color_temp_kelvin=kelvin,
            )

    def loop(self, kwargs):
        if self.ramp is None:
            return
        pct, _ = self.ramp.state()
        for light in self.LIGHTS:
            self.call_service(
                "light/turn_on",
                entity_id=light,
                brightness_pct=pct,
            )
