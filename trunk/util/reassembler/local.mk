# util/reassembler/local.mk

TARGET = reassembler
LOCAL_DEFS = -DTARGET='"$(TARGET)"'
HDRS = announce.h assemble.h decode.h edge.h fatal.h feeder.h files.h headers.h lock.h options.h receive.h status.h sweepers.h
SRCS = announce.c assemble.c decode.c edge.c fatal.c feeder.c files.c headers.c lock.c main.c merge.c receive.c skim.c status.c
OBJS = announce.o assemble.o decode.o edge.o fatal.o feeder.o files.o headers.o lock.o main.o merge.o receive.o skim.o status.o
LIBS = $(SRCDIR)/mstore/mime-api.o $(SRCDIR)/general/strcase.o $(SRCDIR)/general/dputil.o $(SRCDIR)/general/libdynadt.a $(SRCDIR)/general/libexcept.a
JUNK = $(OJBS) $(TARGET)


$(TARGET): $(OBJS) $(LIBS)
	$(PRELOAD) $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)


TAGS: $(HDRS) $(SRCS)
	etags $^
