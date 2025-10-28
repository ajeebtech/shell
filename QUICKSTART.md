# 🚀 Quick Start Guide

## Run the Sandboxed Shell (3 Simple Steps)

### Step 1: Build the Shell
```bash
make
```

### Step 2: Run with GUI
```bash
python3 sandbox_gui.py
```

### Step 3: Try it out!
Type commands in the GUI:
```
help
ls
echo "Hello Sandbox!"
mkdir test
cd test
pwd
stats
```

---

## What You'll See

### The GUI Window Shows:
- 🔒 **Orange border** = Sandboxed environment active
- **Header** with security features enabled
- **Terminal area** with color-coded output
- **Input line** at the bottom
- **Real-time statistics** (runtime, commands executed/blocked)

### Color Meanings:
- 🟢 **Green** = Normal output
- 🔵 **Cyan** = Shell prompt
- 🔴 **Red** = Errors or blocked commands
- 🟡 **Yellow** = Warnings
- 🟠 **Orange** = Sandbox indicators

---

## Try These Examples

### Basic Commands
```bash
sandbox> ls
sandbox> pwd
sandbox> whoami
sandbox> date
```

### Create Files
```bash
sandbox> echo "Test" > test.txt
sandbox> cat test.txt
```

### Try Pipelines
```bash
sandbox> ls | grep txt
sandbox> echo "line1\nline2\nline3" | wc -l
```

### Test Security (These will be blocked!)
```bash
sandbox> sudo ls        # ❌ Blocked - dangerous command
sandbox> rm -rf /       # ❌ Blocked - dangerous command
sandbox> cd /etc        # ❌ Blocked - outside sandbox
```

### View Statistics
```bash
sandbox> stats
```

---

## Running Without GUI (Terminal Only)

If you want to run the shell directly in your terminal:

```bash
./myshell
```

You'll see the same sandbox features, just without the fancy GUI!

---

## Keyboard Shortcuts in GUI

- **Enter** - Execute command
- **Backspace** - Delete character
- **↑ / ↓** - Scroll output
- **Page Up/Down** - Scroll faster
- **Close window** - Exit

---

## Troubleshooting

### "myshell not found"
```bash
make clean && make
```

### "pygame not found"
```bash
pip3 install pygame
```

### "No module named 'pygame'"
```bash
python3 -m pip install pygame
```

---

## What Makes It Sandboxed?

1. ✅ **CPU Limit**: Max 30 seconds per process
2. ✅ **Memory Limit**: Max 100 MB per process
3. ✅ **Process Limit**: Max 20 concurrent processes
4. ✅ **File Limit**: Max 64 open files
5. ✅ **Command Whitelist**: Only safe commands allowed
6. ✅ **Command Blacklist**: Dangerous commands blocked
7. ✅ **Path Restriction**: Can only access `sandbox/` directory

---

## Files in This Project

- `project_sandboxed.c` - Sandboxed shell (C code)
- `sandbox_gui.py` - Visual terminal interface (Python)
- `myshell` - Compiled executable
- `sandbox/` - Your working directory (restricted)
- `README.md` - Full documentation
- `makefile` - Build system

---

## Need Help?

Type `help` in the shell to see all available commands!

```bash
sandbox> help
```

---

**Enjoy your sandboxed shell! 🔒**

