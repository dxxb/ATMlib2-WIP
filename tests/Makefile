
ATMLIB_PATH = $(PWD)/src

ATMLIB_SRC = atm_synth.c atm_pool.c cmd_parse.c atm_log.c
STDOUT_DEMO_SRC = test_synth.c osc_stdout.c
CFLAGS += -O0 -I$(ATMLIB_PATH) -g

SRCS = $(addprefix $(ATMLIB_PATH)/,$(ATMLIB_SRC)) $(STDOUT_DEMO_SRC)

all: runtests

runtests: .depend test_synth
	python3 -m unittest discover -v

test_synth: $(SRCS) | .depend Makefile
	$(CC) $(CFLAGS) -o $@ $^

-include .depend

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MD $^ -MF ./.depend

clean:
	rm test_synth

.PHONY: all depend clean
