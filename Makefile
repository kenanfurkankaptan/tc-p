# the compiler: gcc for C program, define as g++ for C++
CXX = g++

# compiler flags:
# -g     - this flag adds debugging information to the executable file
# -Wall  - this flag is used to turn on most compiler warnings
CPPFLAGS = -g -O -Wall -std=c++20

# The build target 
TARGET = tc++p

RM = rm -rf

# linker flags
# libraries to link
LDFLAGS += -ltuntap++ -ltuntap

# Src files list
SRCDIR := src
SRCSUBDIRS := . net/ip_header net/tcp_header control connection util util/queue

# A directory to store object files
BUILDDIR = .build

HEADERSUFFIX = .h
SRCSUFFIX = .cpp
OBJSUFFIX = .o

SOURCES := $(foreach d,$(addprefix $(SRCDIR)/,$(SRCSUBDIRS)),$(wildcard $(addprefix $d/*,$(SRCSUFFIX))))
OBJECTS := $(addsuffix $(OBJSUFFIX),$(basename $(patsubst $(SRCDIR)%,$(BUILDDIR)%,$(SOURCES))))

# link .o files
$(TARGET): $(OBJECTS)
	@ $(CXX) $^ -o $@ $(LDFLAGS)
	@echo "$(TARGET) is compiled successfully"

# compile .cpp files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@ $(if $(wildcard $(@D)),,mkdir -p $(@D) &&) $(CXX) $(CPPFLAGS) $(LDFLAGS) -c $< -o $@
	@echo $@

clean:
	@$(RM) $(OBJECTS) $(TARGET) $(BUILDDIR)/*
	@echo rm -rf $(OBJECTS)
	@echo rm -rf $(TARGET)
	@echo rm -rf $(BUILDDIR)/*

debug: 
	@echo $(OBJ)
	@echo $(OBJECTS)