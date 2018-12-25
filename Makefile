TARGET_NAME := cpc

UNIT := cpc
	
SRCDIR	:=	src
OBJDIR 	:=	build

SUBBUILD :=	#..//build/lib.so

TPDEP := jansson libcurl uuid #three part dependencies

# ---

TARGET := $(TARGET_NAME:%=lib%.so)
OBJ := $(UNIT:%=%.o)
LIBS := 	$(shell pkg-config --libs $(TPDEP))
CFLAGS := 	$(shell pkg-config --cflags $(TPDEP))
PREFIX ?= /usr/local

.PHONY: all clean test

all : $(TARGET)

#$(SUBBUILD) :
#	$(MAKE) -C $(dir $@)

$(TARGET) : $(SUBBUILD) $(OBJ:%=$(OBJDIR)/%)
	$(CXX) -o $@ -shared $^ $(LIBS)

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) -o $@ --std=c++17 -pedantic-errors -fPIC -c $< $(CFLAGS)

clean :
	$(RM) $(OBJ:%=$(OBJDIR)/%) $(TARGET)
#	$(foreach ELEMENT, $(SUBBUILD), $(MAKE) -C $(dir $(ELEMENT)) clean)

test :
	$(CXX) -o $@ --std=c++17 -L./ -l$(TARGET_NAME) -Wl,-rpath,. src/test.cpp
