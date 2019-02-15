ROPC=./ropc
LANG_TARGETS=gadasm/stack.gds gadasm/mov.gds gadasm/jmp.gds gadasm/arith.gds gadasm/init.gds gadasm/cmp.gds gadasm/mem.gds gadasm/call.gds
CONFIG=tst.cfg

two-stage.sc: gadasm/multistage.gds two-stage.gds $(LANG_TARGETS)
	$(ROPC) -n2 -c $(CONFIG) -o $@ $^

%.sc: gadasm/multistage.gds $(LANG_TARGETS) %.gds
	$(ROPC) -n2 -c $(CONFIG) -o $@ $^
