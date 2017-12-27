OUT := session

CXXFLAGS := -std=c++14 -g -Iexternal/sqlite_orm/include
LDFLAGS := -lsqlite3 -lboost_program_options

$(OUT): session.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

all: $(OUT)

clean:
	rm -f $(OUT)
