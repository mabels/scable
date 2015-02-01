

DEBUG=-g
PTHREAD=-pthread

UNAME_S := $(shell uname -s)
LDFLAGS=$(DEBUG) -lssl -lcrypto
CXXFLAGS=$(DEBUG) $(PTHREAD) -std=c++11 -O3
ifeq ($(UNAME_S), Linux)
	LD=g++
	CXX=g++ 
endif
ifeq ($(UNAME_S),Darwin)
	CXXFLAGS+=-Wdeprecated-declarations -I/usr/local/Cellar/openssl/1.0.2/include
	LD=clang++ -L/usr/local/Cellar/openssl/1.0.2/lib 
	CXX=clang++ 
endif

SOURCES=TestEnDecryption.cc
OBJECTS = $(SOURCES:.cc=.o)
EXES = $(OBJECTS:.o=)

$(EXES): $(OBJECTS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ 

%: %.o
	$(CXX) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(EXES) $(OBJECTS)

