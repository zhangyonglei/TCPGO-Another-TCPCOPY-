#created by kamuszhou@tencent.com 16236914@qq.com
#website: v.qq.com www.dogeye.net

#projname := $(notdir $(CURDIR))
bins := bins/
objs := objs/
deps := deps/
projname := $(bins)horos
libstem := horos
test := $(bins)test
lib_linkname := $(bins)lib$(libstem).so
libname = $(lib_linkname).$(VERSION_NUM)
#soname = $(lib_linkname).$(firstword $(subst ., ,$(VERSION_NUM)))
#don't use so version control
soname = lib$(libstem).so
sources := $(wildcard *.cpp)
objects := $(addprefix $(objs),$(subst .cpp,.o,$(sources)))
dependencies := $(addprefix $(deps),$(subst .cpp,.d,$(sources)))
public_dirs := ./public

VERSION_NUM := 1.0.0
CXXFLAGS += -fvisibility=hidden 
CPPFLAGS += -I$(public_dirs) -g -fPIC -D__DEBUG__
LINKFLAGS := -lpthread -lpcap 
LINKFLAGS4LIB := -shared -Wl,-soname,$(soname) -lpthread -lpcap  
LINKFLAGS4UT := -L./$(bins) -l$(libstem) -Wl,-rpath,. 
RM := rm -rf
MV := mv

vpath %.cpp .
vpath %.h . $(public_dirs)

.PHONY : clean install 

all : $(projname) $(libname) $(test) 

$(projname) : $(bins) $(deps) $(objs) $(objects) 
	g++ $(LINKFLAGS) -o $@ $(objects)
	
$(libname) : $(objects)
	g++ $(LINKFLAGS4LIB) -o $@ $^	
	-ln -s $(PWD)/$@ $(lib_linkname)
	
$(test) : unit_test/test.o
#	ld -rpath . -o test unit_test/test.o
	g++ $(LINKFLAGS4UT) -o $@  $^
	
install :
#	-cp $(libname) /usr/local/lib
#	-ln -s /usr/local/lib/$(libname) /usr/lib/$(soname)
 	
unit_test/test.o: unit_test/unit_test.cpp public/misc.h public/horos.h $(libname)
	g++ -c -o $@ unit_test/unit_test.cpp  
	
$(bins):
	mkdir $@
	 
$(deps):
	mkdir $@
 
$(objs):
	mkdir $@

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

$(objs)%.o : %.cpp
	$(call make-depend, $<,$@,$(subst .o,.d,$@))
	$(COMPILE.C) $(OUTPUT_OPTION) $<

clean :
	$(RM) $(objects) $(dependencies) $(projname) $(libname) $(bins) $(objs) $(deps)
