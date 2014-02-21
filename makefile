#created by kamuszhou@tencent.com 16236914@qq.com
#website: v.qq.com www.dogeye.net

#projname := $(notdir $(CURDIR))
bins := bins/
objs := objs/
deps := deps/
projname := $(bins)horos
projname_alias := $(bins)tcpgo
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
include_dirs = . public $(iniparser)/include $(lua_vm_src) $(boost_home)

# the great boost library
boost_home = boost_1_55_0/
boost_lib_path = $(boost_home)stage/lib 

# third-party ini parser
iniparser = cpp-iniparser
iniparser_a := $(iniparser)/lib$(iniparser).a

# lua virtual machine
lua_lib := lua
lua_vm_root = lua-mirror/
lua_vm_src := $(lua_vm_root)src/
lua_vm_a := $(lua_vm_src)/liblua.a

VERSION_NUM := 1.0.0
CXXFLAGS += -fvisibility=hidden 
CPPFLAGS += $(addprefix -I,$(include_dirs)) -g -D__DEBUG__ -fPIC
LINKFLAGS := -L$(iniparser) -L$(lua_vm_src) -L$(boost_lib_path) -lpthread -lpcap -ldl -l$(iniparser) -l$(lua_lib) -lboost_filesystem -lboost_regex
LINKFLAGS4LIB := -L$(iniparser) -L$(lua_vm_src) -L$(boost_lib_path) -shared -Wl,-soname,$(soname) -lpthread -lpcap -ldl -l$(iniparser) -l$(lua_lib) -lboost_filesystem -lboost_regex
LINKFLAGS4TEST := -L./$(bins) -l$(libstem) -Wl,-rpath,. 
MAKECMDGOAL := 
RM := rm -rf
MV := mv

vpath %.cpp .
vpath %.h . $(include_dirs)

.PHONY : clean install dummy

all : $(projname) $(projname_alias) $(libname) $(test) 

$(projname) : $(bins) $(deps) $(objs) $(objects) $(iniparser_a) $(lua_vm_a)
	g++ -o $@ $(objects) $(LINKFLAGS) 

$(projname_alias) : $(projname)
	-ln $(projname) $(projname_alias)

$(iniparser_a) : 
	make -C $(iniparser)

$(lua_vm_a) :
	make -C $(lua_vm_root) linux
	
$(libname) : $(objects)
	g++ -o $@ $^ $(LINKFLAGS4LIB) 
	-ln -s $(PWD)/$@ $(lib_linkname)
	
$(test) : test/test.o
#	ld -rpath . -o test test/test.o
	g++ -o $@  $^ $(LINKFLAGS4TEST) 

# for debug's purpose
dummy:
	#echo $(dependencies)
	
install :
#	-cp $(libname) /usr/local/lib
#	-ln -s /usr/local/lib/$(libname) /usr/lib/$(soname)
 	
test/test.o: test/test.cpp public/misc.h public/horos.h $(libname)
	$(COMPILE.C) $(OUTPUT_OPTION) $<

$(bins):
	-mkdir $@
	cp my.conf.template $(bins)my.conf
	 
$(deps):
	-mkdir $@
 
$(objs):
	-mkdir $@

ifneq "$(MAKECMDGOAL)" "clean"
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
	$(call make-depend, $<,$@,$(addprefix $(deps),$(subst .cpp,.d,$<)))
	$(COMPILE.C) $(OUTPUT_OPTION) $<
#g++ $(CXXFLAGS) $(CPPFLAGS) $< # lacks -c -o 

clean :
	$(RM) $(objects) $(dependencies) $(projname) $(libname) $(bins) $(objs) $(deps) $(iniparser_a)
	make -C $(iniparser) clean
