FILENAME=vtekser.device
RELDIR=build-release
DEBUGDIR=build-debug
OBJECTS=unitybuild.o

SRCDIRS=.
INCDIRS=. /opt/amiga/m68k-amigaos/ndk-include/

TARGET = hunk-toolchain

ifeq ($(MAKECMDGOALS), debug)
 DIR=$(DEBUGDIR)
 EXTRA_CFLAGS+= -g -O0 -DDEBUG
 EXTRA_ASFLAGS+= -g
 EXTRA_LDFLAGS+= -ldebug
else
 DIR=$(RELDIR)
 EXTRA_CFLAGS+= -g -O2
endif

CC=/opt/amiga/bin/m68k-amigaos-gcc
AS=/opt/amiga/bin/m68k-amigaos-as
EXTRA_LDFLAGS+= -Wl,-Map=$(DIR)/$(FILENAME).map

CFLAGS+= -m68000 -Wall -Wextra -Wno-unused-parameter -Wno-volatile-register-var -fomit-frame-pointer -Wl,--gc-sections -ffunction-sections -fdata-sections
ASFLAGS+= -m68000
LDFLAGS+= -nostartfiles -lamiga

OBJS:=$(addprefix $(DIR)/,$(OBJECTS))

CFLAGS+=$(addprefix -I,$(INCDIRS)) $(EXTRA_CFLAGS)
ASFLAGS+=$(EXTRA_ASFLAGS)
LDFLAGS+=$(EXTRA_LDFLAGS)

# Search paths
vpath %.c $(SRCDIRS)
vpath %.s $(SRCDIRS)

release: $(TARGET)
debug: $(TARGET)

$(DIR)/$(FILENAME):
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

hunk-toolchain: $(DIR) $(OBJS) $(DIR)/$(FILENAME)
	/opt/amiga/bin/m68k-amigaos-objdump -DxS $(DIR)/$(FILENAME) > $(DIR)/$(FILENAME).s

$(DIR):
	@mkdir $(DIR)

$(DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@$(DEL_EXE)
	
$(DIR)/%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<
	@$(DEL_EXE)

RM= rm -rf
DEL_EXE=test -f && $(RM) $(DIR)/$(FILENAME)

.PHONY: release debug clean cleandebug cleanrelease hunk-toolchain

clean: cleandebug
clean: cleanrelease

cleandebug:
	$(info Cleaning debug)
	@$(RM) $(DEBUGDIR) $(DEVNULL)

cleanrelease:
	$(info Cleaning release)
	@$(RM) $(RELDIR) $(DEVNULL)
