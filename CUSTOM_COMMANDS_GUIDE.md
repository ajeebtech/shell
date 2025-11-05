# Custom Commands Guide

## All Commands Are Now Custom!

Your shell **ONLY uses custom implementations** - no original system commands!

## Custom Built-in Commands

### ✅ `add_alias` (replaces `alias`)
Create or list aliases:

```bash
# Create an alias
sandbox> add_alias ll=ls
sandbox> add_alias greet='echo Hello'

# List all aliases
sandbox> add_alias

# Original 'alias' is BLOCKED
sandbox> alias
[SANDBOX BLOCKED] Command 'alias' is not allowed (use 'add_alias' instead)
```

### ✅ `remove_alias` (replaces `unalias`)
Remove an alias:

```bash
sandbox> remove_alias ll
sandbox> remove_alias greet

# Original 'unalias' is BLOCKED
sandbox> unalias ll
[SANDBOX BLOCKED] Command 'unalias' is not allowed (use 'remove_alias' instead)
```

### ✅ `print_history` (replaces `history`)
Show command history:

```bash
sandbox> print_history

# Original 'history' is BLOCKED
sandbox> history
[SANDBOX BLOCKED] Command 'history' is not allowed (use 'print_history' instead)
```

### ✅ Other Custom Built-ins
- `cd` - Change directory
- `exit` - Exit shell
- `help` - Show help
- `stats` - Show sandbox statistics
- `commands` - List all commands

## Custom Sandbox Commands

All external commands use **your custom implementations** from `sandbox/bin/`:

- `ls` - Custom implementation
- `cat` - Custom implementation
- `echo` - Custom implementation
- `pwd` - Custom implementation
- `touch` - Custom implementation
- `mkdir` - Custom implementation
- `wc` - Custom implementation
- `grep` - Custom implementation

## Blocked Original Commands

These original commands are **BLOCKED**:

- ❌ `alias` → Use `add_alias`
- ❌ `unalias` → Use `remove_alias`
- ❌ `history` → Use `print_history`
- ❌ Any command not in `sandbox/bin/` → BLOCKED

## Quick Reference

### Creating Aliases
```bash
# Method 1: With quotes
sandbox> add_alias ll='ls'
sandbox> add_alias ll="ls"

# Method 2: Without quotes
sandbox> add_alias ll=ls

# Method 3: With flags
sandbox> add_alias ll='ls -l'
```

### Managing Aliases
```bash
# List all aliases
sandbox> add_alias

# Remove an alias
sandbox> remove_alias ll

# Test an alias
sandbox> ll    # Runs: ls
```

### Viewing History
```bash
sandbox> print_history
```

## Examples

```bash
# Create shortcuts
sandbox> add_alias ll=ls
sandbox> add_alias cls='clear'
sandbox> add_alias findtxt='grep txt'

# Use them
sandbox> ll
sandbox> cls
sandbox> findtxt test.txt

# List all aliases
sandbox> add_alias

# Remove one
sandbox> remove_alias cls

# View history
sandbox> print_history
```

## Why Custom Names?

- **`add_alias`** instead of `alias` - Makes it clear it's our implementation
- **`remove_alias`** instead of `unalias` - Consistent naming
- **`print_history`** instead of `history` - Our custom version

All commands are **100% custom** - no original system commands! 🎉

