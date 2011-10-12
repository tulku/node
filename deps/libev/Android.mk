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
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ev

LOCAL_SRC_FILES := \
	ev.c

LOCAL_CFLAGS += \
	-DEV_FORK_ENABLE=0 \
	-DEV_EMBED_ENABLE=0 \
	-DEV_MULTIPLICITY=0 \
	-DHAVE_SELECT \
	-DHAVE_SYS_SELECT_H \
	-DEV_SELECT_USE_FD_SET \
	-DEV_STANDALONE \
	-include ev.h

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
