
ifeq ($(OS),Windows_NT)
CXX="x86_64-w64-mingw32-c++.exe"
else
CXX=c++
endif

APP=testapp

CXXFLAGS=-Wall -std=c++11 -m64

# This section needs some mending. Believed to only function on windows.. barely at that
ifeq ($(OS),Windows_NT)
LDFLAGS= -m64  -Llibs -lftd2xx -Llibs/kvaser/lib/x64 -lcanlib32
else
LDFLAGS=-Llibs -lftd2xx_x64 -lcanlib -lusb -lpthread
endif




.PHONY: all clean

all: BUILDMSG $(APP)

BUILDMSG:
	@echo Building..


# # #
# Adapters and corresponding interface code
ADAPTEROBJS=\
	out/adapter.o \
	out/message.o \
 	out/canusb.o \
	out/kvaser.o

out/adapter.o: adapter/adapter.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/message.o: adapter/message.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/canusb.o: adapter/canusb/canusb.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/kvaser.o: adapter/kvaser/kvaser.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# # #
# Commnunication protocols
COMOBJS=\
	$(ADAPTEROBJS) \
	out/gmlan.o \
	out/acdelco_e39.o

out/gmlan.o: ecu/protocol/gmlan.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/gmloader.o: ecu/protocol/gmloader.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/acdelco_e39.o: ecu/targets/acdelco/e39.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# # #
# Tools
TOOLOBJS=\
	out/checksum.o \
	out/tools.o \
	out/file.o \
	out/logger.o \
	out/timer.o

out/file.o: tools/file/file.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/tools.o: tools/tools.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/logger.o: tools/logger/logger.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/timer.o: tools/timer/timer.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

out/checksum.o: tools/checksum/checksum.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@


# # #
# Main binary
OBJS=\
	out/main.o \
	$(TOOLOBJS) \
	$(COMOBJS)


out/main.o: main.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@


$(APP): $(OBJS)
	@$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	@rm -f out/*.o
	@rm -f $(APP)
	@rm -f $(APP).exe
