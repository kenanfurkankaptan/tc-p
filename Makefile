# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
# -g     - this flag adds debugging information to the executable file
# -Wall  - this flag is used to turn on most compiler warnings
CPPFLAGS = -g -Wall -std=c++20

# The build target 
TARGET = tc++p

RM = rm -f

# linker flags
# libraries to link
LDFLAGS += -ltuntap++ -ltuntap


# A directory to store object files
OBJDIR :=objects

SRC =*.cpp
DEPS =*.h
OBJS = main.o tcp_header.o ip_header.o connection.o queue.o utility.o controller.o


# Read this ugly line from the end:
#  - grep all the .cpp files in SRC with wildcard
#  - add the prefix $(OBJDIR) to all the file names with addprefix
#  - replace .cpp in .o with patsubst
# OBJS := $(patsubst %.cpp,%.o,$(addprefix $(OBJDIR)/,$(wildcard $(SRC))))
# OBJS := $(patsubst %.cpp,%.o,$(wildcard $(SRC)))

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -o $@ $(LDFLAGS)
	rm *.o
	
$(OBJS): %.o : %.cpp
	@echo $@
	$(CC) -c $(CPPFLAGS) $< $(LDFLAGS)


clean:
	$(RM) *.o $(TARGET)

debug: 
	@echo $(OBJS)
	@echo $(SRC)
