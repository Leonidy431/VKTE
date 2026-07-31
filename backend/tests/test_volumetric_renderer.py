"""Unit tests for VolumetricRenderer module."""

import pytest
import numpy as np
from app.modules.volumetric.volumetric_renderer import VolumetricRenderer


class TestVolumetricRendererInit:
    """Test VolumetricRenderer initialization."""

    def test_init_defaults(self):
        """Test default initialization."""
        renderer = VolumetricRenderer()
        assert renderer.grid_resolution == 100
        assert renderer.frame_rate_fps == 30
        assert renderer.voxel_grid.shape == (100, 100, 100, 3)
        assert renderer._render_active is False
        assert renderer._frame_count == 0

    def test_init_custom_resolution(self):
        """Test initialization with custom resolution."""
        renderer = VolumetricRenderer(grid_resolution=64, frame_rate_fps=60)
        assert renderer.grid_resolution == 64
        assert renderer.frame_rate_fps == 60
        assert renderer.voxel_grid.shape == (64, 64, 64, 3)

    def test_voxel_grid_initialization(self):
        """Test that voxel grid is initialized to zero."""
        renderer = VolumetricRenderer(grid_resolution=50)
        assert np.all(renderer.voxel_grid == 0.0)
        assert renderer.voxel_grid.dtype == np.float32


class TestVolumetricRendererRenderObject:
    """Test object rendering."""

    def test_render_sphere(self):
        """Test rendering a sphere."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere", scale=1.0)
        assert renderer._render_active is True
        assert renderer._frame_count == 0
        # Check that some voxels are non-zero
        assert np.sum(renderer.voxel_grid) > 0

    def test_render_cube(self):
        """Test rendering a cube."""
        renderer = VolumetricRenderer()
        renderer.render_object("cube", scale=1.0)
        assert renderer._render_active is True
        # Check that some voxels are non-zero
        assert np.sum(renderer.voxel_grid) > 0

    def test_render_torus(self):
        """Test rendering a torus."""
        renderer = VolumetricRenderer()
        renderer.render_object("torus", scale=1.0)
        assert renderer._render_active is True
        # Check that some voxels are non-zero
        assert np.sum(renderer.voxel_grid) > 0

    def test_render_sphere_small_scale(self):
        """Test rendering sphere with minimum scale."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere", scale=0.1)
        assert renderer._render_active is True
        assert np.sum(renderer.voxel_grid) > 0

    def test_render_sphere_large_scale(self):
        """Test rendering sphere with maximum scale."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere", scale=2.0)
        assert renderer._render_active is True
        assert np.sum(renderer.voxel_grid) > 0

    def test_render_cube_small_scale(self):
        """Test rendering cube with small scale."""
        renderer = VolumetricRenderer()
        renderer.render_object("cube", scale=0.1)
        assert np.sum(renderer.voxel_grid) > 0

    def test_render_torus_large_scale(self):
        """Test rendering torus with large scale."""
        renderer = VolumetricRenderer()
        renderer.render_object("torus", scale=2.0)
        assert np.sum(renderer.voxel_grid) > 0

    def test_render_mesh_custom(self):
        """Test rendering mesh_custom (not yet implemented, logs warning, no crash)."""
        renderer = VolumetricRenderer()
        renderer.render_object("mesh_custom")
        assert renderer._render_active is True
        # No geometry is actually produced for mesh_custom yet
        assert np.sum(renderer.voxel_grid) == 0

    def test_render_unsupported_object(self):
        """Test rendering unsupported object type."""
        renderer = VolumetricRenderer()
        with pytest.raises(ValueError, match="Unsupported object type"):
            renderer.render_object("unsupported_type")

    def test_render_scale_out_of_range_low(self):
        """Test rendering with scale below minimum."""
        renderer = VolumetricRenderer()
        with pytest.raises(ValueError, match="Scale out of range"):
            renderer.render_object("sphere", scale=0.05)

    def test_render_scale_out_of_range_high(self):
        """Test rendering with scale above maximum."""
        renderer = VolumetricRenderer()
        with pytest.raises(ValueError, match="Scale out of range"):
            renderer.render_object("sphere", scale=2.1)

    def test_render_clears_previous_grid(self):
        """Test that rendering clears previous voxel grid."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere", scale=1.0)
        initial_sum = np.sum(renderer.voxel_grid)

        renderer.render_object("cube", scale=0.1)  # Small cube
        new_sum = np.sum(renderer.voxel_grid)
        # New grid should have less data than sphere
        assert new_sum < initial_sum

    def test_render_object_multiple_times(self):
        """Test rendering multiple objects in sequence."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere", scale=1.0)
        assert renderer._render_active is True

        renderer.render_object("cube", scale=1.0)
        assert renderer._render_active is True

        renderer.render_object("torus", scale=1.0)
        assert renderer._render_active is True

    def test_render_frame_count_reset(self):
        """Test that frame count resets on new render."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere")
        # Telemetry increments frame count
        renderer.get_telemetry()
        renderer.get_telemetry()
        assert renderer._frame_count == 2

        # Rendering new object should reset frame count
        renderer.render_object("cube")
        assert renderer._frame_count == 0


class TestVolumetricRendererSphereProperties:
    """Test sphere rendering properties."""

    def test_sphere_has_rgb_values(self):
        """Test that rendered sphere has RGB channel values."""
        renderer = VolumetricRenderer(grid_resolution=50)
        renderer.render_object("sphere", scale=1.0)

        # Find a non-zero voxel and check it has RGB
        non_zero = np.where(np.sum(renderer.voxel_grid, axis=3) > 0)
        assert len(non_zero[0]) > 0
        idx = non_zero[0][0], non_zero[1][0], non_zero[2][0]
        rgb = renderer.voxel_grid[idx]
        assert len(rgb) == 3
        assert all(0 <= v <= 1.0 for v in rgb)

    def test_sphere_intensity_gradient(self):
        """Test that sphere has intensity gradient (brighter at center)."""
        renderer = VolumetricRenderer(grid_resolution=50)
        center = 25
        renderer.render_object("sphere", scale=1.0)

        center_intensity = np.sum(renderer.voxel_grid[center, center, :])
        edge_intensity = np.sum(renderer.voxel_grid[5, 5, :])
        # Center should have higher intensity than edges
        assert center_intensity > edge_intensity


class TestVolumetricRendererStatus:
    """Test status reporting."""

    def test_get_status_inactive(self):
        """Test status when not rendering."""
        renderer = VolumetricRenderer()
        status = renderer.get_status()
        assert status["rendering"] is False
        assert status["grid_resolution"] == 100
        assert status["frame_rate_fps"] == 30
        assert status["frame_count"] == 0
        assert status["active_voxels"] == 0
        assert status["total_voxels"] == 1000000

    def test_get_status_after_render(self):
        """Test status after rendering object."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere")
        status = renderer.get_status()
        assert status["rendering"] is True
        assert status["active_voxels"] > 0
        assert status["total_voxels"] == 1000000

    def test_get_status_custom_resolution(self):
        """Test status with custom resolution."""
        renderer = VolumetricRenderer(grid_resolution=50)
        status = renderer.get_status()
        assert status["grid_resolution"] == 50
        assert status["total_voxels"] == 125000

    def test_get_telemetry(self):
        """Test telemetry reporting."""
        renderer = VolumetricRenderer()
        renderer.render_object("cube")
        telemetry = renderer.get_telemetry()
        assert telemetry["rendering"] is True
        assert telemetry["frame"] == 1
        assert telemetry["active_voxels"] > 0

    def test_get_telemetry_increments_frame(self):
        """Test that telemetry increments frame count."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere")
        tel1 = renderer.get_telemetry()
        tel2 = renderer.get_telemetry()
        tel3 = renderer.get_telemetry()
        assert tel1["frame"] == 1
        assert tel2["frame"] == 2
        assert tel3["frame"] == 3

    def test_get_voxel_grid_is_copy(self):
        """Test that get_voxel_grid returns a copy."""
        renderer = VolumetricRenderer()
        renderer.render_object("sphere")
        grid1 = renderer.get_voxel_grid()
        grid2 = renderer.get_voxel_grid()

        # Modify grid1
        grid1[0, 0, 0] = [1.0, 1.0, 1.0]

        # grid2 should be unchanged
        assert not np.array_equal(grid1, grid2)
        assert grid2[0, 0, 0].sum() == 0.0  # Unaffected


class TestVolumetricRendererGridAllocation:
    """Test voxel grid allocation and management."""

    def test_grid_float32_dtype(self):
        """Test that voxel grid uses float32."""
        renderer = VolumetricRenderer()
        assert renderer.voxel_grid.dtype == np.float32

    def test_grid_shape_xyz_rgb(self):
        """Test that grid shape includes RGB channels."""
        renderer = VolumetricRenderer(grid_resolution=64)
        assert renderer.voxel_grid.shape == (64, 64, 64, 3)

    def test_large_resolution(self):
        """Test creating renderer with large resolution."""
        renderer = VolumetricRenderer(grid_resolution=128)
        assert renderer.grid_resolution == 128
        assert renderer.voxel_grid.shape == (128, 128, 128, 3)

    def test_small_resolution(self):
        """Test creating renderer with small resolution."""
        renderer = VolumetricRenderer(grid_resolution=32)
        assert renderer.grid_resolution == 32
        assert renderer.voxel_grid.shape == (32, 32, 32, 3)


class TestVolumetricRendererMultipleInstances:
    """Test multiple renderer instances."""

    def test_independent_renderers(self):
        """Test that multiple renderer instances are independent."""
        renderer1 = VolumetricRenderer(grid_resolution=50)
        renderer2 = VolumetricRenderer(grid_resolution=75)

        renderer1.render_object("sphere")
        renderer2.render_object("cube")

        assert renderer1.grid_resolution == 50
        assert renderer2.grid_resolution == 75
        assert renderer1.voxel_grid.shape[0] == 50
        assert renderer2.voxel_grid.shape[0] == 75

    def test_independent_frame_counts(self):
        """Test that frame counts are independent."""
        renderer1 = VolumetricRenderer()
        renderer2 = VolumetricRenderer()

        renderer1.render_object("sphere")
        renderer2.render_object("cube")

        renderer1.get_telemetry()
        renderer1.get_telemetry()
        renderer2.get_telemetry()

        assert renderer1._frame_count == 2
        assert renderer2._frame_count == 1
