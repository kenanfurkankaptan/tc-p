# the compiler: gcc for C program, define as g++ for C++
CXX = g++

# compiler flags:
# -g     		- this flag adds debugging information to the executable file
# -Wall  		- this flag is used to turn on most compiler warnings
# -Wextra  		- this flag enables some extra warning flags that are not enabled by -Wall.
# -Wshadow 		- warn whenever a local variable or type declaration shadows another variable
# -Wconversion  - warn for implicit conversions that may alter a value
# -Wpedantic  	- issue all the warnings demanded by strict ISO C and ISO C++; reject all programs that use forbidden extensions, and some other programs that
#				 do not follow ISO C and ISO C++. For ISO C, follows the version of the ISO C standard specified by any -std option used. 
# -Werror  		- make all warnings into errors.
# -Wnon-virtual-dtor - warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
# -fanalyzer  	- control static analysis
CPPFLAGS = -g -O -Wall -Wextra -Wpedantic -std=c++20

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

# link .o files and create $(TARGET)
$(TARGET): $(OBJECTS)
	@ $(CXX) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "$(TARGET) is compiled successfully"

# compile .cpp files to .o files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@echo $@
	@ $(if $(wildcard $(@D)),,mkdir -p $(@D) &&) $(CXX) $(CPPFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJECTS) $(TARGET) $(BUILDDIR)/*
	@echo rm -rf $(OBJECTS)
	@echo rm -rf $(TARGET)
	@echo rm -rf $(BUILDDIR)/*