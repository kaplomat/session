OUT := session

CXXFLAGS := -std=c++1y -g -Weffc++
LDFLAGS := -lsqlite3

$(OUT): session.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

all: $(OUT)

clean:
	rm -f $(OUT)
