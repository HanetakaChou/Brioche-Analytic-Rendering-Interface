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
	SPIRV_DIR := $(LOCAL_PATH)/../shaders/spirv/debug
else
	SPIRV_DIR := $(LOCAL_PATH)/../shaders/spirv/release
endif
ifeq ($(OS),Windows_NT)
	GLSL_COMPILER_PATH := $(LOCAL_PATH)/../../shaderc/bin/win32/x64/glslc.exe
else
	GLSL_COMPILER_PATH := $(LOCAL_PATH)/../../shaderc/bin/linux/x64/glslc
endif

GLSL_COMPILER_FLAGS :=
ifeq (true, $(APP_DEBUG))
	GLSL_COMPILER_FLAGS += -g -O0
else
	GLSL_COMPILER_FLAGS += -O
endif

all : \
	$(SPIRV_DIR)/_internal_full_screen_transfer_vertex.inl \
	$(SPIRV_DIR)/_internal_full_screen_transfer_fragment.inl \
	$(SPIRV_DIR)/_internal_deforming_compute.inl \
	$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.inl \
	$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.inl \
	$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.inl \
	$(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.inl \
	$(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl \
	$(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl \
	$(SPIRV_DIR)/_internal_forward_shading_vertex.inl \
	$(SPIRV_DIR)/_internal_forward_shading_fragment.inl \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.inl \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.inl \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.inl \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl \
	$(SPIRV_DIR)/_internal_post_processing_vertex.inl \
	$(SPIRV_DIR)/_internal_post_processing_fragment.inl

$(SPIRV_DIR)/_internal_full_screen_transfer_vertex.inl $(SPIRV_DIR)/_internal_full_screen_transfer_vertex.d : $(SHADERS_DIR)/full_screen_transfer_vertex.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=310es --target-env=vulkan1.0 -mfmt=num -fshader-stage=vert $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_full_screen_transfer_vertex.d" -o "$(SPIRV_DIR)/_internal_full_screen_transfer_vertex.inl" "$(SHADERS_DIR)/full_screen_transfer_vertex.bsl"

$(SPIRV_DIR)/_internal_full_screen_transfer_fragment.inl $(SPIRV_DIR)/_internal_full_screen_transfer_fragment.d : $(SHADERS_DIR)/full_screen_transfer_fragment.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=310es --target-env=vulkan1.0 -mfmt=num -fshader-stage=frag $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_full_screen_transfer_fragment.d" -o "$(SPIRV_DIR)/_internal_full_screen_transfer_fragment.inl" "$(SHADERS_DIR)/full_screen_transfer_fragment.bsl"

$(SPIRV_DIR)/_internal_deforming_compute.inl $(SPIRV_DIR)/_internal_deforming_compute.d : $(SHADERS_DIR)/deforming_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_deforming_compute.d" -o "$(SPIRV_DIR)/_internal_deforming_compute.inl" "$(SHADERS_DIR)/deforming_compute.bsl"

$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.inl $(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.d : $(SHADERS_DIR)/environment_lighting_sh_projection_clear_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.d" -o "$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.inl" "$(SHADERS_DIR)/environment_lighting_sh_projection_clear_compute.bsl"

$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.inl $(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.d : $(SHADERS_DIR)/environment_lighting_sh_projection_equirectangular_map_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.d" -o "$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.inl" "$(SHADERS_DIR)/environment_lighting_sh_projection_equirectangular_map_compute.bsl"

$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.inl $(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.d : $(SHADERS_DIR)/environment_lighting_sh_projection_octahedral_map_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.d" -o "$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.inl" "$(SHADERS_DIR)/environment_lighting_sh_projection_octahedral_map_compute.bsl"

$(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.inl $(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.d : $(SHADERS_DIR)/environment_lighting_skybox_vertex.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=vert $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.d" -o "$(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.inl" "$(SHADERS_DIR)/environment_lighting_skybox_vertex.bsl"

$(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl $(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d : $(SHADERS_DIR)/environment_lighting_skybox_equirectangular_map_fragment.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=frag $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d" -o "$(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl" "$(SHADERS_DIR)/environment_lighting_skybox_equirectangular_map_fragment.bsl"

$(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl $(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d : $(SHADERS_DIR)/environment_lighting_skybox_octahedral_map_fragment.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=frag $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d" -o "$(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl" "$(SHADERS_DIR)/environment_lighting_skybox_octahedral_map_fragment.bsl"

$(SPIRV_DIR)/_internal_forward_shading_vertex.inl $(SPIRV_DIR)/_internal_forward_shading_vertex.d : $(SHADERS_DIR)/forward_shading_vertex.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=vert $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_forward_shading_vertex.d" -o "$(SPIRV_DIR)/_internal_forward_shading_vertex.inl" "$(SHADERS_DIR)/forward_shading_vertex.bsl"

$(SPIRV_DIR)/_internal_forward_shading_fragment.inl $(SPIRV_DIR)/_internal_forward_shading_fragment.d : $(SHADERS_DIR)/forward_shading_fragment.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=frag $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_forward_shading_fragment.d" -o "$(SPIRV_DIR)/_internal_forward_shading_fragment.inl" "$(SHADERS_DIR)/forward_shading_fragment.bsl"

$(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.inl $(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_zero_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.d" -o "$(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_zero_compute.bsl"

$(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.inl $(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_clear_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.d" -o "$(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_clear_compute.bsl"

$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl $(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d : $(SHADERS_DIR)/voxel_cone_tracing_voxelization_vertex.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=vert $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d" -o "$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_vertex.bsl"

$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.inl $(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.d : $(SHADERS_DIR)/voxel_cone_tracing_voxelization_fragment.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=frag $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.d" -o "$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.inl" "$(SHADERS_DIR)/voxel_cone_tracing_voxelization_fragment.bsl"

$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl $(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_low_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d" -o "$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_low_compute.bsl"

$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl $(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_medium_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d" -o "$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_medium_compute.bsl"

$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl $(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d : $(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_high_compute.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=comp $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d" -o "$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl" "$(SHADERS_DIR)/voxel_cone_tracing_cone_tracing_high_compute.bsl"

$(SPIRV_DIR)/_internal_post_processing_vertex.inl $(SPIRV_DIR)/_internal_post_processing_vertex.d : $(SHADERS_DIR)/post_processing_vertex.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=vert $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_post_processing_vertex.d" -o "$(SPIRV_DIR)/_internal_post_processing_vertex.inl" "$(SHADERS_DIR)/post_processing_vertex.bsl"

$(SPIRV_DIR)/_internal_post_processing_fragment.inl $(SPIRV_DIR)/_internal_post_processing_fragment.d : $(SHADERS_DIR)/post_processing_fragment.bsl
	$(HIDE) $(call host-mkdir,$(SPIRV_DIR))
	$(HIDE) "$(GLSL_COMPILER_PATH)" -std=460core --target-env=vulkan1.0 -mfmt=num -fshader-stage=frag $(GLSL_COMPILER_FLAGS) -MD -MF "$(SPIRV_DIR)/_internal_post_processing_fragment.d" -o "$(SPIRV_DIR)/_internal_post_processing_fragment.inl" "$(SHADERS_DIR)/post_processing_fragment.bsl"

-include \
	$(SPIRV_DIR)/_internal_full_screen_transfer_vertex.d \
	$(SPIRV_DIR)/_internal_full_screen_transfer_fragment.d \
	$(SPIRV_DIR)/_internal_deforming_compute.d \
	$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.d \
	$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.d \
	$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.d \
	$(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.d \
	$(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d \
	$(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d \
	$(SPIRV_DIR)/_internal_forward_shading_vertex.d \
	$(SPIRV_DIR)/_internal_forward_shading_fragment.d \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.d \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.d \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.d \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d \
	$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d \
	$(SPIRV_DIR)/_internal_post_processing_vertex.d \
	$(SPIRV_DIR)/_internal_post_processing_fragment.d

clean:
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_full_screen_transfer_vertex.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_full_screen_transfer_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_deforming_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_forward_shading_vertex.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_forward_shading_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_post_processing_vertex.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_post_processing_fragment.inl)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_full_screen_transfer_vertex.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_full_screen_transfer_fragment.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_deforming_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_clear_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_equirectangular_map_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_sh_projection_octahedral_map_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_skybox_vertex.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_skybox_equirectangular_map_fragment.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_environment_lighting_skybox_octahedral_map_fragment.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_forward_shading_vertex.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_forward_shading_fragment.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_zero_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_clear_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_vertex.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_voxelization_fragment.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_low_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_medium_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_voxel_cone_tracing_cone_tracing_high_compute.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_post_processing_vertex.d)
	$(HIDE) $(call host-rm,$(SPIRV_DIR)/_internal_post_processing_fragment.d)

.PHONY : \
	all \
	clean
