# SPDX-License-Identifier: BSD-2-Clause
# 
# Copyright (c) 2022 Vincent DEFERT. All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions 
# are met:
# 
# 1. Redistributions of source code must retain the above copyright 
# notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright 
# notice, this list of conditions and the following disclaimer in the 
# documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.

# Prerequisites --------------------------------------------------------
#
# Besides make, his project requires: 
#
# - sdcc
# - stcgal
# - minicom
# - doxygen

# Usage ----------------------------------------------------------------
#
# Build executable in release mode:
#   make
#
# Build executable in debug mode:
#   make BUILD_MODE=debug
#
# Build documentation:
#   make doc
#
# Upload executable to MCU:
#   make upload
#
# Open serial console in new window:
#   make console
#
# Clean project (remove all build files):
#   make clean

# Target MCU settings --------------------------------------------------

MCU_FREQ_KHZ := 24000
STACK_SIZE := 128
# STC8H1K28-36I-LQFP32
XRAM_SIZE := 1024
FLASH_SIZE := 28160
HAS_DUAL_DPTR := y
MEMORY_MODEL := --model-large

# Define UNISTC_DIR, HAL_DIR, DRIVER_DIR, and MAKE_DIR -----------------
# TODO: Adjust path to match you installation directory
include ../../makefiles/0-directories.mk

# Project settings -----------------------------------------------------

CONSOLE_BAUDRATE := 115200
CONSOLE_PORT := /dev/ttyUSB0

ISP_PORT := /dev/ttyUSB0
STCGAL_OPTIONS := -A rts -a

PROJECT_NAME := encoder-demo

PROJECT_FLAGS := -DENABLE_PRINTF

# To enable printf() to print over UART, add the following files to SRCS:
#
#	$(HAL_DIR)/fifo-buffer.c \
#	$(HAL_DIR)/serial-console.c \
#	$(HAL_DIR)/timer-hal.c \
#	$(HAL_DIR)/uart-hal.c \
#
# and add -DENABLE_PRINTF to PROJECT_FLAGS.

SRCS := \
	$(HAL_DIR)/delay.c \
	$(HAL_DIR)/gpio-hal.c \
	$(HAL_DIR)/advpwm-hal.c \
	$(HAL_DIR)/fifo-buffer.c \
	$(HAL_DIR)/serial-console.c \
	$(HAL_DIR)/timer-hal.c \
	$(HAL_DIR)/uart-hal.c \
	main.c

# Boilerplate rules ----------------------------------------------------
include $(MAKE_DIR)/1-mcu-settings.mk
-include $(DEP_FILE)
include $(MAKE_DIR)/2-mcu-rules.mk
