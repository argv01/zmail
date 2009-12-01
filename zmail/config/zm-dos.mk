# zm-dos.mk	Copyright 1992 Z-Code Software Corp.

#
# Definitions
#

LDFLAGS = -maxreal 0ffffh -stack 96000 -maxdata 0 -dosorder -twocase -pack
LDDBFLAGS =

#
# Default targets
#

# For DOS, using HC386, we create a file which sidesteps the 128-char limit
# of DOS command lines.
#
all : $(ZMAIL)

$(ZMAIL) : $(OBJECT)
	@echo loading $(ZMAIL)...
	@echo $(OBJECT) $(LIB_PATHS) $(SHELL_LIBS) $(GUI_LIBS) \
		$(LOCAL_LIBS) $(LDFLAGS) > $(TARGET,R).lnk
	@$(CC) @$(TARGET,R).lnk -o $(TARGET)
	@rm $(TARGET,R).lnk

lint :
	@echo Go buy PC-Lint.

clean :
	nuke -r -v *.obj        # core!?  What core?

tags :  $(SRCS) $(HDRS)
	@echo Updating tags file...
	!foreach file $?
		@ctags $(file)
	!end

#
# Create links
#

motif/gui_api.c :	gui/gui_api.c
	cp $? $@
motif/m_frame.c :	gui/zm_frame.c
	cp $? $@
olit/gui_api.c :	gui/gui_api.c
	cp $? $@
olit/o_frame.c :	gui/zm_frame.c
	cp $? $@

#
# Special Targets
#

.c.$O :
	@%set CMD128=$(CC) $(CFLAGS) $(ZCFLAGS) -c $*.c
	!if length(CMD128) < 128
		$(CC) $(CFLAGS) $(ZCFLAGS) -c $*.c
	!else
		@echo $(ZCFLAGS) -c > hctmp
		@echo $(CC) $(CFLAGS) $(ZCFLAGS) -c $*.c
		@$(CC) $(CFLAGS) @hctmp $*.c
	!endif
