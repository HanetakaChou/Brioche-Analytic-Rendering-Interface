# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# ====================================================================
#
# Host system auto-detection.
#
# ====================================================================
ifeq ($(OS),Windows_NT)
	# On all modern variants of Windows (including Cygwin and Wine)
	# the OS environment variable is defined to 'Windows_NT'
	#
	# The value of PROCESSOR_ARCHITECTURE will be x86 or AMD64
	#
	HOST_OS := windows

	# Trying to detect that we're running from Cygwin is tricky
	# because we can't use $(OSTYPE): It's a Bash shell variable
	# that is not exported to sub-processes, and isn't defined by
	# other shells (for those with really weird setups).
	#
	# Instead, we assume that a program named /bin/uname.exe
	# that can be invoked and returns a valid value corresponds
	# to a Cygwin installation.
	#
	UNAME := $(shell /bin/uname.exe -s 2>NUL)
	ifneq (,$(filter CYGWIN% MINGW32% MINGW64%,$(UNAME)))
		HOST_OS := unix
		_ := $(shell rm -f NUL) # Cleaning up
	endif
else
	HOST_OS := unix
endif

# -----------------------------------------------------------------------------
# Function : host-mkdir
# Arguments: 1: directory path
# Usage    : $(call host-mkdir,<path>
# Rationale: This function expands to the host-specific shell command used
#            to create a path if it doesn't exist.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-mkdir = md $(subst /,\,"$1") >NUL 2>NUL || rem
else
host-mkdir = mkdir -p $1
endif

# -----------------------------------------------------------------------------
# Function : host-rm
# Arguments: 1: list of files
# Usage    : $(call host-rm,<files>)
# Rationale: This function expands to the host-specific shell command used
#            to remove some files.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-rm = \
	$(eval __host_rm_files := $(foreach __host_rm_file,$1,$(subst /,\,$(wildcard $(__host_rm_file)))))\
	$(if $(__host_rm_files),del /f/q $(__host_rm_files) >NUL 2>NUL || rem)
else
host-rm = rm -f $1
endif

#
# Copyright (C) YuqiaoZhang(HanetakaChou)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

HIDE := @

LOCAL_PATH := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
SHADERS_DIR := $(LOCAL_PATH)/../shaders
ifeq (true, $(APP_DEBUG))
	DXIL_DIR := $(LOCAL_PATH)/../shaders/dxil/debug
else
	DXIL_DIR := $(LOCAL_PATH)/../shaders/dxil/release
endif
ifeq ($(OS),Windows_NT)
	HLSL_COMPILER_PATH := $(realpath $(LOCAL_PATH)/../../DirectXShaderCompiler/bin/win32/x64/dxc.exe)
else
	UNAME := $(strip $(shell uname -s 2>/dev/null))
	ifeq ($(UNAME),Darwin)
		HLSL_COMPILER_PATH := $(realpath $(LOCAL_PATH)/../../DirectXShaderCompiler/bin/macos/x86_64/dxc)
	else
		HLSL_COMPILER_PATH := $(realpath $(LOCAL_PATH)/../../DirectXShaderCompiler/bin/linux/x86_64/dxc)
	endif
endif

HLSL_COMPILER_DEBUG_FLAGS :=
ifeq (true, $(APP_DEBUG))
	HLSL_COMPILER_DEBUG_FLAGS += -Od -Zi -Qembed_debug
else
	HLSL_COMPILER_DEBUG_FLAGS += 
endif

all : \
	$(DXIL_DIR)/_internal_full_screen_transfer_vertex.inl \
	$(DXIL_DIR)/_internal_full_screen_transfer_fragment.inl \
	$(DXIL_DIR)/_internal_deforming_compute.inl \
	$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.inl \
	$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.inl \
	$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.inl \
	$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.inl \
	$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.inl \
	$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.inl \
	$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl \
	$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl \
	$(DXIL_DIR)/_internal_forward_shading_vertex.inl \
	$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.inl \
	$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl \
	$(DXIL_DIR)/_internal_post_processing_vertex.inl \
	$(DXIL_DIR)/_internal_post_processing_fragment.inl

$(DXIL_DIR)/_internal_full_screen_transfer_vertex.inl $(DXIL_DIR)/_internal_full_screen_transfer_vertex.d : $(SHADERS_DIR)/full_screen_transfer_vertex.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -MD -MF "$(DXIL_DIR)/_internal_full_screen_transfer_vertex.d" -Fo "$(DXIL_DIR)/_internal_full_screen_transfer_vertex.inl" "$(SHADERS_DIR)/full_screen_transfer_vertex.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -Fh "$(DXIL_DIR)/_internal_full_screen_transfer_vertex.inl" -Vn "full_screen_transfer_vertex_shader_module_code" "$(SHADERS_DIR)/full_screen_transfer_vertex.bsl"

$(DXIL_DIR)/_internal_full_screen_transfer_fragment.inl $(DXIL_DIR)/_internal_full_screen_transfer_fragment.d : $(SHADERS_DIR)/full_screen_transfer_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_full_screen_transfer_fragment.d" -Fo "$(DXIL_DIR)/_internal_full_screen_transfer_fragment.inl" "$(SHADERS_DIR)/full_screen_transfer_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_full_screen_transfer_fragment.inl" -Vn "full_screen_transfer_fragment_shader_module_code" "$(SHADERS_DIR)/full_screen_transfer_fragment.bsl"

$(DXIL_DIR)/_internal_deforming_compute.inl $(DXIL_DIR)/_internal_deforming_compute.d : $(SHADERS_DIR)/deforming_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_deforming_compute.d" -Fo "$(DXIL_DIR)/_internal_deforming_compute.inl" "$(SHADERS_DIR)/deforming_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_deforming_compute.inl" -Vn "deforming_compute_shader_module_code" "$(SHADERS_DIR)/deforming_compute.bsl"

$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.inl $(DXIL_DIR)/_internal_area_lighting_emissive_vertex.d : $(SHADERS_DIR)/area_lighting_emissive_vertex.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -MD -MF "$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.d" -Fo "$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.inl" "$(SHADERS_DIR)/area_lighting_emissive_vertex.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -Fh "$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.inl" -Vn "area_lighting_emissive_vertex_shader_module_code" "$(SHADERS_DIR)/area_lighting_emissive_vertex.bsl"

$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.inl $(DXIL_DIR)/_internal_area_lighting_emissive_fragment.d : $(SHADERS_DIR)/area_lighting_emissive_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.d" -Fo "$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.inl" "$(SHADERS_DIR)/area_lighting_emissive_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.inl" -Vn "area_lighting_emissive_fragment_shader_module_code" "$(SHADERS_DIR)/area_lighting_emissive_fragment.bsl"

$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.inl $(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.d : $(SHADERS_DIR)/environment_lighting_sh_projection_environment_map_clear_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.d" -Fo "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.inl" "$(SHADERS_DIR)/environment_lighting_sh_projection_environment_map_clear_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.inl" -Vn "environment_lighting_sh_projection_environment_map_clear_compute_shader_module_code" "$(SHADERS_DIR)/environment_lighting_sh_projection_environment_map_clear_compute.bsl"

$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.inl $(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.d : $(SHADERS_DIR)/environment_lighting_sh_projection_equirectangular_environment_map_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.d" -Fo "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.inl" "$(SHADERS_DIR)/environment_lighting_sh_projection_equirectangular_environment_map_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.inl" -Vn "environment_lighting_sh_projection_equirectangular_environment_map_compute_shader_module_code" "$(SHADERS_DIR)/environment_lighting_sh_projection_equirectangular_environment_map_compute.bsl"

$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.inl $(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.d : $(SHADERS_DIR)/environment_lighting_sh_projection_octahedral_environment_map_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.d" -Fo "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.inl" "$(SHADERS_DIR)/environment_lighting_sh_projection_octahedral_environment_map_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.inl" -Vn "environment_lighting_sh_projection_octahedral_environment_map_compute_shader_module_code" "$(SHADERS_DIR)/environment_lighting_sh_projection_octahedral_environment_map_compute.bsl"

$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.inl $(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.d : $(SHADERS_DIR)/environment_lighting_skybox_vertex.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -MD -MF "$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.d" -Fo "$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.inl" "$(SHADERS_DIR)/environment_lighting_skybox_vertex.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -Fh "$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.inl" -Vn "environment_lighting_skybox_vertex_shader_module_code" "$(SHADERS_DIR)/environment_lighting_skybox_vertex.bsl"

$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl $(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d : $(SHADERS_DIR)/environment_lighting_skybox_equirectangular_map_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d" -Fo "$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl" "$(SHADERS_DIR)/environment_lighting_skybox_equirectangular_map_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl" -Vn "environment_lighting_skybox_equirectangular_map_fragment_shader_module_code" "$(SHADERS_DIR)/environment_lighting_skybox_equirectangular_map_fragment.bsl"

$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl $(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d : $(SHADERS_DIR)/environment_lighting_skybox_octahedral_map_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d" -Fo "$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl" "$(SHADERS_DIR)/environment_lighting_skybox_octahedral_map_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl" -Vn "environment_lighting_skybox_octahedral_map_fragment_shader_module_code" "$(SHADERS_DIR)/environment_lighting_skybox_octahedral_map_fragment.bsl"

$(DXIL_DIR)/_internal_forward_shading_vertex.inl $(DXIL_DIR)/_internal_forward_shading_vertex.d : $(SHADERS_DIR)/forward_shading_vertex.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -MD -MF "$(DXIL_DIR)/_internal_forward_shading_vertex.d" -Fo "$(DXIL_DIR)/_internal_forward_shading_vertex.inl" "$(SHADERS_DIR)/forward_shading_vertex.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -Fh "$(DXIL_DIR)/_internal_forward_shading_vertex.inl" -Vn "forward_shading_vertex_shader_module_code" "$(SHADERS_DIR)/forward_shading_vertex.bsl"

$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.inl $(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.d : $(SHADERS_DIR)/forward_shading_physically_based_rendering_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.d" -Fo "$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.inl" "$(SHADERS_DIR)/forward_shading_physically_based_rendering_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.inl" -Vn "forward_shading_physically_based_rendering_fragment_shader_module_code" "$(SHADERS_DIR)/forward_shading_physically_based_rendering_fragment.bsl"

$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.inl $(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.d : $(SHADERS_DIR)/forward_shading_toon_shading_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.d" -Fo "$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.inl" "$(SHADERS_DIR)/forward_shading_toon_shading_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.inl" -Vn "forward_shading_toon_shading_fragment_shader_module_code" "$(SHADERS_DIR)/forward_shading_toon_shading_fragment.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_zero_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_zero_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.inl" -Vn "voxel_cone_tracing_zero_compute_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_zero_compute.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_clear_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_clear_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.inl" -Vn "voxel_cone_tracing_clear_compute_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_clear_compute.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d : $(SHADERS_DIR)/voxel_cone_tracing_voxelization_vertex.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_vertex.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl" -Vn "voxel_cone_tracing_voxelization_vertex_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_vertex.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.d : $(SHADERS_DIR)/voxel_cone_tracing_voxelization_physically_based_rendering_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.inl" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_physically_based_rendering_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.inl" -Vn "voxel_cone_tracing_voxelization_physically_based_rendering_fragment_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_physically_based_rendering_fragment.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.d : $(SHADERS_DIR)/voxel_cone_tracing_voxelization_toon_shading_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.inl" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_toon_shading_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.inl" -Vn "voxel_cone_tracing_voxelization_toon_shading_fragment_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_toon_shading_fragment.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_pack_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_pack_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.inl" -Vn "voxel_cone_tracing_pack_compute_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_pack_compute.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_low_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_low_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl" -Vn "voxel_cone_tracing_cone_tracing_low_compute_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_low_compute.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_medium_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_medium_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl" -Vn "voxel_cone_tracing_cone_tracing_medium_compute_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_medium_compute.bsl"

$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl $(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_high_compute.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -MD -MF "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d" -Fo "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_high_compute.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T cs_6_0 -Fh "$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl" -Vn "voxel_cone_tracing_cone_tracing_high_compute_shader_module_code" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_high_compute.bsl"

$(DXIL_DIR)/_internal_post_processing_vertex.inl $(DXIL_DIR)/_internal_post_processing_vertex.d : $(SHADERS_DIR)/post_processing_vertex.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -MD -MF "$(DXIL_DIR)/_internal_post_processing_vertex.d" -Fo "$(DXIL_DIR)/_internal_post_processing_vertex.inl" "$(SHADERS_DIR)/post_processing_vertex.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T vs_6_0 -Fh "$(DXIL_DIR)/_internal_post_processing_vertex.inl" -Vn "post_processing_vertex_shader_module_code" "$(SHADERS_DIR)/post_processing_vertex.bsl"

$(DXIL_DIR)/_internal_post_processing_fragment.inl $(DXIL_DIR)/_internal_post_processing_fragment.d : $(SHADERS_DIR)/post_processing_fragment.bsl
	$(HIDE) $(call host-mkdir,$(DXIL_DIR))
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -MD -MF "$(DXIL_DIR)/_internal_post_processing_fragment.d" -Fo "$(DXIL_DIR)/_internal_post_processing_fragment.inl" "$(SHADERS_DIR)/post_processing_fragment.bsl"
	$(HIDE) "$(HLSL_COMPILER_PATH)" $(HLSL_COMPILER_DEBUG_FLAGS) -T ps_6_0 -Fh "$(DXIL_DIR)/_internal_post_processing_fragment.inl" -Vn "post_processing_fragment_shader_module_code" "$(SHADERS_DIR)/post_processing_fragment.bsl"

-include \
	$(DXIL_DIR)/_internal_full_screen_transfer_vertex.d \
	$(DXIL_DIR)/_internal_full_screen_transfer_fragment.d \
	$(DXIL_DIR)/_internal_deforming_compute.d \
	$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.d \
	$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.d \
	$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.d \
	$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.d \
	$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.d \
	$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.d \
	$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d \
	$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d \
	$(DXIL_DIR)/_internal_forward_shading_vertex.d \
	$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.d \
	$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d \
	$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d \
	$(DXIL_DIR)/_internal_post_processing_vertex.d \
	$(DXIL_DIR)/_internal_post_processing_fragment.d

clean:
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_full_screen_transfer_vertex.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_full_screen_transfer_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_deforming_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_forward_shading_vertex.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_post_processing_vertex.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_post_processing_fragment.inl)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_full_screen_transfer_vertex.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_full_screen_transfer_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_deforming_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_area_lighting_emissive_vertex.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_area_lighting_emissive_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_sh_projection_environment_map_clear_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_sh_projection_equirectangular_environment_map_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_sh_projection_octahedral_environment_map_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_skybox_vertex.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_forward_shading_vertex.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_forward_shading_physically_based_rendering_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_forward_shading_toon_shading_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_zero_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_clear_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_physically_based_rendering_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_voxelization_toon_shading_fragment.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_pack_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_post_processing_vertex.d)
	$(HIDE) $(call host-rm,$(DXIL_DIR)/_internal_post_processing_fragment.d)

.PHONY : \
	all \
	clean
