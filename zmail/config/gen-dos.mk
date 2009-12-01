# gen-dos.mk	Copyright 1992 Z-Code Software Corp.
#
# General defintions for DOS builds of Z-Mail, which tie together the DOS
# definitions (def-dos.mk) and the root source definitions (zm-root.mk).
#

ROOT_HDRS = $(SHELL_HDRS) $(CHILD_HDRS) $(DOS_HDRS) $(LICENSE_HDRS)
ROOT_SRC = $(MESSAGE_SRC) $(SHELL_SRC) $(CHILD_SRC) $(DOS_SRC) $(LICENSE_SRC)
ROOT_OBJ = $(MESSAGE_OBJ) $(SHELL_OBJ) $(CHILD_OBJ) $(DOS_OBJ) $(LICENSE_OBJ)
