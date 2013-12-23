#makefile created by 16236914@qq.com on August 10 2011.
#Using GNU make 3.81, GCC 4.3.2
#The projct aims to supply assistance functions.

#projname := $(notdir $(CURDIR))
projname := horos
sources := $(wildcard *.cpp)
objects := $(subst .cpp,.o,$(sources))
dependencies := $(subst .cpp,.d,$(sources))
public_dirs := ./public

CPPFLAGS += -I$(public_dirs) -g -D__DEBUG__
LINKFLAGS := -lpthread -lpcap
RM := rm -rf
MV := mv

vpath %.cpp .
vpath %.h . $(public_dirs)

.PHONY : test clean

all : $(projname)

$(projname) : $(objects)
	g++ $(LINKFLAGS) -o $@ $^	
	

# just for the purpose of testing ...
test :

ifneq "$(MAKECMDGOALS)" "clean"
  -include $(dependencies)
endif

#$(call make-depend,source-file,object-file,depend-file) 
# -MM option causes g++ to omit "system" headers from the prerequisites list.
# This option implies -E which will stop after the preprocessing stage; do not
# run the compiler proper. 
# -MF option specifies the dependecy filename.
# -MT target change the target of the rule emitted by dependency generation.
define make-depend
  g++ -MM -MF $3 -MT $2 $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $1
endef

%.o : %.cpp
	$(call make-depend, $<,$@,$(subst .o,.d,$@))
	$(COMPILE.C) $(OUTPUT_OPTION) $<

clean :
	$(RM) $(objects) $(dependencies)
