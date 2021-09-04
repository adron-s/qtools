CC       = gcc
LIBS     = -lreadline
CFLAGS   = -O2 -Wno-unused-result -Wunused

OBJS     = hdlc.o qcio.o memio.o chipconfig.o
BINS     = qcommand qrmem qrflash qdload mibibsplit qwflash qwdirect qefs qnvram qblinfo qident qterminal qbadblock qflashparm

.PHONY: all clean

all: $(BINS)

%.o: %.c include.h
	$(CC) -c $(CFLAGS) $(filter %.c,$^) -o $@

$(BINS): %: %.o $(OBJS)
	$(CC) $(LDFLAGS) $(filter %.o,$^) -o $@ $(LIBS)

clean:
	rm -f *.o
	rm -f $(BINS)

qefs : efsio.o
qdload : sahara.o
qdload qrflash qwdirect qbadblock : ptable.o
