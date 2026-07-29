"""
Volumetric Renderer Module

Converts 3D objects into voxel grids suitable for laser scanning.

Reference:
- VOLUMETRIC_DISPLAY_SPECIFICATION.md (100×100×100 voxel grid, 10-60 FPS)
- Phase 4-5: detailed design → prototype (placeholder for full voxelization engine)
"""

import logging
import math
from typing import Dict, Any, Optional
import numpy as np

logger = logging.getLogger(__name__)


class VolumetricRenderer:
    """
    Renders 3D objects into a volumetric voxel grid.

    Placeholder implementation; Phase 5-6 would integrate:
    - Mesh-to-voxel conversion (marching cubes, etc.)
    - Anti-aliasing for volumetric rendering
    - Texture mapping onto voxel grid
    - Real-time performance optimization (voxel budget per frame)
    """

    def __init__(self, grid_resolution: int = 100, frame_rate_fps: int = 30):
        """
        Initialize renderer.

        Args:
            grid_resolution: Voxel grid size (grid_resolution × grid_resolution × grid_resolution).
            frame_rate_fps: Target frame rate (10-60 FPS).
        """
        self.grid_resolution = grid_resolution
        self.frame_rate_fps = frame_rate_fps

        # Allocate voxel grid: (R, G, B) intensity per voxel, 0–1.0
        self.voxel_grid = np.zeros(
            (grid_resolution, grid_resolution, grid_resolution, 3),
            dtype=np.float32,
        )

        self._render_active = False
        self._frame_count = 0

        logger.info(
            f"VolumetricRenderer initialized: {grid_resolution}³ voxels, "
            f"{frame_rate_fps} FPS target"
        )

    def render_object(self, object_type: str, scale: float = 1.0):
        """
        Render a 3D object into the voxel grid.

        Args:
            object_type: Type of object ("sphere", "cube", "torus", "mesh_custom").
            scale: Scaling factor (0.1–2.0).

        Raises:
            ValueError: If object_type is not supported.
        """
        if object_type not in ["sphere", "cube", "torus", "mesh_custom"]:
            raise ValueError(f"Unsupported object type: {object_type}")

        if not (0.1 <= scale <= 2.0):
            raise ValueError(f"Scale out of range [0.1, 2.0], got {scale}")

        logger.info(f"Rendering {object_type} (scale={scale})")

        # Clear previous grid
        self.voxel_grid.fill(0.0)

        if object_type == "sphere":
            self._render_sphere(scale)
        elif object_type == "cube":
            self._render_cube(scale)
        elif object_type == "torus":
            self._render_torus(scale)
        elif object_type == "mesh_custom":
            logger.warning("mesh_custom rendering not yet implemented")

        self._render_active = True
        self._frame_count = 0

    def _render_sphere(self, scale: float):
        """Render a sphere at center of voxel grid."""
        center = self.grid_resolution / 2.0
        radius = (self.grid_resolution / 2.0) * scale

        for i in range(self.grid_resolution):
            for j in range(self.grid_resolution):
                for k in range(self.grid_resolution):
                    # Distance from center
                    dist = math.sqrt((i - center) ** 2 + (j - center) ** 2 + (k - center) ** 2)

                    if dist <= radius:
                        # Solid sphere
                        intensity = 1.0 - (dist / radius) * 0.5  # Gradient falloff
                        self.voxel_grid[i, j, k] = [intensity, intensity * 0.8, intensity * 0.6]

    def _render_cube(self, scale: float):
        """Render a cube at center of voxel grid."""
        center = self.grid_resolution / 2.0
        half_size = (self.grid_resolution / 2.0) * scale

        for i in range(self.grid_resolution):
            for j in range(self.grid_resolution):
                for k in range(self.grid_resolution):
                    # Check if inside cube
                    if (abs(i - center) <= half_size and
                        abs(j - center) <= half_size and
                        abs(k - center) <= half_size):
                        self.voxel_grid[i, j, k] = [0.8, 0.9, 1.0]

    def _render_torus(self, scale: float):
        """Render a torus at center of voxel grid."""
        center = self.grid_resolution / 2.0
        major_radius = (self.grid_resolution / 3.0) * scale
        minor_radius = (self.grid_resolution / 8.0) * scale

        for i in range(self.grid_resolution):
            for j in range(self.grid_resolution):
                for k in range(self.grid_resolution):
                    # Torus distance formula
                    dx = i - center
                    dz = k - center
                    dist_from_axis = math.sqrt(dx ** 2 + dz ** 2)
                    dist_from_torus = abs(dist_from_axis - major_radius) + abs(j - center)

                    if dist_from_torus <= minor_radius:
                        intensity = 1.0 - (dist_from_torus / minor_radius) * 0.5
                        self.voxel_grid[i, j, k] = [intensity, 0.5, intensity * 0.9]

    def get_status(self) -> Dict[str, Any]:
        """Get current renderer status."""
        active_voxel_count = np.count_nonzero(
            np.sum(self.voxel_grid, axis=3) > 0.01
        )

        return {
            "rendering": self._render_active,
            "grid_resolution": self.grid_resolution,
            "frame_rate_fps": self.frame_rate_fps,
            "frame_count": self._frame_count,
            "active_voxels": int(active_voxel_count),
            "total_voxels": self.grid_resolution ** 3,
        }

    def get_telemetry(self) -> Dict[str, Any]:
        """Get real-time telemetry for WebSocket streaming."""
        self._frame_count += 1

        active_voxel_count = np.count_nonzero(
            np.sum(self.voxel_grid, axis=3) > 0.01
        )

        return {
            "rendering": self._render_active,
            "frame": self._frame_count,
            "active_voxels": int(active_voxel_count),
        }

    def get_voxel_grid(self) -> np.ndarray:
        """Get current voxel grid for laser scanning."""
        return self.voxel_grid.copy()
