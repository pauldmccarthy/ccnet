gitrev=$(shell git log -1 --abbrev-commit --format='%h (%ad)' --date=iso8601)

CFLAGS  = -std=gnu99
CFLAGS += -Wall
CFLAGS += -pedantic 
CFLAGS += -I.
CFLAGS += -DGIT_REVISION="\"$(gitrev)\""
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -O3
LDFLAGS = -lm

# Add macports lib locations, and getline 
# hack, on OS X. OSTYPE is defined by 
# default, but needs to be exported, 
# otherwise it won't be picked up by make
ifeq ($(OSTYPE), darwin10.0)
  LDFLAGS += -L/opt/local/lib -largp
  CFLAGS  += -I/opt/local/include
  CFLAGS  += -Dgetline=cnet_getline
  CFLAGS  += -Dgetdelim=cnet_getdelim
endif

sources    = $(wildcard io/*.c) 
sources   += $(wildcard graph/*.c)
sources   += $(wildcard stats/*.c)
sources   += $(wildcard util/*.c)
sources   += $(wildcard timeseries/*.c)
objtargets = $(sources:.c=.o)
objfiles   = $(addprefix obj/,$(notdir $(objtargets)))

exes=dumpimg    \
     dumphdr    \
     cnvnifti   \
     extval     \
     shiftimg   \
     scaleimg   \
     avgimg     \
     cnvimg     \
     catimg     \
     cutimg     \
     countimg   \
     cropimg    \
     mkhdr      \
     nanfiximg  \
     patchhdr   \
     cprune     \
     cextract   \
     creduce    \
     cmerge     \
     cgen       \
     ceo        \
     cdot       \
     cnet       \
     ctrim      \
     cvtk       \
     ngdb2img   \
     dumpvolume \
     tsmat      \
     dumpmat    \
     tsgraph    \
     dumpngdb   \
     labelngdb  \
     tsgen      \
     repimg


default: $(exes)

%.o: %.c
	@mkdir -p obj
	gcc $(CFLAGS) -c -o obj/$(@F) $<

$(exes): $(objtargets)
	@mkdir -p bin
	gcc $(CFLAGS) $(LDFLAGS) -o bin/$@ $@.c $(objfiles)

clean:
	rm -rf bin obj
