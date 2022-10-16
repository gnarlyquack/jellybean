EXENAME := jellybean
CXX_FILE_EXT := cpp
CXX_HEADER_EXT := hpp
BUILDDIR := build

QTLIBS := Qt6Core Qt6Gui Qt6Widgets
LIBS :=


# C++ options

#CXX := clang++
CXX := g++
CXXFLAGS := --std=c++17
CXXFLAGS += -g
#CXXFLAGS += -Og
#CXXFLAGS += -O3
CXXFLAGS += -Wall -Wextra -Wpedantic
CXXFLAGS += -Werror
CXXFLAGS += -Wcast-qual
CXXFLAGS += -Wconversion
CXXFLAGS += -Wshadow
CXXFLAGS += -Wunused
CXXFLAGS += -Wzero-as-null-pointer-constant
CXXFLAGS += -fPIC # required by Qt

CPPFLAGS :=
LDFLAGS :=
LDLIBS :=

DEBUGGER := lldb



# Automatically find source files

CXXFILES := $(shell find src -name '*.$(CXX_FILE_EXT)')



# Automatically configure libraries using pkg-config

ifneq ($(strip $(QTLIBS)), )
QT_DEFINES := $(shell pkg-config --cflags-only-other $(QTLIBS))
QT_INCLUDES := $(shell pkg-config --cflags-only-I $(QTLIBS))
LDLIBS += $(shell pkg-config --libs $(QTLIBS))

CPPFLAGS += $(QT_DEFINES)
CPPFLAGS += $(QT_INCLUDES:-I%=-isystem%)
endif


ifneq ($(strip $(LIBS)), )
CXXFLAGS += $(shell pkg-config --cflags $(LIBS))
LDLIBS += $(shell pkg-config --libs $(LIBS))
endif


# Automatically generate object file names from source file names
# Object files retain the original file extension (.cpp, .c) to ensure unique
# object file names regardless of the source file extension.

CXXOBJECTS := $(CXXFILES:%=%.o)



# Automatically generate dependencies as described at
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
#
# This relies upon the compiler's ability to generate make-compatible
# dependency files during compilation (using, in the case of clang and gcc,
# various -M flags).
#
# DEPFLAGS and POSTCOMPILE are meant to be used in rules, so they must be
# recursively expanded to ensure the automatic variables they reference expand
# to their correct value.
#
# Since dependency files are generated into their own build directory and
# makefiles don't typically have a file extension, we simply use the same name
# as the source file they're generated from. This ensures the resulting
# dependency file name is unique and also allows us to simplify the recipe to
# generate them by using the $* automatic variable
#
# POSTCOMPILE ensures that killing the build doesn't result in corrupt
# dependency files and that the compiled object file has a later timestamp than
# the generated dependency file (thus preventing spurious rebuilds).

SOURCES := $(CXXFILES)

DEPFILES := $(SOURCES:%=%.d)
DEPFLAGS = -MT $@ -MD -MP -MF $<.temp
POSTCOMPILE = mv -f $<.temp $< && touch $@


# Default rule

OBJECTS := $(CXXOBJECTS)

$(BUILDDIR)/$(EXENAME): $(OBJECTS) $(LDLIBS) | $(BUILDDIR)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)


# Other convenience commands

.PHONY: run
run: $(BUILDDIR)/$(EXENAME)
	$<


.PHONY: debug
debug: $(BUILDDIR)/$(EXENAME)
	$(DEBUGGER) $<


.PHONY: clean
clean:
	rm -rf $(BUILDDIR)
	find src -name '*.o' -delete
	find src -name '*.d' -delete



# Create build directories
# Use mkdir -p, which doesn't error if a directory already exists, to support
# running make with the -B/--always-build option

$(BUILDDIR):
	mkdir -p $@



# Compile C++ files

$(CXXOBJECTS): %.o: %.d
	$(CXX) $(DEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $*
	$(POSTCOMPILE)



# Dependency files

# Instead of providing an explicit recipe to (re-)build dependency files
# directly, a blank recipe causes them to be indirectly built by triggering
# (re-)compilation of the depending object file.
#
# Making this rule explicit for each dependency file (rather than implicit for
# all dependency files) ensures they aren't deleted as temporary files.
$(DEPFILES):

include $(wildcard $(DEPFILES))
