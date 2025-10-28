# Sandboxed Shell - OS Project

A custom Unix shell implementation with comprehensive sandboxing features and a visual terminal interface using pygame.

## 🔒 Security Features

### 1. **Resource Limits**
- **CPU Time**: 30 seconds per process
- **Memory**: 100 MB per process
- **Max Processes**: 20 concurrent processes
- **Open Files**: 64 file descriptors maximum

### 2. **Command Whitelist**
Only pre-approved commands are allowed to execute:
- File operations: `ls`, `cat`, `touch`, `mkdir`, `rmdir`, `cp`, `mv`
- Text processing: `grep`, `head`, `tail`, `wc`, `sort`, `uniq`
- System info: `pwd`, `date`, `whoami`, `hostname`, `which`
- Utilities: `echo`, `find`, `sleep`, `clear`

### 3. **Command Blacklist**
Dangerous commands are explicitly blocked:
- `sudo`, `su`, `rm`, `chmod`, `chown`
- `mkfs`, `dd`, `mount`, `umount`
- `reboot`, `shutdown`, `halt`, `init`
- `killall`, `pkill`

### 4. **File System Restrictions**
- All file operations are restricted to the `sandbox/` directory
- Prevents access to sensitive system files
- Read-only access to `/bin`, `/usr/bin` for executables

### 5. **Real-time Monitoring**
- Tracks commands executed and blocked
- Displays runtime statistics
- Visual indicators in GUI

## 📋 Prerequisites

- **macOS** (tested on macOS)
- **Xcode Command Line Tools** (for gcc)
- **Homebrew** (for readline library)
- **Python 3.7+**
- **pygame library**

### Install Dependencies

```bash
# Install readline (if not already installed)
brew install readline

# Install Python dependencies
pip3 install pygame

# Or use the provided requirements file
pip3 install -r requirements.txt
```

## 🚀 Quick Start

### 1. Setup and Build

```bash
# Setup sandbox environment and install dependencies
make setup

# Build the sandboxed shell
make

# Or build everything in one go
make clean setup all
```

### 2. Run with GUI

```bash
python3 sandbox_gui.py
```

### 3. Run without GUI (terminal only)

```bash
./myshell
```

## 🎮 Using the GUI

### Interface Layout

```
╔══════════════════════════════════════════════════╗
║  🔒 SANDBOXED SHELL ENVIRONMENT                  ║
║  ✓ Resource Limits  ✓ Command Whitelist         ║
║  ✓ Path Restrictions ✓ Monitoring Active        ║
╠══════════════════════════════════════════════════╣
║                                                  ║
║  Terminal Output Area                            ║
║  (Shows shell output with color coding)          ║
║                                                  ║
╠══════════════════════════════════════════════════╣
║  sandbox> [Type commands here]                   ║
╚══════════════════════════════════════════════════╝
```

### Keyboard Shortcuts

- **Enter**: Execute command
- **Backspace**: Delete character
- **↑/↓**: Scroll terminal output
- **Page Up/Down**: Scroll faster
- **Close window**: Exit shell

### Color Coding

- 🟢 **Green**: Normal output
- 🔵 **Cyan**: Shell prompts
- 🟡 **Yellow**: Warnings
- 🔴 **Red**: Errors and blocked commands
- 🟠 **Orange**: Sandbox indicators

## 📖 Available Commands

### Built-in Commands

- `help` - Show available commands
- `stats` - Display sandbox statistics
- `history` - Show command history
- `cd <dir>` - Change directory (within sandbox)
- `alias` - Create command aliases
- `unalias` - Remove aliases
- `exit` - Exit shell

### External Commands (Whitelisted)

```
ls      cat     echo    pwd     grep    touch
mkdir   rmdir   cp      mv      head    tail
wc      sort    uniq    find    which   date
whoami  hostname sleep  clear
```

### Examples

```bash
# Create and navigate directories
sandbox> mkdir test
sandbox> cd test
sandbox> pwd

# File operations
sandbox> echo "Hello World" > hello.txt
sandbox> cat hello.txt

# Pipelines work too!
sandbox> ls | grep txt | wc -l

# Try a blocked command
sandbox> sudo ls
[SANDBOX BLOCKED] Command 'sudo' is not allowed (security risk)

# View statistics
sandbox> stats
```

## 🏗️ Architecture

### Components

1. **project_sandboxed.c** - Main C shell implementation with sandboxing
2. **sandbox_gui.py** - pygame-based visual terminal interface
3. **makefile** - Build system
4. **sandbox/** - Restricted directory for file operations

### Security Implementation

#### Resource Limits (setrlimit)
```c
void setup_resource_limits() {
    struct rlimit limit;
    limit.rlim_cur = MAX_CPU_TIME;
    limit.rlim_max = MAX_CPU_TIME;
    setrlimit(RLIMIT_CPU, &limit);
    // ... memory, processes, files
}
```

#### Command Validation
```c
int is_command_allowed(char *cmd) {
    if (is_command_blocked(cmd)) return 0;
    if (!is_command_whitelisted(cmd)) return 0;
    return 1;
}
```

#### Path Validation
```c
int is_path_allowed(const char *path) {
    char resolved_path[PATH_MAX];
    realpath(path, resolved_path);
    // Check if within sandbox directory
    return strncmp(resolved_path, SANDBOX_DIR, len) == 0;
}
```

## 🔧 Development

### Build Options

```bash
# Build sandboxed shell (default)
make

# Build original shell (without sandbox)
make original

# Clean build files
make clean

# Setup + build
make clean setup all
```

### Modifying Whitelist

Edit `project_sandboxed.c` lines 19-25:

```c
const char *allowed_commands[] = {
    "ls", "cat", "echo", // Add your commands here
    NULL
};
```

### Adjusting Resource Limits

Edit `project_sandboxed.c` lines 17-20:

```c
#define MAX_CPU_TIME 30        // Seconds
#define MAX_MEMORY 100         // MB
#define MAX_PROCESSES 20       // Count
#define MAX_OPEN_FILES 64      // Count
```

## 📊 Features Demonstrated

### Shell Features
- ✅ Command parsing and execution
- ✅ Built-in commands (cd, exit, history, help)
- ✅ I/O redirection (`>`, `<`)
- ✅ Pipelines (`|`)
- ✅ Background processes (`&`)
- ✅ Command aliases
- ✅ Tab completion
- ✅ History management

### Security Features
- ✅ Resource limitation (CPU, memory, processes, files)
- ✅ Command whitelisting/blacklisting
- ✅ File system access control
- ✅ Real-time monitoring
- ✅ Execution logging

### User Interface
- ✅ Visual terminal with pygame
- ✅ Color-coded output
- ✅ Real-time statistics
- ✅ Scrollable output
- ✅ Professional appearance

## 🐛 Troubleshooting

### "myshell not found"
```bash
make clean
make
```

### "readline not found"
```bash
brew install readline
make clean
make
```

### "pygame not found"
```bash
pip3 install pygame
# or
python3 -m pip install pygame
```

### Permission Denied
```bash
chmod +x myshell
```

### Sandbox Directory Issues
```bash
make setup
# or manually
mkdir -p sandbox
```

## 📝 Project Structure

```
os/
├── project.c              # Original shell implementation
├── project_sandboxed.c    # Sandboxed version (enhanced)
├── sandbox_gui.py         # pygame terminal interface
├── makefile              # Build system
├── requirements.txt      # Python dependencies
├── README.md            # This file
├── myshell              # Compiled executable
└── sandbox/             # Restricted directory
    └── (user files)
```

## 🎓 Educational Value

This project demonstrates:

1. **Process Management**: fork(), exec(), wait()
2. **System Calls**: setrlimit(), chdir(), open(), pipe()
3. **Security**: Sandboxing, resource limits, access control
4. **IPC**: Pipes for inter-process communication
5. **Signal Handling**: SIGCHLD for process management
6. **File I/O**: Redirection, file operations
7. **User Interface**: GUI design with pygame

## 📄 License

Educational project for OS coursework.

## 👨‍💻 Author

Created for Operating Systems course project.

---

**Note**: This is a sandboxed environment. Some commands may be restricted for security purposes.

