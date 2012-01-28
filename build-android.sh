#!/bin/sh
export NDK_MODULE_PATH=.:..
ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=Application.mk V=1
