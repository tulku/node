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
LOCAL_PATH  := $(call my-dir)

ifndef NODE_PREFIX
	NODE_PREFIX := $(LOCAL_PATH)
endif

include $(CLEAR_VARS)

intermediates := $(LOCAL_PATH)/out/$(APP_OPTIM)

include $(LOCAL_PATH)/Android.jsnatives.mk
src/node_javascript.cc : $(jsnatives)

LOCAL_MODULE := node

LOCAL_CPP_EXTENSION := .cc

# FIXME: create a generic dependency that derives from
# LOCAL_EXPORT_INCLUDES for the deps
LOCAL_C_INCLUDES += \
	$(intermediates)/src \
	src \
	deps/c-ares \
	deps/c-ares/android \
	deps/libev \
	deps/libeio \
	deps/http_parser \
	deps/v8/include
	
LOCAL_SRC_FILES := \
	src/node.cc \
	src/node_buffer.cc \
	src/node_cares.cc \
	src/node_child_process.cc \
	src/node_constants.cc \
	src/node_dtrace.cc \
	src/node_events.cc \
	src/node_extensions.cc \
	src/node_file.cc \
	src/node_http_parser.cc \
	src/node_idle_watcher.cc \
	src/node_io_watcher.cc \
	src/node_javascript.cc \
	src/node_main.cc \
	src/node_net.cc \
	src/node_os.cc \
	src/node_script.cc \
	src/node_signal_watcher.cc \
	src/node_stat_watcher.cc \
	src/node_string.cc \
	src/node_timer.cc \
	src/platform_android.cc
# FIXME: re-add stdio
#	src/node_stdio.cc

ifdef ANDROID_PATH_OPENSSL
	LOCAL_C_INCLUDES += \
		$(ANDROID_PATH_OPENSSL)/include
	LOCAL_SRC_FILES += \
		src/node_crypto.cc
endif

LOCAL_STATIC_LIBRARIES := \
	deps/c-ares \
	deps/libev \
	deps/libeio \
	deps/http_parser
	
LOCAL_CFLAGS += \
	-D__POSIX__ \
	-DNODE_CFLAGS=\"\" \
	-DNODE_PREFIX=\"$(NODE_PREFIX)\" \
	-DPLATFORM=\"android\" \
	-include sys/select.h

include $(BUILD_SHARED_LIBRARY)
