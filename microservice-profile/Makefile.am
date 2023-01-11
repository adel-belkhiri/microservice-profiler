AM_CPPFLAGS = -I.. -I../include
AM_CFLAGS = -fno-omit-frame-pointer -fPIC
AM_CXXFLAGS = -fno-omit-frame-pointer -fPIC

lib_LTLIBRARIES = libmicroservice-profile.la

libmicroservice_profile_la_SOURCES = \
    profiler.cc \
	tracer_provider_factory.cc \
	profile-span-processor.cc

libmicroservice_profile_la_LIBADD = \
    -L../microservice-profile-base/.libs \
	-L/usr/local/lib \
    -lmicroservice-profile-base \
	-lopentelemetry_trace
