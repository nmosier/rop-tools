ROPC=./ropc
LANG_TARGETS=gadasm/stack.gds gadasm/mov.gds gadasm/jmp.gds gadasm/arith.gds gadasm/init.gds gadasm/cmp.gds gadasm/mem.gds gadasm/call.gds
CONFIG=tst.cfg

%.sc: $(LANG_TARGETS) %.gds
	$(ROPC) -c $(CONFIG) -o $@ $^
