"""Host-side data acquisition client for the STM32H745 shotgun test bench.

This module reads the newline-delimited JSON telemetry emitted by the test
bench over a serial link, exposes a small command API, and persists shot
events to disk for later analysis.

The code targets Python 3.9+ and follows PEP 8 / PEP 257 conventions.
"""

from __future__ import annotations

import json
import time
from dataclasses import dataclass, field, asdict
from typing import Iterator, List, Optional

try:
    import serial  # pyserial; optional at import time for offline testing.
except ImportError:  # pragma: no cover - exercised only without pyserial.
    serial = None


DEFAULT_BAUDRATE = 921600
DEFAULT_TIMEOUT_S = 1.0


@dataclass
class ShotEvent:
    """A single logged shot decoded from a ``shot_event`` telemetry packet."""

    shot_id: int
    timestamp_ms: int
    distance_m: float
    barrel_temp_c: float
    recoil_peak_g: float
    hit_x_mm: int = 0
    hit_y_mm: int = 0
    ammo_type: str = "unknown"

    @classmethod
    def from_packet(cls, packet: dict) -> "ShotEvent":
        """Build a :class:`ShotEvent` from a decoded telemetry dictionary."""
        return cls(
            shot_id=int(packet.get("shot_id", 0)),
            timestamp_ms=int(packet.get("timestamp_ms", 0)),
            distance_m=float(packet.get("distance_m", 0.0)),
            barrel_temp_c=float(packet.get("barrel_temp_c", 0.0)),
            recoil_peak_g=float(packet.get("recoil_peak_g", 0.0)),
            hit_x_mm=int(packet.get("hit_x_mm", 0)),
            hit_y_mm=int(packet.get("hit_y_mm", 0)),
            ammo_type=str(packet.get("ammo_type", "unknown")),
        )


@dataclass
class Session:
    """An acquisition session grouping the shots of one firing string."""

    name: str
    ammo_type: str
    expected_distance_m: float
    shots: List[ShotEvent] = field(default_factory=list)

    def add(self, shot: ShotEvent) -> None:
        """Append a decoded shot to the session."""
        self.shots.append(shot)

    def save(self, path: str) -> None:
        """Serialize the session to a JSON file."""
        payload = {
            "name": self.name,
            "ammo_type": self.ammo_type,
            "expected_distance_m": self.expected_distance_m,
            "shots": [asdict(shot) for shot in self.shots],
        }
        with open(path, "w", encoding="utf-8") as handle:
            json.dump(payload, handle, indent=2)


class ShotgunTestBench:
    """Serial client for the test bench.

    Example:
        bench = ShotgunTestBench("/dev/ttyUSB0")
        bench.start_session("group_1", "factory_28g", 25.0)
        for shot in bench.stream_shots():
            print(shot)
    """

    def __init__(
        self,
        port: str,
        baudrate: int = DEFAULT_BAUDRATE,
        timeout: float = DEFAULT_TIMEOUT_S,
    ) -> None:
        if serial is None:
            raise RuntimeError(
                "pyserial is required for live acquisition; "
                "install it with 'pip install pyserial'."
            )
        self._link = serial.Serial(port, baudrate, timeout=timeout)
        self._session: Optional[Session] = None

    def _send_command(self, command: str, **params) -> None:
        """Encode and transmit a JSON command to the bench."""
        message = {"command": command, **params}
        payload = (json.dumps(message) + "\n").encode("utf-8")
        self._link.write(payload)

    def start_session(
        self, name: str, ammo_type: str, expected_distance_m: float
    ) -> Session:
        """Open a new session and instruct the bench to begin logging."""
        self._session = Session(name, ammo_type, expected_distance_m)
        self._send_command(
            "start_session",
            session_name=name,
            ammo_type=ammo_type,
            expected_distance_m=expected_distance_m,
        )
        return self._session

    def end_session(self) -> Optional[Session]:
        """Close the current session and return it."""
        self._send_command("stop_session")
        session, self._session = self._session, None
        return session

    def stream_shots(self, duration_s: Optional[float] = None) -> Iterator[ShotEvent]:
        """Yield shot events as they arrive.

        Args:
            duration_s: Optional wall-clock limit; ``None`` streams until the
                serial link is closed by the caller.
        """
        deadline = None if duration_s is None else time.monotonic() + duration_s
        while deadline is None or time.monotonic() < deadline:
            line = self._link.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                continue
            try:
                packet = json.loads(line)
            except json.JSONDecodeError:
                continue
            if packet.get("type") == "shot_event":
                shot = ShotEvent.from_packet(packet)
                if self._session is not None:
                    self._session.add(shot)
                yield shot

    def close(self) -> None:
        """Release the serial link."""
        self._link.close()
