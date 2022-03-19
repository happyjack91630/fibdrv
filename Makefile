CONFIG_MODULE_SIG = n
TARGET_MODULE := fibdrv

obj-m := $(TARGET_MODULE).o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CPUID =  7
GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS) client
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) client out
	rm -f time_result
	rm -f plot_statistic.png
	rm -f client_kernel_user
	rm -f client_double_seq
load:
	sudo insmod $(TARGET_MODULE).ko
unload:
	sudo rmmod $(TARGET_MODULE) || true >/dev/null

client: client.c
	$(CC) -o $@ $^

client_plot: client_plot.c
	$(CC) -o $@ $^ -lm

client_kernel_user: client_kernel_user.c
	$(CC) -o $@ $^ -lm

client_double_seq: client_double_seq.c
	$(CC) -o $@ $^ -lm


PRINTF = env printf
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m
pass = $(PRINTF) "$(PASS_COLOR)$1 Passed [-]$(NO_COLOR)\n"

check: all
	$(MAKE) unload
	$(MAKE) load
#	sudo ./client > out
	sudo taskset -c $(CPUID) ./client > out
	$(MAKE) unload
	@scripts/verify.py

plot_kernel_user: all
	$(MAKE) unload
	$(MAKE) load
	$(MAKE) client_kernel_user
	rm -f time_result
	sudo taskset -c $(CPUID) ./client_kernel_user
	gnuplot scripts/plot_result.gp
	$(MAKE) unload

plot_seq_doubling: all
	$(MAKE) unload
	$(MAKE) load
	$(MAKE) client_double_seq
	rm -f time_result
	sudo taskset -c $(CPUID) ./client_double_seq
	gnuplot scripts/plot_result.gp
	$(MAKE) unload

