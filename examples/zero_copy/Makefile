# Optional variables:
# - DEBUG: If set to 'Y', Debug libs are used

ifeq ($(RTIMEHOME),)
	$(error Please set RTIMEHOME environment variable)
endif
ifeq ($(RTIMEOS),)
	$(error Please set RTIMEOS environment variable)
endif


OS=$(shell uname -a)
GCCVERSION=$(shell gcc --version)


ifeq ($(DEBUG),Y)
LIBSUFFIX_DEBUG=d
CFLAGS += -g
endif
LIBSUFFIX=z$(LIBSUFFIX_DEBUG)

ARCH_LIBS= -lm

ifneq ($(findstring QOS,$(RTIMEARCH)),)
        ARCH_LIBS= -lsocket
        CFLAGS = -Y_gpp -std=c99
        LFLAGS =
        CC = $(QNX_HOST)/usr/bin/qcc -Vgcc/8.3.0,gcc_ntoaarch64le
        DEFINES = -DRTI_UNIX
endif
DEFINES += -DRTI_PIL -DOSAPI_CC_DEF_H=osapi_cc_gcc.h -DRTI_CERT -DNDEBUG

INCLUDES = -I$(RTIMEHOME)/include/rti_me/rti_me_psl -I$(RTIMEHOME)/include/rti_me

PSL_LIB=-L$(RTIMEHOME)/lib/$(RTIMEARCH)-$(RTIMEOS) -lrti_me_psl$(LIBSUFFIX)
PIL_LIB=-L$(RTIMEHOME)/lib/${RTIMEARCH} -lrti_me_netiozcopy$(LIBSUFFIX) -lrti_me$(LIBSUFFIX)

LIBS = $(PSL_LIB) $(ARCH_LIBS) $(PIL_LIB) $(PSL_LIB)
# LIBS = $(PSL_LIB) $(ARCH_LIBS) $(PIL_LIB)
# LIBS = $(ARCH_LIBS) $(PSL_LIB) $(PIL_LIB)
# LIBS = $(ARCH_LIBS) $(PIL_LIB) $(PSL_LIB)

COMMONSRCS    = HelloWorld.c HelloWorldPlugin.c HelloWorldSupport.c HelloWorldApplication.c
IDL_HEADERS   = HelloWorld.h HelloWorldPlugin.h HelloWorldSupport.h
IDL_SRCS      = HelloWorld.c HelloWorldPlugin.c HelloWorldSupport.c
EXEC          = HelloWorld_publisher HelloWorld_subscriber
DIRECTORIES   = objs.dir objs/${RTIMEARCH}.dir
COMMONOBJS    = $(COMMONSRCS:%.c=objs/${RTIMEARCH}/%.o)

OUTSUFFIX = out
OUTCOMMAND = ${CC} ${LFLAGS} -o $(@:%.${OUTSUFFIX}=%) $(@:%.${OUTSUFFIX}=%.o) $(COMMONOBJS) $(LIBS)

MKDIRCOMMAND = @if [ ! -d $* ]; then echo "Making directory $*"; mkdir -p $* ; fi;


# We actually stick the objects in a sub directory to keep your directory clean.

${RTIMEARCH} : $(DIRECTORIES) $(COMMONOBJS) \
               $(EXEC:%=objs/${RTIMEARCH}/%.o) \
               $(EXEC:%=objs/${RTIMEARCH}/%.${OUTSUFFIX})

objs/${RTIMEARCH}/%.${OUTSUFFIX} : objs/${RTIMEARCH}/%.o
	${OUTCOMMAND}

objs/${RTIMEARCH}/%.o : %.c
	${CC} ${CFLAGS}  -o $@ $(DEFINES) $(INCLUDES) -c $<

# Rule to rebuild the generated files when the .idl file change
$(IDL_SRCS) $(IDL_HEADERS): HelloWorld.idl
	$(RTIMEHOME)/rtiddsgen/scripts/rtiddsgen -replace -micro -language C HelloWorld.idl

generate: $(IDL_SRCS) $(IDL_HEADERS)

# Here is how we create those subdirectories automatically.
%.dir :
	${MKDIRCOMMAND}

