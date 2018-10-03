
ATMLIB_PATH = $(PWD)/src

ATMLIB_SRC = atm_synth.c atm_pool.c atm_log.c
STDOUT_DEMO_SRC = test_synth.c osc_stdout.c
CFLAGS += -O0 -I$(ATMLIB_PATH) -g

SRCS = $(addprefix $(ATMLIB_PATH)/,$(ATMLIB_SRC)) $(STDOUT_DEMO_SRC)

WAV_FILES = $(patsubst %.atm,%.wav,$(ATM_FILES))

all: runtests

ifneq (clean, $(MAKECMDGOALS))
ifeq (wave_files, $(MAKECMDGOALS))
-include atm_test_files
endif
-include .depend
endif

runtests: test_synth $(wildcard *.py)
	python3 -m unittest discover -v

atm_test_files: | runtests
	@echo "ATM_FILES = \\" > atm_test_files
	@ls *.atm | xargs >> atm_test_files

wave_files: $(WAV_FILES) atm_test_files

%.wav: %.atm
	@echo "$^ -> $@"
	@./test_synth -s $^ 2>/dev/null | sox -traw -r16000 -b16 -c1 -esigned-integer -q - $@

test_synth: $(SRCS) .depend
	$(CC) $(CFLAGS) -o $@ $^

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MD $^ -MF ./.depend

clean:
	rm -f test_synth *.wav *.atm *.trace atm_test_files
	rm -f ./.depend

.PHONY: all wave_files runtests depend clean
