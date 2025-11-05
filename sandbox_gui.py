#!/usr/bin/env python3
"""
Sandboxed Shell GUI - Pygame Terminal Interface
A visual terminal interface for the sandboxed shell
"""

import pygame
import subprocess
import threading
import queue
import os
import sys
from datetime import datetime
from collections import deque

# Initialize pygame
pygame.init()

# Colors - Terminal theme
BG_COLOR = (15, 20, 25)           # Dark background
TEXT_COLOR = (0, 255, 100)         # Green terminal text
PROMPT_COLOR = (100, 200, 255)     # Cyan for prompt
SANDBOX_COLOR = (255, 180, 0)      # Orange for sandbox indicators
ERROR_COLOR = (255, 80, 80)        # Red for errors
WARNING_COLOR = (255, 255, 100)    # Yellow for warnings
BORDER_COLOR = (255, 165, 0)       # Orange border
HEADER_BG = (25, 30, 35)           # Slightly lighter for header
INPUT_BG = (20, 25, 30)            # Input area background

# Screen setup
WIDTH, HEIGHT = 1200, 800
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("🔒 Sandboxed Shell - OS Project")

# Fonts
try:
    FONT_NAME = 'Monaco'
    font = pygame.font.SysFont(FONT_NAME, 14, bold=False)
    title_font = pygame.font.SysFont(FONT_NAME, 16, bold=True)
    small_font = pygame.font.SysFont(FONT_NAME, 12, bold=False)
except:
    font = pygame.font.Font(None, 16)
    title_font = pygame.font.Font(None, 20)
    small_font = pygame.font.Font(None, 14)

class TerminalLine:
    """Represents a single line in the terminal"""
    def __init__(self, text, color=TEXT_COLOR, is_prompt=False):
        self.text = text
        self.color = color
        self.is_prompt = is_prompt
        self.timestamp = datetime.now()

class SandboxedTerminal:
    def __init__(self):
        self.output_lines = deque(maxlen=1000)  # Keep max 1000 lines
        self.input_buffer = ""
        self.scroll_offset = 0
        self.cursor_visible = True
        self.cursor_timer = 0
        
        # Process state
        self.process = None
        self.output_queue = queue.Queue()
        self.reader_thread = None
        self.running = True
        
        # Sandbox info
        self.start_time = datetime.now()
        self.commands_executed = 0
        self.commands_blocked = 0
        
        # Tab completion
        self.available_commands = [
            'cd', 'exit', 'print_history', 'add_alias', 'remove_alias', 'help', 'stats', 'commands',
            'ls', 'cat', 'display', 'pwd', 'grep', 'touch', 'mkdir', 'rmdir', 
            'cp', 'mv', 'head', 'tail', 'wc', 'sort', 'uniq', 'find', 
            'which', 'date', 'whoami', 'hostname', 'sleep', 'clear'
        ]
        self.completion_matches = []
        self.completion_index = 0
        self.last_completion_input = ""  # Track what input we're completing
        
        # Start the shell
        self.start_shell()
        
        # Welcome message
        self.add_welcome_message()
    
    def start_shell(self):
        """Start the sandboxed shell process"""
        try:
            # Check if executable exists
            shell_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'myshell')
            if not os.path.exists(shell_path):
                self.add_line("ERROR: myshell executable not found!", ERROR_COLOR)
                self.add_line("Please compile with: make clean && make", WARNING_COLOR)
                return
            
            self.process = subprocess.Popen(
                [shell_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=1,
                universal_newlines=True,
                cwd=os.path.dirname(os.path.abspath(__file__))
            )
            
            # Start output reader thread
            self.reader_thread = threading.Thread(target=self.read_output, daemon=True)
            self.reader_thread.start()
            
        except Exception as e:
            self.add_line(f"ERROR: Failed to start shell: {e}", ERROR_COLOR)
    
    def read_output(self):
        """Read output from shell in separate thread"""
        try:
            for line in iter(self.process.stdout.readline, ''):
                if line:
                    self.output_queue.put(line.rstrip())
                if not self.running:
                    break
        except Exception as e:
            self.output_queue.put(f"[ERROR] {e}")
    
    def add_line(self, text, color=TEXT_COLOR, is_prompt=False):
        """Add a line to the terminal display"""
        self.output_lines.append(TerminalLine(text, color, is_prompt))
    
    def add_welcome_message(self):
        """Add welcome message"""
        self.add_line("=" * 80, SANDBOX_COLOR)
        self.add_line("  Sandboxed Shell Terminal Interface", SANDBOX_COLOR)
        self.add_line("  Waiting for shell to initialize...", TEXT_COLOR)
        self.add_line("=" * 80, SANDBOX_COLOR)
        self.add_line("", TEXT_COLOR)
    
    def update_output(self):
        """Update output from queue"""
        lines_added = 0
        while not self.output_queue.empty() and lines_added < 10:  # Process max 10 lines per frame
            try:
                line = self.output_queue.get_nowait()
                
                # Determine color based on content
                color = TEXT_COLOR
                if "[SANDBOX BLOCKED]" in line or "ERROR" in line:
                    color = ERROR_COLOR
                    self.commands_blocked += 1
                elif "[Sandbox" in line or "sandbox>" in line:
                    color = PROMPT_COLOR
                elif "✓" in line or "SUCCESS" in line:
                    color = (100, 255, 100)
                
                self.add_line(line, color)
                lines_added += 1
                
            except queue.Empty:
                break
    
    def send_command(self, command):
        """Send command to shell"""
        if not self.process or self.process.poll() is not None:
            self.add_line("ERROR: Shell process is not running", ERROR_COLOR)
            return
        
        try:
            self.process.stdin.write(command + '\n')
            self.process.stdin.flush()
            self.add_line(f"$ {command}", PROMPT_COLOR, is_prompt=True)
            self.commands_executed += 1
        except Exception as e:
            self.add_line(f"ERROR: Failed to send command: {e}", ERROR_COLOR)
    
    def draw_header(self, surface):
        """Draw header with sandbox information"""
        header_height = 100
        
        # Header background
        pygame.draw.rect(surface, HEADER_BG, (10, 10, WIDTH - 20, header_height))
        pygame.draw.rect(surface, BORDER_COLOR, (10, 10, WIDTH - 20, header_height), 2)
        
        # Title
        title = title_font.render("🔒 SANDBOXED SHELL ENVIRONMENT", True, SANDBOX_COLOR)
        surface.blit(title, (20, 20))
        
        # Status indicators (two rows)
        y = 50
        indicators = [
            ("✓ Resource Limits", TEXT_COLOR),
            ("✓ Command Whitelist", TEXT_COLOR),
            ("✓ Path Restrictions", TEXT_COLOR),
            ("✓ Monitoring Active", TEXT_COLOR),
        ]
        
        for i, (text, color) in enumerate(indicators):
            x = 20 + (i % 2) * 280
            y_pos = y + (i // 2) * 22
            status_text = small_font.render(text, True, color)
            surface.blit(status_text, (x, y_pos))
        
        # Runtime stats (right side)
        runtime = (datetime.now() - self.start_time).seconds
        stats_lines = [
            f"Runtime: {runtime}s",
            f"Executed: {self.commands_executed}",
            f"Blocked: {self.commands_blocked}"
        ]
        
        for i, line in enumerate(stats_lines):
            stat_text = small_font.render(line, True, WARNING_COLOR)
            surface.blit(stat_text, (WIDTH - 200, 50 + i * 20))
    
    def draw_terminal(self, surface):
        """Draw terminal output area"""
        terminal_y = 120
        terminal_height = HEIGHT - 200
        terminal_width = WIDTH - 40
        
        # Terminal background
        pygame.draw.rect(surface, BG_COLOR, (20, terminal_y, terminal_width, terminal_height))
        pygame.draw.rect(surface, BORDER_COLOR, (20, terminal_y, terminal_width, terminal_height), 1)
        
        # Calculate visible lines
        line_height = 18
        max_visible_lines = (terminal_height - 20) // line_height
        
        # Get lines to display
        total_lines = len(self.output_lines)
        start_idx = max(0, total_lines - max_visible_lines + self.scroll_offset)
        end_idx = min(total_lines, start_idx + max_visible_lines)
        
        visible_lines = list(self.output_lines)[start_idx:end_idx]
        
        # Draw lines
        y = terminal_y + 10
        for line in visible_lines:
            # Truncate long lines
            display_text = line.text[:150] if len(line.text) > 150 else line.text
            text_surface = font.render(display_text, True, line.color)
            surface.blit(text_surface, (30, y))
            y += line_height
        
        # Scroll indicator
        if self.scroll_offset < 0:
            scroll_text = small_font.render(f"↑ Scrolled {-self.scroll_offset} lines", True, WARNING_COLOR)
            surface.blit(scroll_text, (WIDTH - 200, terminal_y + 5))
    
    def draw_input_area(self, surface):
        """Draw input area at bottom"""
        input_y = HEIGHT - 70
        input_height = 50
        
        # Input background
        pygame.draw.rect(surface, INPUT_BG, (20, input_y, WIDTH - 40, input_height))
        pygame.draw.rect(surface, BORDER_COLOR, (20, input_y, WIDTH - 40, input_height), 2)
        
        # Prompt
        prompt_text = "sandbox> "
        prompt_surface = font.render(prompt_text, True, PROMPT_COLOR)
        surface.blit(prompt_surface, (30, input_y + 15))
        
        # Input text
        input_surface = font.render(self.input_buffer, True, TEXT_COLOR)
        input_x = 30 + prompt_surface.get_width()
        surface.blit(input_surface, (input_x, input_y + 15))
        
        # Cursor
        if self.cursor_visible:
            cursor_x = input_x + input_surface.get_width()
            pygame.draw.line(surface, TEXT_COLOR, 
                           (cursor_x, input_y + 15), 
                           (cursor_x, input_y + 33), 2)
        
        # Help text
        help_text = small_font.render("Type 'help' for commands | TAB to autocomplete | ↑↓ to scroll | 'exit' to quit", 
                                     True, (100, 100, 100))
        surface.blit(help_text, (30, input_y + input_height + 5))
    
    def draw_warnings(self, surface):
        """Draw security warning"""
        warning_y = HEIGHT - 15
        warning = small_font.render("⚠ All commands monitored • Sandboxed environment active", 
                                   True, WARNING_COLOR)
        text_rect = warning.get_rect(center=(WIDTH // 2, warning_y))
        surface.blit(warning, text_rect)
    
    def get_file_completions(self, prefix):
        """Get list of files matching prefix in sandbox directory"""
        try:
            sandbox_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sandbox')
            if not os.path.exists(sandbox_dir):
                return []
            
            files = os.listdir(sandbox_dir)
            matches = [f for f in files if f.startswith(prefix)]
            return sorted(matches)
        except:
            return []
    
    def handle_tab_completion(self):
        """Handle tab completion for commands and files"""
        if not self.input_buffer:
            return
        
        parts = self.input_buffer.split()
        
        # Determine what we're trying to complete
        if len(parts) <= 1 and not self.input_buffer.endswith(' '):
            # Completing a command
            prefix = parts[0] if parts else ""
            completion_key = f"cmd:{prefix}"
            
            # If this is a new completion (different from last time), find matches
            if completion_key != self.last_completion_input:
                self.completion_matches = [cmd for cmd in self.available_commands 
                                          if cmd.startswith(prefix)]
                self.completion_index = 0
                self.last_completion_input = completion_key
            
            if self.completion_matches:
                # Show current match and move to next
                self.input_buffer = self.completion_matches[self.completion_index]
                self.completion_index = (self.completion_index + 1) % len(self.completion_matches)
                
        else:
            # Completing a file argument
            if self.input_buffer.endswith(' '):
                prefix = ""
                base_parts = parts
            else:
                prefix = parts[-1] if parts else ""
                base_parts = parts[:-1]
            
            completion_key = f"file:{' '.join(parts[:-1] if not self.input_buffer.endswith(' ') else parts)}:{prefix}"
            
            # If this is a new completion, find matches
            if completion_key != self.last_completion_input:
                self.completion_matches = self.get_file_completions(prefix)
                self.completion_index = 0
                self.last_completion_input = completion_key
            
            if self.completion_matches:
                # Replace the last word with current match
                if self.input_buffer.endswith(' '):
                    self.input_buffer = ' '.join(parts) + ' ' + self.completion_matches[self.completion_index]
                else:
                    self.input_buffer = ' '.join(base_parts + [self.completion_matches[self.completion_index]])
                
                self.completion_index = (self.completion_index + 1) % len(self.completion_matches)
    
    def handle_key(self, event):
        """Handle keyboard input"""
        if event.key == pygame.K_RETURN:
            if self.input_buffer.strip():
                self.send_command(self.input_buffer)
                self.input_buffer = ""
                self.scroll_offset = 0  # Auto-scroll to bottom
                # Reset completion state
                self.completion_matches = []
                self.completion_index = 0
                self.last_completion_input = ""
        
        elif event.key == pygame.K_TAB:
            # Handle tab completion
            self.handle_tab_completion()
        
        elif event.key == pygame.K_BACKSPACE:
            self.input_buffer = self.input_buffer[:-1]
            # Reset completion state when user edits
            self.completion_matches = []
            self.completion_index = 0
            self.last_completion_input = ""
        
        elif event.key == pygame.K_UP:
            self.scroll_offset -= 1
        
        elif event.key == pygame.K_DOWN:
            self.scroll_offset = min(0, self.scroll_offset + 1)
        
        elif event.key == pygame.K_PAGEUP:
            self.scroll_offset -= 10
        
        elif event.key == pygame.K_PAGEDOWN:
            self.scroll_offset = min(0, self.scroll_offset + 10)
        
        elif event.unicode and event.unicode.isprintable():
            self.input_buffer += event.unicode
            # Reset completion state when user types
            self.completion_matches = []
            self.completion_index = 0
            self.last_completion_input = ""
    
    def cleanup(self):
        """Clean up resources"""
        self.running = False
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=2)
            except:
                self.process.kill()

def main():
    terminal = SandboxedTerminal()
    clock = pygame.time.Clock()
    running = True
    frame_count = 0
    
    # Main loop
    while running:
        frame_count += 1
        
        # Event handling
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            
            elif event.type == pygame.KEYDOWN:
                terminal.handle_key(event)
        
        # Update cursor blink
        if frame_count % 30 == 0:
            terminal.cursor_visible = not terminal.cursor_visible
        
        # Update output from shell
        terminal.update_output()
        
        # Drawing
        screen.fill((10, 10, 15))
        terminal.draw_header(screen)
        terminal.draw_terminal(screen)
        terminal.draw_input_area(screen)
        terminal.draw_warnings(screen)
        
        # Draw outer border
        pygame.draw.rect(screen, BORDER_COLOR, (5, 5, WIDTH - 10, HEIGHT - 10), 3)
        
        pygame.display.flip()
        clock.tick(60)  # 60 FPS
    
    # Cleanup
    terminal.cleanup()
    pygame.quit()
    sys.exit(0)

if __name__ == "__main__":
    main()

