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

LOCAL_MODULE    := c-ares
LOCAL_C_INCLUDES += \
	android/
 
LOCAL_SRC_FILES := \
	ares__close_sockets.c \
	ares__get_hostent.c \
	ares__read_line.c \
	ares__timeval.c \
	ares_cancel.c \
	ares_data.c \
	ares_destroy.c \
	ares_expand_name.c \
	ares_expand_string.c \
	ares_fds.c \
	ares_free_hostent.c \
	ares_free_string.c \
	ares_gethostbyaddr.c \
	ares_gethostbyname.c \
	ares_getnameinfo.c \
	ares_getopt.c \
	ares_getsock.c \
	ares_init.c \
	ares_library_init.c \
	ares_llist.c \
	ares_mkquery.c \
	ares_nowarn.c \
	ares_options.c \
	ares_parse_a_reply.c \
	ares_parse_aaaa_reply.c \
	ares_parse_mx_reply.c \
	ares_parse_ns_reply.c \
	ares_parse_ptr_reply.c \
	ares_parse_srv_reply.c \
	ares_parse_txt_reply.c \
	ares_process.c \
	ares_query.c \
	ares_search.c \
	ares_send.c \
	ares_strcasecmp.c \
	ares_strdup.c \
	ares_strerror.c \
	ares_timeout.c \
	ares_version.c \
	ares_writev.c \
	bitncmp.c \
	inet_net_pton.c \
	inet_ntop.c
	
LOCAL_CFLAGS += \
	-DHAVE_CONFIG_H \
	-include sys/select.h

include $(BUILD_SHARED_LIBRARY)
