#==================BUILD SETTINGS==================#

PLAT= 
CC= gcc
CFLAGS= -O2 -std=c99 -g
LDFLAGS=
SG_E= 
LIBS= 

MKCLIFLAGS= "--no-print-directory"
PLATS := linux mingw Darwin
SG_N= stormground

RESOURCE=resource.rc
RESOBJ=

ALL= all

SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))
SHADERS := $(wildcard shaders/*.vert) $(wildcard shaders/*.frag)
SHADEROBJS := $(patsubst %, %.o, $(SHADERS))
EXTOBJECTS= ../lib/cJSON/cJSON.o ../lib/glad.o ../lib/hydrogen/hydrogen.o

$(if $(shell which clang-format), $(eval CFMT=clang-format), $(info clang-format isnt present and will not format code))

#==================TARGET SETTINGS=================#

target: $(PLAT)

Linux linux:
	@echo Building $(SG_N) for linux
	$(MAKE) $(MKCLIFLAGS) $(ALL) PLAT=linux SG_E=$(SG_N) LIB_PREF=lib LIB_EXT=.a CFLAGS="-O2 -std=c11 -I../include -I../lib" LIBS="-llua -lglfw3 -lGL -lpthread -lX11 -lXrandr -lXi -ldl -lm" 

Darwin:
	@echo Building $(SG_N) src for Darwin/Mac
	xxd -i shaders/main.vert > shaders/main.vert.c
	xxd -i shaders/main.frag > shaders/main.frag.c
	xxd -i shaders/main.comp > shaders/main.comp.c
	xxd -i shaders/display.frag > shaders/display.frag.c
	xxd -i shaders/display.vert > shaders/display.vert.c
	$(MAKE) $(MKCLIFLAGS) $(ALL) PLAT=Darwin SG_E=$(SG_N) LIB_PREF=lib LIB_EXT=.a \
	CFLAGS="-O2 -std=c11 -I../include -I../lib -g" \
	LIBS="-llua -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreFoundation -lm"

mingw:
	@echo Building $(SG_N).exe for Windows
	$(MAKE) $(MKCLIFLAGS) $(ALL) PLAT=mingw SG_E=$(SG_N).exe RESOBJ=resource.res LIB_EXT=.lib CC="x86_64-w64-mingw32-gcc" CFLAGS="-O2 -std=c99 -Wno-overflow -I../include -I../lib" LIBS="-llua -lglfw3mingw -lm -lopengl32 -lgdi32 -ldwmapi"

#======================BUILD=======================#

echo:
	@echo PLAT=$(PLAT)
	@echo CC=$(CC)
	@echo CFLAGS=$(CFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo SG_E=$(SG_E)
	@echo LIBS=$(LIBS)

resource:
	x86_64-w64-mingw32-windres resource.rc -O coff resource.res

all: $(OBJECTS) $(SHADEROBJS) $(RESOBJ)
	$(CC) -L../lib -L../build $(OBJECTS) $(SHADEROBJS) $(EXTOBJECTS) $(RESOBJ) -o $(SG_E) $(LIBS)
	@mv $(SG_E) ../build/$(SG_E)

# Shader sources
SHADERSRC = $(wildcard shaders/*.vert) $(wildcard shaders/*.frag)
# Remove .comp from SHADERSRC if it was included by wildcard
SHADERSRC := $(filter-out shaders/main.comp, $(SHADERSRC))

# Shader objects
SHADEROBJS = $(patsubst shaders/%.vert, shaders/%.vert.o, $(filter %.vert, $(SHADERSRC))) \
             $(patsubst shaders/%.frag, shaders/%.frag.o, $(filter %.frag, $(SHADERSRC)))

ifeq ($(PLAT),Darwin)
# Ensure .o files depend on .c files, and .c files depend on shader sources
$(SHADEROBJS): %.o : %.c

shaders/%.c: shaders/%
	xxd -i $< > $@

shaders/%.o: shaders/%.c
	$(CC) $(CFLAGS) -c $< -o $@
else
shaders/%.o: shaders/%
	ld -r -b binary $< -o $@
endif

%.res: %.rc
	x86_64-w64-mingw32-windres -O coff -o $@ $<
	

clean:
	rm -f $(OBJECTS) $(SG_E) shaders/*.o *.res shaders/*.vert.c shaders/*.frag.c

.PHONY: target format

sg.o: sg.c sg.h sgshader.h sginput.h sgcli.h 
sginput.o: sginput.c sginput.h sg.h
sgcli.o: sgcli.c sgcli.h sg.h
sgshader.o: sgshader.c sgshader.h sg.h
sgapi.o: sgapi.c sgapi.h sg.h
sgimage.o: sgimage.c sgimage.h sg.h
#end of file