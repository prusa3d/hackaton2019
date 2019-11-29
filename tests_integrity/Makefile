RFC1123_DATETIME := $(shell LC_ALL=C TZ='GMT' date +'"%a, %d %b %Y %T GMT"')
CFLAGS := -g $(shell pkg-config --cflags lwip) -I./ -Wall -Wmaybe-uninitialized \
	-D RFC1123_DATETIME='$(RFC1123_DATETIME)'
CXXFLAGS := $(CFLAGS) -fpermissive
LDFLAGS := $(shell pkg-config --libs lwip) -pthread

all: lwsapi

lwsapi: main.o lwsapi.o http_states.o connect.o
	$(CXX) main.o lwsapi.o http_states.o connect.o -o lwsapi $(LDFLAGS)

hello_world: main.o lwsapi.o hello_world.o
	$(CXX) main.o lwsapi.o hello_world.o -o hello_world $(LDFLAGS)

clean:
	$(RM) -f *.o lwsapi

info:
	@echo $(CFLAGS)
	@echo $(CPPFLAGS)
	@echo $(CFLAGS)
	@echo $(CPPFLAGS)


