# Project Axiom — Built from scratch. Engineered for the future.
# Copyright (c) 2026 Omid Teimory. All rights reserved.

.PHONY: all clean dll test test_loss

all:
	@$(MAKE) -C core all

clean:
	@$(MAKE) -C core clean

dll:
	@$(MAKE) -C core dll

test:
	@$(MAKE) -C core test

test_loss:
	@$(MAKE) -C core test_loss

%:
	@$(MAKE) -C core $@

