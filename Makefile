# the compiler: gcc for C program, define as g++ for C++
CXX = g++

# COMPILER FLAGS:

# -g	- this flag adds debugging information to the executable file
# -O	- the compiler tries to reduce code size and execution time, it may create distruptions in debuggig, remove it in debugging
CPPFLAGS = -g -O3 -std=c++20

# -Wall  					- this flag is used to turn on most compiler warnings
# -Wextra  					- this flag enables some extra warning flags that are not enabled by -Wall.
# -Wshadow 					- warn whenever a local variable or type declaration shadows another variable
# -Wconversion  			- warn for implicit conversions that may alter a value
# -Wpedantic  				- issue all the warnings demanded by strict ISO C and ISO C++; reject all programs that use forbidden extensions, and some other programs that
#							 do not follow ISO C and ISO C++. For ISO C, follows the version of the ISO C standard specified by any -std option used. 
# -Werror  					- make all warnings into errors.
# -Wnon-virtual-dtor		- warns whenever a class with virtual function does not declare a virtual destructor, unless the destructor 
#							 in the base class is made protected. This helps catch hard to track down memory errors
# -Wdelete-non-virtual-dtor	- warns whenever delete is invoked on a pointer to a class that has virtual functions but no virtual destructor,
#							 unless the class is marked final
# -fno-omit-frame-pointer	- option instructs the compiler not to store the stack frame pointer in a register, nicer stack traces in error messages
COMPILER_WARNINGS = -Wall -Wextra -Wconversion -Wpedantic -Wnon-virtual-dtor -Wdelete-non-virtual-dtor -fno-omit-frame-pointer

# -fsanitize=address				- memory error detector. Memory access instructions are instrumented to detect out-of-bounds and use-after-free bugs
# -fsanitize=pointer-compare 		- instrument comparison operation (<, <=, >, >=) with pointer operands
# -fsanitize=pointer-subtract		- instrument subtraction with pointer operands
# -detect_invalid_pointer_pairs=2	- enable pointer-subtract in run time !!! DO NOT USE IT !!! IT REQUIRES MASSIVE AMOUNT OF MEMORY EG. ~30gb FOR THIS PROJECT !!!
# -D_GLIBCXX_SANITIZE_VECTOR		- detect container overflow
ADDRESS = -fsanitize=address,pointer-compare,pointer-subtract -D_GLIBCXX_SANITIZE_VECTOR

# -fsanitize=undefined					- undefined behavior detector. Various computations are instrumented to detect undefined behavior at runtime.
# -fsanitize=float-divide-by-zero		- detect floating-point division by zero
# -fsanitize=float-cast-overflow		- floating-point type to integer conversion checking
# -fsanitize=signed-integer-overflow	- signed integer overflow checking
UNDEFINED = -fsanitize=undefined,float-divide-by-zero,float-cast-overflow,signed-integer-overflow,

# -fsanitize=thread		- data race detector, memory access instructions are instrumented to detect data race bugs.
THREAD= -fsanitize=thread

# -fsanitize=leak 	-run-time memory leak detector
# If you just need leak detection, and don't want to bear the ASan slowdown, you can build with -fsanitize=leak instead of -fsanitize=address.
LEAK = -fsanitize=leak


# thread sanitizer is not compatible with address sanitizers, undefined sanitizers or leak saintizers
CPPFLAGS += $(COMPILER_WARNINGS) $(UNDEFINED) $(ADDRESS) 


# The build target 
TARGET = tc++p

RM = rm -rf

# linker flags
# libraries to link
LDFLAGS += -ltuntap++ -ltuntap

# Src files list
SRCDIR := src
SRCSUBDIRS := . net/ip_header net/tcp_header control connection connection_info util util/queue

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

debug:
	@echo $(CPPFLAGS)
