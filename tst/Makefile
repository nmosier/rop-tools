ROPC=./ropc
LANG_TARGETS=gadasm/stack.gds gadasm/mov.gds gadasm/jmp.gds gadasm/arith.gds gadasm/init.gds gadasm/cmp.gds gadasm/mem.gds gadasm/call.gds
CONF_ECHO=echo.conf
CONF_ECHO3=echo3.conf

%.sc: gadasm/multistage.gds $(LANG_TARGETS) %.gds
	$(ROPC) -n 2 -c $(CONF_ECHO) -o $@ $^

%-2.sc: gadasm/multistage.gds $(LANG_TARGETS) %.gds
	$(ROPC) -n 2 -c $(CONF_ECHO3) -o $@ $^

%: %.c
	gcc -g -fno-stack-protector -o $@ $^

tests: echo echo3 fib.sc vulnerable.sc binomial-2+.sc
	( ./echo < fib.sc && ./echo3 vulnerable.sc < binomial-2+.sc && echo "tests passed" ) || "tests failed"

.PHONY: setup
setup:
	$(info Disabling ASLR...)
	echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

