TARG := runCGI

ifeq ("$(DEBUG)","1")
CPPFLAGS := -ggdb3
else
CPPFLAGS := -Os
endif

LIBS := -l boost_regex -l boost_program_options -lyaml-cpp 

# please execute
# $ wget http://yaml-cpp.googlecode.com/files/yaml-cpp-0.2.6.tar.gz
# $ tar xzf yaml-cpp-0.2.6.tar.tgz
# $ cd yaml-cpp
# $ mkdir build && cd build && cmake ..
# $ make
# Now you can either make install, or set the following constant
# to the path of yaml-cpp (for example /opt/yaml-cpp)
YAMLCPP_BASE_DIR=

# if you want to keep your working directory pristine, you can
# add your changes to myconf.mk, override defaults and still
# have git status -s report nothing
# You can set YAMLCPP_BASE_DIR variable there.

-include myconf.mk

ifdef YAMLCPP_BASE_DIR
INC += -I $(YAMLCPP_BASE_DIR)/include
LIBS += -L $(YAMLCPP_BASE_DIR)/build
endif


include makecppexec.mk
