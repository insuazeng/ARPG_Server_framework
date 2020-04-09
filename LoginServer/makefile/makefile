
-include makefile.target

CXX = g++

CXXFLAGS = -I"../../CommonFiles" -I"../../CommonLib/include" -I"../../CommonLib/include/3rd" -I"../../LoginServer" -DEPOLL_MULTI_THREAD -O2 -g3 -std=c++11 -Wall -c -fmessage-length=0 -MMD -MP

LDFLAGS =  -L"../../CommonLib/lib/linux_debug_x64" -lrt -lpthread -lcommonlib -lmysqlclient -ldl -lz -lpbc -ljson -lmongoclient -lboost_chrono -lboost_date_time -lboost_regex -lboost_system -lboost_thread

RUN_OBJ := LoginServer

OBJ := $(patsubst %.cpp,%.o, $(target))
OBJD := $(patsubst %.cpp,%.d, $(target))

all: $(RUN_OBJ)

clean:
	rm -f $(OBJ) $(OBJD)
	rm -f $(RUN_OBJ) ../../../Arpg2016_output/Linux_Debug/$(RUN_OBJ)

$(RUN_OBJ) : $(OBJ)
	@echo "going to build $(RUN_OBJ)"
	$(CXX) $(OBJ) $(LDFLAGS) -o $(RUN_OBJ)
	cp -f $(RUN_OBJ) ../../../Arpg2016_output/Linux_Debug/$(RUN_OBJ)

%.o : %.cpp
	@echo "---> Compiling $<  $@ "
	$(CXX) $(CXXFLAGS) -c $< -o $@

