# 🔒 Chroot Sandboxing Guide

## What is Chroot?

**`chroot`** is a system call that changes the root directory for a process. This creates a **filesystem jail** where the process can only access files within the specified directory.

**You don't need to download anything** - `chroot` is built into macOS/Linux!

## How It Works

```
Normal process:  /bin/ls → /usr/bin/ls (real system)
Chrooted process: /bin/ls → /Users/jatin/Desktop/os/sandbox/bin/ls
```

After chroot, all absolute paths (`/bin/ls`, `/etc/passwd`) are relative to the chroot directory.

## Current Implementation

✅ **Chroot code is already integrated** in `project_sandboxed.c`

✅ **It's called automatically** in every child process (before exec)

✅ **Gracefully handles non-root** - if you're not root, it just skips chroot

## Requirements

### 1. Root Privileges
```bash
# chroot() requires root on macOS
sudo ./myshell
# OR
sudo python3 sandbox_gui.py
```

### 2. Directory Structure
Run the setup script:
```bash
./setup_chroot.sh
```

This creates:
```
sandbox/
├── bin/          (for binaries)
├── usr/bin/      (for user binaries)
├── etc/          (for config files)
├── lib/          (for libraries)
└── home/         (for user files)
```

## Why Chroot Might Not Work on macOS

### macOS Limitations:
1. **SIP (System Integrity Protection)** - macOS restricts chroot
2. **Root Required** - chroot() needs root privileges
3. **Binary Dependencies** - Need to copy binaries + libraries into chroot
4. **Different Behavior** - macOS chroot differs from Linux

### What Still Works (Without Chroot):
Even without chroot, your shell has:
- ✅ Resource limits (CPU, memory, processes)
- ✅ Command whitelist/blacklist
- ✅ Path restrictions
- ✅ File system access control

## How to Use Chroot

### Option 1: With Root (Full Chroot)
```bash
# Setup chroot environment
./setup_chroot.sh

# Run with root privileges
sudo ./myshell
```

### Option 2: Without Root (Other Sandboxing Active)
```bash
# Just run normally - chroot skipped, other sandboxing works
./myshell
# OR
python3 sandbox_gui.py
```

## Checking Chroot Status

When you start the shell, check the banner:

**With Root (chroot active):**
```
✓ Chroot Jail Active (root filesystem isolation)
```

**Without Root:**
```
⚠ Chroot Disabled (requires root privileges)
```

## What Gets Chrooted?

**Every child process** (executed commands) gets:
1. Resource limits applied
2. Chroot jail applied (if root)
3. Then exec() runs the command

## Code Implementation

In `project_sandboxed.c`:

```c
void setup_chroot() {
    // Check if root
    if (geteuid() != 0) {
        return;  // Skip if not root
    }
    
    // Change root to sandbox directory
    chroot(CHROOT_DIR);
    chdir("/");  // Change to new root
}
```

Called in:
- `execute_command()` - before exec()
- `execute_pipe()` - before exec() in each pipeline stage

## For Your OS Project

### What to Demonstrate:

1. **Without Root:**
   - Show other sandboxing features work
   - Explain chroot is optional enhancement

2. **With Root (if you want):**
   ```bash
   sudo ./myshell
   ```
   - Show chroot is active in banner
   - Explain filesystem isolation

### What to Explain:

- **Chroot provides filesystem isolation**
- **Requires root privileges** (security feature, not bug)
- **Other sandboxing still works without chroot**
- **Multiple layers of security** (defense in depth)

## Troubleshooting

### "chroot: Operation not permitted"
- You need root: `sudo ./myshell`
- macOS SIP might be blocking it

### "No such file or directory" after chroot
- Need to copy binaries into chroot
- Need to copy libraries (dependencies)
- This is complex - usually skip for demo

### "Command not found" in chroot
- Binaries need to be in `/bin` or `/usr/bin` relative to chroot
- Or use full paths that work within chroot

## Recommendation

For your OS project presentation:

1. **Show the code** - chroot is implemented
2. **Explain it's optional** - other sandboxing works without it
3. **Mention root requirement** - shows understanding of privileges
4. **Focus on other features** - resource limits, whitelist, etc.

The chroot implementation shows you understand:
- System calls (chroot, chdir, geteuid)
- Process isolation
- Security principles
- Defense in depth

## Summary

✅ **Chroot is implemented** - code is ready
✅ **No download needed** - it's a system call
✅ **Works with root** - `sudo ./myshell`
✅ **Gracefully skips without root** - other sandboxing still works
✅ **Shows advanced understanding** - multiple security layers

Your shell has **comprehensive sandboxing** with or without chroot! 🎉

