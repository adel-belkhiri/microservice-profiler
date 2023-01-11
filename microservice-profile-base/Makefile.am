AM_CPPFLAGS = -I.. -I../include
AM_CFLAGS = -fno-omit-frame-pointer
AM_CXXFLAGS = -fno-omit-frame-pointer

lib_LTLIBRARIES = libmicroservice-profile-base.la

libmicroservice_profile_base_la_SOURCES = \
    microservice_profile.cc \
    memory.h \
    module_abi.h \
    module_api.c \
    module_api.h \
    profiling_timer.cc \
    profiling_timer.h \
    signal_handler.cc \
    signal_handler.h \
    stacktrace.h
libmicroservice_profile_base_la_LIBADD = \
    -ldl \
    -lunwind
