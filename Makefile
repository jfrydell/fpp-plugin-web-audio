include /opt/fpp/src/makefiles/common/setup.mk

all: libfpp-web-audio.so

OBJECTS_fpp_WebAudio_so += src/FPPWebAudio.o
LIBS_fpp_WebAudio_so += -L/opt/fpp/src -lfpp
CXXFLAGS_src/FPPWebAudio.o += -I/opt/fpp/src

%.o: %.cpp Makefile
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

libfpp-web-audio.so: $(OBJECTS_fpp_WebAudio_so) /opt/fpp/src/libfpp.so 
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_fpp_WebAudio_so) $(LIBS_fpp_WebAudio_so) $(LDFLAGS) -o $@

clean:
	rm -f libfpp-web-audio.so $(OBJECTS_fpp_WebAudio_so)
