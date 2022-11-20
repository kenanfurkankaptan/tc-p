# the compiler: gcc for C program, define as g++ for C++
CXX = g++

# compiler flags:
# -g     - this flag adds debugging information to the executable file
# -Wall  - this flag is used to turn on most compiler warnings
CPPFLAGS = -g -O -Wall -std=c++20

# The build target 
TARGET = tc++p

RM = rm -f

# linker flags
# libraries to link
LDFLAGS += -ltuntap++ -ltuntap

# A directory to store object files
OBJDIR :=objects
BUILDDIR = build


SRCDIRS := . net control util

HEADERSUFFIX = .h
SRCSUFFIX = .cpp
OBJSUFFIX = .o


SOURCES := $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $d/*,$(SRCSUFFIX))))
OBJECTS := $(addsuffix $(OBJSUFFIX),$(basename $(patsubst $(SRCDIRS)%,$(BUILDDIR)/%,$(SOURCES))))

# link .o files
$(TARGET): $(OBJECTS)
	@$(CXX) $^ -o $@ $(LDFLAGS)
	@$(RM) $(OBJECTS)
	@echo "$(TARGET) is compiled successfully"


# compile .cpp files
$(OBJECTS): %.o : %.cpp
	@$(CXX) $(CPPFLAGS) $(LDFLAGS) -c $< -o $@
	@echo $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

debug: 
	@echo $(SOURCES)
	@echo $(OBJECTS)