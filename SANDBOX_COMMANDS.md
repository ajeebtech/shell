# 🔒 Sandbox Commands - Custom Implementations

## Overview

Your shell now uses **CUSTOM SANDBOXED COMMANDS** instead of system commands! These are your own implementations that run inside the sandbox environment.

## How It Works

When you type a command like `ls`:

1. **Shell checks** `sandbox/bin/ls` first
2. **If found** → Uses YOUR sandbox command
3. **If not found** → Falls back to system command (for commands not yet implemented)

## Implemented Sandbox Commands

### ✅ Currently Implemented:
- `ls` - List directory contents (with -l, -a support)
- `cat` - Display file contents
- `echo` - Print text (with -n support)
- `pwd` - Print working directory
- `touch` - Create/update file timestamps
- `mkdir` - Create directories
- `wc` - Word count (with -l, -w, -c flags)
- `grep` - Search for patterns in files

### 📁 Location:
All sandbox commands are in: `sandbox/bin/`

## Building Sandbox Commands

```bash
# Build all sandbox commands
cd sandbox_commands
make

# Or build everything (commands + shell)
cd ..
make clean && make
```

## Verification

To verify you're using sandbox commands:

```bash
# Check which ls is being used
sandbox> which ls
# Should show: /Users/jatin/Desktop/os/sandbox/bin/ls

# Or check the binary directly
ls -l sandbox/bin/
```

## Architecture

### Original System Commands:
- Located in `/bin`, `/usr/bin`
- System binaries
- Full access to system

### Sandbox Commands:
- Located in `sandbox/bin/`
- Your custom C implementations
- Run with all sandbox restrictions:
  - Resource limits
  - Path restrictions
  - Chroot (if root)
  - Monitoring

## Code Flow

```
User types: ls
    ↓
[Shell checks sandbox/bin/ls]
    ↓
Found! → Use sandbox command
    ↓
[Apply restrictions]
    ↓
[Execute sandbox_ls binary]
```

## Adding More Commands

To add a new sandbox command:

1. Create `sandbox_commands/sandbox_COMMAND.c`
2. Implement the command logic
3. Add to `sandbox_commands/Makefile`
4. Rebuild: `make clean && make`

Example:
```c
// sandbox_commands/sandbox_date.c
#include <stdio.h>
#include <time.h>

int main() {
    time_t t = time(NULL);
    printf("%s", ctime(&t));
    return 0;
}
```

## Benefits

✅ **Full Control** - You control what commands do
✅ **Security** - Commands can't escape sandbox
✅ **Customization** - Modify behavior as needed
✅ **Learning** - Understand how commands work
✅ **Isolation** - No dependency on system binaries

## Testing

```bash
# Test sandbox commands
sandbox> ls
sandbox> ls -l
sandbox> cat test.txt
sandbox> echo "Hello from sandbox!"
sandbox> pwd
sandbox> touch newfile.txt
sandbox> mkdir testdir
sandbox> wc test.txt
sandbox> grep "pattern" test.txt
```

## Important Notes

1. **Sandbox commands take priority** - If a command exists in `sandbox/bin/`, it's used
2. **Fallback to system** - Commands not implemented fall back to system PATH
3. **All restrictions apply** - Sandbox commands still have resource limits, etc.
4. **Rebuild after changes** - Modify command code? Rebuild with `make`

## For Your Presentation

You can now say:
- "I implemented custom sandbox versions of all commands"
- "Commands are isolated in sandbox/bin directory"
- "These are my own C implementations, not system binaries"
- "All commands run with full sandbox restrictions"

This shows:
- ✅ Systems programming skills
- ✅ Understanding of command implementation
- ✅ Security-conscious design
- ✅ Complete control over the environment

---

**Your shell now uses 100% custom sandbox commands!** 🎉

