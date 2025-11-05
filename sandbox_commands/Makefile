CC = gcc
CFLAGS = -Wall -std=c99
SANDBOX_BIN = ../sandbox/bin

# All sandbox commands
COMMANDS = sandbox_ls sandbox_cat sandbox_echo sandbox_pwd \
           sandbox_touch sandbox_mkdir sandbox_wc sandbox_grep

all: $(COMMANDS)
	@mkdir -p $(SANDBOX_BIN)
	@echo "Installing sandbox commands to $(SANDBOX_BIN)..."
	@for cmd in $(COMMANDS); do \
		if [ "$$cmd" = "sandbox_echo" ]; then \
			cp $$cmd $(SANDBOX_BIN)/display; \
		else \
			cp $$cmd $(SANDBOX_BIN)/$${cmd#sandbox_}; \
		fi \
	done
	@echo "✓ Sandbox commands installed!"

sandbox_ls: sandbox_ls.c
	$(CC) $(CFLAGS) -o sandbox_ls sandbox_ls.c

sandbox_cat: sandbox_cat.c
	$(CC) $(CFLAGS) -o sandbox_cat sandbox_cat.c

sandbox_echo: sandbox_echo.c
	$(CC) $(CFLAGS) -o sandbox_echo sandbox_echo.c

sandbox_pwd: sandbox_pwd.c
	$(CC) $(CFLAGS) -o sandbox_pwd sandbox_pwd.c

sandbox_touch: sandbox_touch.c
	$(CC) $(CFLAGS) -o sandbox_touch sandbox_touch.c

sandbox_mkdir: sandbox_mkdir.c
	$(CC) $(CFLAGS) -o sandbox_mkdir sandbox_mkdir.c

sandbox_wc: sandbox_wc.c
	$(CC) $(CFLAGS) -o sandbox_wc sandbox_wc.c

sandbox_grep: sandbox_grep.c
	$(CC) $(CFLAGS) -o sandbox_grep sandbox_grep.c

clean:
	rm -f $(COMMANDS)
	rm -rf $(SANDBOX_BIN)

.PHONY: all clean

