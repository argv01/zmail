# util/compface/local.mk

GUI_LIBS =

COMP_OBJ   = file.$O arith.$O gen.$O compress.$O   cmain.$O   compface.$O
UNCOMP_OBJ = file.$O arith.$O gen.$O compress.$O uncmain.$O uncompface.$O

compface : compface.$O $(COMP_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(COMP_OBJ)   $(LIBRARIES) -o $@

uncompface : uncompface.$O $(UNCOMP_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(UNCOMP_OBJ) $(LIBRARIES) -o $@
