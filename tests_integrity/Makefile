CFLAGS := -g $(shell pkg-config --cflags lwip) -I./ -Wall -Wmaybe-uninitialized
CXXFLAGS := $(CFLAGS) -fpermissive
LDFLAGS := $(shell pkg-config --libs lwip) -pthread

all: lwsgi

lwsgi: main.o lwsgi.o http_states.o connect.o
	$(CXX) main.o lwsgi.o http_states.o connect.o -o lwsgi $(LDFLAGS)

clean:
	$(RM) -f *.o lwsgi

info:
	@echo $(CFLAGS)
	@echo $(CPPFLAGS)
	@echo $(CFLAGS)
	@echo $(CPPFLAGS)


