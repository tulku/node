#!/bin/sh
export NDK_MODULE_PATH=.:../../external
/Applications/android-ndk-r6b/ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=Application.mk V=1
