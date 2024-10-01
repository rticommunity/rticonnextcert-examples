# Optional variables:
# - DEBUG: If set to 'Y', it turns on debugging information
# - SHARED: If set to 'Y', shared libraries will be used

# Set RTIMEHOME to installation dir
ifeq ($(RTIMEHOME),)
    RTIMEHOME = ../../../..
endif

OS=$(shell uname -a)
GCCVERSION=$(shell gcc --version)

# Determine host arch automatically from OS, gcc version
ifeq ($(RTIMEARCH),)
	ifneq ($(findstring Darwin,$(OS)),)
		RTIMEARCH=i86Darwin10gcc4.2.1
	endif

	ifneq ($(findstring Linux,$(OS)),)
		ifneq ($(findstring 3.4.3,$(GCCVERSION)),)
			RTIMEARCH=i86Linux2.6gcc3.4.3
		endif
		ifneq ($(findstring 4.4.5,$(GCCVERSION)),)
			RTIMEARCH=i86Linux2.6gcc4.4.5
		endif
	endif

	ifneq ($(findstring sparc,$(OS)),)
		RTIMEARCH=sparcSol2.8gcc3.2
	endif
endif

# valid host should be set by here
ifeq ($(RTIMEARCH),)
$(error Unsupported host architecture)
endif

# Default architecture is 32 bits
M_FLAG=-m32
ifneq ($(findstring x86_64,$(RTIMEARCH)),)
        M_FLAG=-m64
endif

ifeq ($(DEBUG),Y)
LIBSUFFIX_DEBUG=d
CFLAGS += -g
endif
ifeq ($(SHARED),Y)
LIBSUFFIX=$(LIBSUFFIX_DEBUG)
else
LIBSUFFIX=z$(LIBSUFFIX_DEBUG)
endif

ARCH_LIBS= -lm

ifneq ($(findstring Darwin,$(RTIMEOS)),)
        CFLAGS = $(M_FLAG) -std=c99
        LFLAGS = $(M_FLAG)
        CC = gcc
        DEFINES = -DRTI_UNIX -DRTI_DARWIN -DRTI_POSIX_THREADS
endif
ifneq ($(findstring Linux,$(RTIMEOS)),)
        ARCH_LIBS+=-ldl -lnsl -lpthread -lrt
        CFLAGS = $(M_FLAG) -std=c99
        LFLAGS = $(M_FLAG) -static-libgcc
        CC = gcc
        DEFINES = -DRTI_UNIX -DRTI_LINUX -DRTI_POSIX_THREADS
endif
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

ifneq ($(findstring pentiumVx6.9gcc4.3.3,$(RTIMEARCH)),)
	DEFINES = -DRTI_VXWORKS -march=pentium -fno-builtin -DTOOL=gnu -DCPU=PENTIUM

	ifneq ($(findstring _rtp,$(RTIMEARCH)),)
                OUTSUFFIX = vxe
                DEFINES += -DRTI_RTP

                ifeq ($(SHARED),Y)
                    LFLAGS += -non-static
                endif

		ifneq ($(findstring Linux,$(OS)),)
	        	validHost = Y
			CC = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ccpentium
                	LD = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ldpentium
                	INCLUDES += -I$(WIND_USR)/h \
			            -I$(WIND_USR)/h/wrn/coreip
			CFLAGS += -mrtp
                        LFLAGS += -mrtp
			LIBS += -L$(WIND_BASE)/target/lib/usr/lib/pentium/PENTIUM/common
		endif
		OUTCOMMAND = $(CC) ${LFLAGS} -o $@ $(@:%.${OUTSUFFIX}=%.o) $(COMMONOBJS) $(LIBS)
	else
                ifeq ($(SHARED),Y)
                    LIBS =
                endif
        	OUTSUFFIX = so
		ifneq ($(findstring Linux,$(OS)),)
	        	validHost = Y
                	CC = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ccpentium
                	LD = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ldpentium
                	INCLUDES += -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/h/wrn/coreip
		endif
		OUTCOMMAND = $(LD) -r -o $@ $(@:%.${OUTSUFFIX}=%.o) $(COMMONOBJS) $(LIBS)
	endif

	ifeq ($(validHost),)
	    OS = $(shell uname)
            $(error Invalid host $(OS) for target $(RTIMEARCH))
        endif
else
ifneq ($(findstring ppc604Vx6.9gcc4.3.3,$(RTIMEARCH)),)
	DEFINES = -DRTI_VXWORKS -fno-builtin -mlongcall -DTOOL=gnu -DCPU=PPC32

	ifneq ($(findstring _rtp,$(RTIMEARCH)),)
		OUTSUFFIX = vxe
		DEFINES += -DRTI_RTP
                ifeq ($(SHARED),Y)
                    LFLAGS += -non-static
                endif

		ifneq ($(findstring Linux,$(OS)),)
	        	validHost = Y
			CC = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ccppc
                	LD = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ldppc
                	INCLUDES += -I$(WIND_USR)/h \
			            -I$(WIND_USR)/h/wrn/coreip
			CFLAGS += -mrtp
                        LFLAGS += -mrtp
			LIBS += -L$(WIND_BASE)/target/lib/usr/lib/ppc/PPC32/common
		endif
		OUTCOMMAND = $(CC) ${LFLAGS} -o $@ $< $(COMMONOBJS) $(LIBS)
	else
                ifeq ($(SHARED),Y)
                    LIBS =
                endif

        	OUTSUFFIX = so
		ifneq ($(findstring Linux,$(OS)),)
	        	validHost = Y
                	CC = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ccppc
                	LD = $(WIND_BASE)/../gnu/4.3.3-vxworks-6.9/x86-linux2/bin/ldppc
                	INCLUDES += -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/h/wrn/coreip
		endif
		OUTCOMMAND = $(LD) -r -o $@ $(@:%.${OUTSUFFIX}=%.o) $(COMMONOBJS) $(LIBS)
	endif

	ifeq ($(validHost),)
	    OS = $(shell uname)
            $(error Invalid host $(OS) for target $(RTIMEARCH))
        endif
else
ifeq ($(RTIMEARCH),ppc604Vx5.5gcc)
        DEFINES = -DRTI_VXWORKS -mcpu=604 -G 0 -fno-builtin -mlongcall -DCPU=PPC604
        OUTSUFFIX = so
        OUTCOMMAND = $(LD) -r -o $@ $(@:%.${OUTSUFFIX}=%.o) $(COMMONOBJS) $(LIBS)
        ifneq ($(findstring sparc,$(OS)),)
	        validHost = Y
                CC = $(WIND_BASE)/host/sun4-solaris2/bin/ccppc
                LD = $(WIND_BASE)/host/sun4-solaris2/bin/ldppc
	endif
        ifneq ($(findstring Win,$(OS)),)
	        validHost = Y
                CC = $(WIND_BASE)/gnu/3.3.2/x86-win32/bin/ccppc
                LD = $(WIND_BASE)/gnu/3.3.2/x86-win32/bin/ldppc
                INCLUDES += -I$(WIND_BASE)/vthreads-2.2.2/target/vThreads/h -I$(WIND_BASE)/vthreads-2.2.2/target/val/h -I$(WIND_BASE)/vthreads-2.2.2/target/vThreads/h/netinet
                DEFINES += -DVTHREADS
	endif

	ifeq ($(validHost),)
	    OS = $(shell uname)
            $(error Invalid host $(OS) for target $(RTIMEARCH))
        endif
else
        OUTSUFFIX = out
        OUTCOMMAND = ${CC} ${LFLAGS} -o $(@:%.${OUTSUFFIX}=%) $(@:%.${OUTSUFFIX}=%.o) $(COMMONOBJS) $(LIBS)
endif
endif
endif

ifneq ($(findstring WIN,$(OS)),)
        MKDIRCOMMAND = if not exist $* ( mkdir "$*" );
else
        MKDIRCOMMAND = @if [ ! -d $* ]; then echo "Making directory $*"; mkdir -p $* ; fi;
endif

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

ifeq ($(OS_ARCH), i86Win32)
	call $(RTIMEHOME)/rtiddsgen/scripts/rtiddsgen.bat -replace -micro -language C HelloWorld.idl
else
	$(RTIMEHOME)/rtiddsgen/scripts/rtiddsgen -replace -micro -language C HelloWorld.idl
endif

generate: $(IDL_SRCS) $(IDL_HEADERS)

# Here is how we create those subdirectories automatically.
%.dir :
	${MKDIRCOMMAND}

