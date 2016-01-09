#CXX=g++
CPPFLAGS=-DSC_DEBUG -Wall
LDFLAGS=-v
SOURCES=../wfLZ.c main.c lookup3.c
OBJECTS=$(SOURCES:.c=.o)
DEPS=../wfLZ.h
EXECUTABLE=wflz

all: $(SOURCES) $(EXECUTABLE) $(DEPS)

%.o:%.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@
	
$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.PHONY : clean
clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)

