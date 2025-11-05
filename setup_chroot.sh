#!/bin/bash
# Quick setup script for chroot environment
# Note: chroot requires root privileges on macOS

SANDBOX_DIR="/Users/jatin/Desktop/os/sandbox"

echo "Setting up chroot environment..."
echo "Note: chroot functionality requires root privileges"
echo ""

# Create basic directory structure
echo "Creating directory structure..."
mkdir -p "$SANDBOX_DIR/bin"
mkdir -p "$SANDBOX_DIR/usr/bin"
mkdir -p "$SANDBOX_DIR/usr/lib"
mkdir -p "$SANDBOX_DIR/lib"
mkdir -p "$SANDBOX_DIR/etc"
mkdir -p "$SANDBOX_DIR/tmp"
mkdir -p "$SANDBOX_DIR/home"

# Create a basic /etc/passwd (needed for some commands)
echo "Creating basic /etc/passwd..."
echo "root:x:0:0:root:/root:/bin/sh" > "$SANDBOX_DIR/etc/passwd"
echo "$(whoami):x:$(id -u):$(id -g):User:/home:$(which sh)" >> "$SANDBOX_DIR/etc/passwd"

# Create basic /etc/group
echo "Creating basic /etc/group..."
echo "root:x:0:" > "$SANDBOX_DIR/etc/group"
echo "$(whoami):x:$(id -g):" >> "$SANDBOX_DIR/etc/group"

echo ""
echo "✓ Basic chroot structure created in: $SANDBOX_DIR"
echo ""
echo "IMPORTANT NOTES:"
echo "================"
echo "1. chroot() requires ROOT privileges (sudo)"
echo "2. On macOS, chroot may have additional restrictions"
echo "3. You need to copy binaries and libraries into the chroot"
echo "4. The shell will work without chroot (other sandboxing still active)"
echo ""
echo "To use chroot, you would need to:"
echo "  sudo ./myshell"
echo ""
echo "For normal use (without root), the shell still has:"
echo "  ✓ Resource limits"
echo "  ✓ Command whitelist"
echo "  ✓ Path restrictions"
echo "  ✓ File system access control"
echo ""
echo "Setup complete!"

