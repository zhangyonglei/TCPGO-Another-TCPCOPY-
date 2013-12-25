#makefile created by 16236914@qq.com on August 10 2011.
#Using GNU make 3.81, GCC 4.3.2
#The projct aims to supply assistance functions.

#projname := $(notdir $(CURDIR))
projname := horos
libstem := horos
test := test
lib_linkname := lib$(libstem).so
libname = $(lib_linkname).$(VERSION_NUM)
#soname = $(lib_linkname).$(firstword $(subst ., ,$(VERSION_NUM)))
#don't use so version control
soname = $(lib_linkname)
sources := $(wildcard *.cpp)
objects := $(subst .cpp,.o,$(sources))
dependencies := $(subst .cpp,.d,$(sources))
public_dirs := ./public

VERSION_NUM := 1.0.0
CXXFLAGS += -fvisibility=hidden 
CPPFLAGS += -I$(public_dirs) -g -fPIC -D__DEBUG__
LINKFLAGS := -lpthread -lpcap 
LINKFLAGS4LIB := -shared -Wl,-soname,$(soname) -lpthread -lpcap  
LINKFLAGS4UT := -L. -l$(libstem) -Wl,-rpath,. 
RM := rm -rf
MV := mv

vpath %.cpp .
vpath %.h . $(public_dirs)

.PHONY : clean install

all : $(projname) $(libname) $(test)

$(projname) : $(objects)
	g++ $(LINKFLAGS) -o $@ $^
	
$(libname) : $(objects)
	g++ $(LINKFLAGS4LIB) -o $@ $^	
	-ln -s $(libname) $(lib_linkname)
	
test : unit_test/test.o
#	ld -rpath . -o test unit_test/test.o
	g++ -o $@  $^ $(LINKFLAGS4UT)
	
install :
#	-cp $(libname) /usr/local/lib
#	-ln -s /usr/local/lib/$(libname) /usr/lib/$(soname)
 	
unit_test/test.o: unit_test/unit_test.cpp public/misc.h public/horos.h $(libname)
	g++ -c -o $@ unit_test/unit_test.cpp  

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
  g++ -MM -MF $3 -MT $2 $(CPPFLAGS) $(TARGET_ARCH) $1
endef

%.o : %.cpp
	$(call make-depend, $<,$@,$(subst .o,.d,$@))
	$(COMPILE.C) $(OUTPUT_OPTION) $<

clean :
	$(RM) $(objects) $(dependencies) $(projname) $(libname)
