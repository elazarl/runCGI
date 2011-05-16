SRCFILES := $(shell find $(PROJDIRS) -maxdepth 1 -type f -name \*.cpp)
HDRFILES := $(shell find $(PROJDIRS) -maxdepth 1 -type f -name \*.h)

OBJFILES := $(patsubst %.cpp,%.o,$(SRCFILES))
DEPFILES := $(patsubst %.cpp,%.d,$(SRCFILES))

ALLFILES := $(SRCFILES) $(HDRFILES)

.PHONY: all clean

all: $(TARG)

$(TARG): $(TARG).a
	g++ $? -o $@ $(LIBS) 

$(TARG).a: $(OBJFILES)
	@ar r $@ $?

clean:
	-@$(RM) $(wildcard $(OBJFILES) $(DEPFILES) $(TARG).a $(TARG))

-include $(DEPFILES)

%.o: %.cpp Makefile
	g++ $(CPPFLAGS) $(INC) -MMD -MP -c $< -o $@

