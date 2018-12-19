#########################
# customise these
CFILES := Cache_Coherence.c
PROG := Cache_Coherence
CFLAGS := -Wall -Wextra -g
LDFLAGS :=
########################

# -MMD generates dependencies while compiling
CFLAGS += -MMD
CC := gcc -w -lm

OBJFILES := $(CFILES:.c=.o)
DEPFILES := $(CFILES:.c=.d)

$(PROG) : $(OBJFILES)
		$(LINK.o) $(LDFLAGS) -o $@ $^

clean :
		rm -f $(PROG) $(OBJFILES) $(DEPFILES)

-include $(DEPFILES)