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

