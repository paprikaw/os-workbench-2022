NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
INC_PATH       := include/ framework/

export AM_HOME := /workspaces/NJU_OS/abstract-machine
ifeq ($(ARCH),)
export ARCH    := x86_64-qemu
endif

include $(AM_HOME)/Makefile
include ../Makefile.lab
# image: git
# test: 
# 	@gcc $(shell find src/ -name "*.c") \
# 	    $(shell find test/ -name "*.c") \
# 	-Iframework -Itest -DTEST -lpthread \
# 	-o build/test
# 	@build/test
# .PHONY: test

testall: 
	@gcc src/pmm.c \
	    $(shell find test/ -name "*.c") \
	-ggdb \
	-Iframework -Itest -DTEST -lpthread \
	-o build/test
	@build/test 0

.PHONY: testall