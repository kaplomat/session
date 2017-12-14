OUT := session

CXXFLAGS := -std=c++1y -g -Weffc++
LDFLAGS := -lsqlite3 -lboost_program_options

$(OUT): session.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

all: $(OUT)

clean:
	rm -f $(OUT)
