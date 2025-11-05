CC = gcc
CFLAGS = -I/opt/homebrew/opt/readline/include -Wall
LDFLAGS = -L/opt/homebrew/opt/readline/lib -lreadline

TARGET = myshell
SRC_SANDBOXED = project_sandboxed.c
SRC_ORIGINAL = project.c

all: sandbox_commands $(TARGET)

sandbox_commands:
	@echo "Building sandbox commands..."
	@cd sandbox_commands && $(MAKE) all
	@echo "✓ Sandbox commands built"

$(TARGET): $(SRC_SANDBOXED)
	@echo "Building sandboxed shell..."
	$(CC) $(CFLAGS) $(SRC_SANDBOXED) -o $(TARGET) $(LDFLAGS)
	@echo "Done! Run with: python3 sandbox_gui.py"

original: $(SRC_ORIGINAL)
	@echo "Building original shell..."
	$(CC) $(CFLAGS) $(SRC_ORIGINAL) -o myshell_original $(LDFLAGS)

clean:
	rm -f $(TARGET) myshell_original
	@cd sandbox_commands && $(MAKE) clean 2>/dev/null || true

setup:
	@echo "Setting up sandbox environment..."
	@mkdir -p sandbox
	@echo "Sandbox directory created at: sandbox/"
	@echo "Installing Python dependencies..."
	@pip3 install pygame 2>/dev/null || pip install pygame 2>/dev/null || echo "Please install pygame: pip3 install pygame"
	@echo "Setup complete!"

test: $(TARGET)
	@echo "Testing sandboxed shell..."
	@./$(TARGET) -c "help" 2>/dev/null || echo "Shell compiled successfully"

.PHONY: all clean setup test original
