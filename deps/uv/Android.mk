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

LOCAL_MODULE := uv

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/uv-private \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/ares/config_android
 
LOCAL_SRC_FILES := \
	src/uv-common.c \
	src/ares/ares__close_sockets.c \
	src/ares/ares__get_hostent.c \
	src/ares/ares__read_line.c \
	src/ares/ares__timeval.c \
	src/ares/ares_cancel.c \
	src/ares/ares_data.c \
	src/ares/ares_destroy.c \
	src/ares/ares_expand_name.c \
	src/ares/ares_expand_string.c \
	src/ares/ares_fds.c \
	src/ares/ares_free_hostent.c \
	src/ares/ares_free_string.c \
	src/ares/ares_gethostbyaddr.c \
	src/ares/ares_gethostbyname.c \
	src/ares/ares_getnameinfo.c \
	src/ares/ares_getopt.c \
	src/ares/ares_getsock.c \
	src/ares/ares_init.c \
	src/ares/ares_library_init.c \
	src/ares/ares_llist.c \
	src/ares/ares_mkquery.c \
	src/ares/ares_nowarn.c \
	src/ares/ares_options.c \
	src/ares/ares_parse_a_reply.c \
	src/ares/ares_parse_aaaa_reply.c \
	src/ares/ares_parse_mx_reply.c \
	src/ares/ares_parse_ns_reply.c \
	src/ares/ares_parse_ptr_reply.c \
	src/ares/ares_parse_srv_reply.c \
	src/ares/ares_parse_txt_reply.c \
	src/ares/ares_process.c \
	src/ares/ares_query.c \
	src/ares/ares_search.c \
	src/ares/ares_send.c \
	src/ares/ares_strcasecmp.c \
	src/ares/ares_strdup.c \
	src/ares/ares_strerror.c \
	src/ares/ares_timeout.c \
	src/ares/ares_version.c \
	src/ares/ares_writev.c \
	src/ares/bitncmp.c \
	src/ares/inet_net_pton.c \
	src/ares/inet_ntop.c \
	src/unix/cares.c \
	src/unix/core.c \
	src/unix/dl.c \
	src/unix/error.c \
	src/unix/fs.c \
	src/unix/linux.c \
	src/unix/pipe.c \
	src/unix/process.c \
	src/unix/stream.c \
	src/unix/tcp.c \
	src/unix/thread.c \
	src/unix/tty.c \
	src/unix/udp.c \
	src/unix/uv-eio.c \
	src/unix/eio/eio.c \
	src/unix/ev/ev.c

# debug
ifeq ($(debug),true)
LOCAL_CFLAGS += \
	-DDEBUG \
	-g \
	-O0 \
	-Wall \
	-Wextra
endif

# ares
LOCAL_CFLAGS += \
	-DHAVE_CONFIG_H \
	-include sys/select.h
	
# eio
LOCAL_CFLAGS += \
	-DHAVE_PREADWRITE=1 \
	-DHAVE_SENDFILE \
	-D_GNU_SOURCE \
	-DHAVE_UTIMES
	
# ev	
LOCAL_CFLAGS += \
	-DEV_FORK_ENABLE=0 \
	-DEV_EMBED_ENABLE=0 \
	-DHAVE_SELECT \
	-DHAVE_SYS_SELECT_H \
	-DEV_SELECT_USE_FD_SET \
	-DEV_STANDALONE

# common flags
LOCAL_CFLAGS += \
	-D__POSIX__ \
	-DEIO_STACKSIZE=65536 \
	-D_LARGEFILE_SOURCE \
	-D_FILE_OFFSET_BITS=64 \
	-DHAVE_FDATASYNC=1 \
	-D_FORTIFY_SOURCE=2 \
	-DPLATFORM=\"android\" \
	-DBUILDING_UV_SHARED \
	-Wno-unused-parameter

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
