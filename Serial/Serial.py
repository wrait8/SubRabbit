import threading
import serial
import colorama
from colorama import Fore, Back, Style
import time
import sys
import os
import datetime
import readline
import atexit
import uuid
import argparse
import shutil
import subprocess

TIMEOUT_DURATION = 15  # seconds

CYAN = Style.BRIGHT + Fore.CYAN
GREEN = Style.BRIGHT + Fore.GREEN
BLUE = Style.BRIGHT + Fore.BLUE
RED = Style.BRIGHT + Fore.RED
YELLOW = Style.BRIGHT + Fore.YELLOW
MAGENTA = Style.BRIGHT + Fore.MAGENTA
WHITE = Style.BRIGHT + Fore.WHITE
RESET = Fore.RESET + Style.RESET_ALL

# === CONFIGURATION ===
PORT = "COM3"
BAUD = 115200

# Default FQBN used for arduino-cli compile/upload. This is a starting
# point only -- a stock "arduino:avr:leonardo" core assumes 16MHz/5V.
# For a custom 32u4 board running 8MHz/3.3V you'll want your own
# boards.txt entry (SpenceKonde-style ATmega32u4 8MHz core is the usual
# route) and point DEFAULT_FQBN at that instead.
DEFAULT_FQBN = "arduino:avr:leonardo"

# Where to look for a bundled arduino-cli binary relative to this script,
# before falling back to PATH.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Repo that firmware releases are pulled from for the [U]pgrade option.
GITHUB_REPO = "wrait8/SubRabbit"

# === LOGGING / STATE ===
SESSION_ID = None
ser = None
log_buffer = []
should_exit = False
is_reconnecting = False
exit_pressed = False

# === COMMAND LINE ARGUMENTS ===
parser = argparse.ArgumentParser(description='Serial Terminal + Sketch Uploader')
parser.add_argument('--output', '-o', type=str, nargs='?', const='auto', help='Save log output')
parser.add_argument('--port', '-p', type=str, help=f'Serial port (default: {PORT})')
parser.add_argument('--baud', '-b', type=int, help=f'Baud rate (default: {BAUD})')
parser.add_argument('--fqbn', type=str, help=f'Default board FQBN (default: {DEFAULT_FQBN})')
args = parser.parse_args()

if args.port:
    PORT = args.port
if args.baud:
    BAUD = args.baud
if args.fqbn:
    DEFAULT_FQBN = args.fqbn

output_filename = None
if args.output:
    output_filename = None if args.output == 'auto' else args.output

# === COMMAND HISTORY SETUP ===
histfile = os.path.join(os.path.expanduser("~"), ".serial_terminal_history")
try:
    readline.read_history_file(histfile)
except FileNotFoundError:
    pass
atexit.register(readline.write_history_file, histfile)


# === BANNER ===
def ascii_banner():
    banner = r"""
                        (`.         ,-,
                        ` `.    ,;' /
                         `.  ,'/ .'
                          `. X /.'
                .-;--''--.._` ` (
              .'            /   `
             ,           ` '   Q '
             ,         ,   `._    \
          ,.|         '     `-.;_'
          :  . `  ;    `  ` --,.._;
           ' `    ,   )   .'
              `._ ,  '   /_
                 ; ,''-,;' ``-
                  ``-..__``--`
"""
    print(banner)
    print()


def get_timestamp():
    return datetime.datetime.now().strftime("[%H:%M:%S]")



    return datetime.datetime.now().strftime("[%H:%M:%S]")


# === SYSTEM COMMAND HELPERS (used inside the live terminal) ===
def is_command_executable(command):
    cmd_parts = command.strip().split()
    if not cmd_parts:
        return False
    executable = cmd_parts[0]
    if shutil.which(executable):
        return True
    if sys.platform == 'win32':
        for ext in ['.exe', '.bat', '.cmd', '.ps1']:
            if shutil.which(executable + ext):
                return True
    return False


def execute_system_command(command):
    try:
        print(YELLOW + f"\n[*] Executing: {command}" + RESET)
        print("-" * 50)
        timeout = 30
        try:
            result = subprocess.run(
                command, shell=True,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                text=True, timeout=timeout
            )
            if result.stdout:
                print(result.stdout, end='')
            if result.stderr:
                print(RED + result.stderr + RESET, end='')
            if result.returncode not in (0, 1):
                print(YELLOW + f"[!] Command exited with code: {result.returncode}" + RESET)
        except subprocess.TimeoutExpired:
            print(RED + f"[!] Command timed out after {timeout} seconds" + RESET)
        print("-" * 50)
        return True
    except Exception as e:
        print(RED + f"[!] Failed to execute: {e}" + RESET)
        print("-" * 50)
        return False


def save_log(data_buffer, log_file_path):
    if data_buffer and log_file_path:
        with open(log_file_path, 'w', encoding='utf-8') as f:
            f.write(''.join(data_buffer))
        print(GREEN + f"[+] Log saved to: {log_file_path}" + RESET)


def save_log_automatically(buffer):
    if not buffer:
        return
    global output_filename
    if output_filename:
        log_file_path = output_filename
    else:
        log_file_path = f"serial_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}_{SESSION_ID}.txt"
    save_log(buffer, log_file_path)


# === ARDUINO-CLI INTEGRATION ===
def _detect_arduino_cli_asset_suffix():
    """Map this machine's OS/arch to arduino-cli's release asset naming,
    e.g. 'Linux_64bit.tar.gz', 'Windows_64bit.zip', 'macOS_ARM64.tar.gz'."""
    import platform as _platform

    machine = _platform.machine().lower()

    if sys.platform == 'win32':
        os_name, ext = "Windows", "zip"
    elif sys.platform == 'darwin':
        os_name, ext = "macOS", "tar.gz"
    else:
        os_name, ext = "Linux", "tar.gz"

    if machine in ('x86_64', 'amd64'):
        arch_name = "64bit"
    elif machine in ('i386', 'i686', 'x86'):
        arch_name = "32bit"
    elif machine in ('arm64', 'aarch64'):
        arch_name = "ARM64"
    elif machine.startswith('armv7'):
        arch_name = "ARMv7"
    elif machine.startswith('armv6'):
        arch_name = "ARMv6"
    else:
        arch_name = "64bit"  # best-effort fallback

    return os_name, arch_name, ext, f"{os_name}_{arch_name}.{ext}"


def download_and_install_arduino_cli():
    """Fetch the matching arduino-cli binary straight from Arduino's official
    GitHub releases and drop it next to this script. Nothing is bundled or
    committed to the repo -- this always pulls fresh from the source, so
    there's no redistribution / license-file concern on our end."""
    import json
    import tarfile
    import tempfile
    import urllib.request
    import zipfile

    os_name, arch_name, ext, suffix = _detect_arduino_cli_asset_suffix()
    print(YELLOW + "[*] " + RESET + "arduino-cli not found, fetching from Arduino's GitHub releases..." + RESET)

    api_url = "https://api.github.com/repos/arduino/arduino-cli/releases/latest"
    req = urllib.request.Request(
        api_url,
        headers={"Accept": "application/vnd.github+json", "User-Agent": "SubRabbit-Toolkit"}
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            data = json.loads(resp.read().decode())
    except Exception as e:
        print(RED + f"[!] Failed to reach GitHub: {e}" + RESET)
        return None

    assets = data.get("assets", [])
    match = next((a for a in assets if a.get("name", "").endswith(suffix)), None)
    if not match:
        print(RED + f"[!] No release asset found matching {suffix}" + RESET)
        return None

    print(GREEN + f"[+] Found: {match['name']}" + RESET)

    tmp_dir = tempfile.mkdtemp(prefix="arduino_cli_dl_")
    archive_path = os.path.join(tmp_dir, match["name"])
    try:
        print(YELLOW + "[*] " + RESET + f"Downloading {match['name']}..." + RESET)
        urllib.request.urlretrieve(match["browser_download_url"], archive_path)
    except Exception as e:
        print(RED + f"[!] Download failed: {e}" + RESET)
        return None

    target_name = "arduino-cli.exe" if sys.platform == 'win32' else "arduino-cli"
    dest_path = os.path.join(SCRIPT_DIR, target_name)

    try:
        if ext == "zip":
            with zipfile.ZipFile(archive_path) as zf:
                for member in zf.namelist():
                    if os.path.basename(member) == target_name:
                        with zf.open(member) as src, open(dest_path, 'wb') as dst:
                            shutil.copyfileobj(src, dst)
                        break
        else:
            with tarfile.open(archive_path, 'r:gz') as tf:
                for member in tf.getmembers():
                    if os.path.basename(member.name) == target_name:
                        extracted = tf.extractfile(member)
                        with open(dest_path, 'wb') as dst:
                            shutil.copyfileobj(extracted, dst)
                        break
    except Exception as e:
        print(RED + f"[!] Failed to extract arduino-cli: {e}" + RESET)
        return None

    if not os.path.isfile(dest_path):
        print(RED + "[!] Extraction finished but the binary wasn't found inside the archive." + RESET)
        return None

    if sys.platform != 'win32':
        os.chmod(dest_path, 0o755)

    print(GREEN + f"[+] arduino-cli ready at: {dest_path}" + RESET)
    return dest_path


def find_arduino_cli():
    """Look next to this script first, then PATH, then self-download the
    matching official release if neither has it."""
    candidates = ["arduino-cli", "arduino-cli.exe"]
    for name in candidates:
        local_path = os.path.join(SCRIPT_DIR, name)
        if os.path.isfile(local_path):
            return local_path

    on_path = shutil.which("arduino-cli")
    if on_path:
        return on_path

    return download_and_install_arduino_cli()


def run_arduino_cli(cli_path, cli_args, label):
    print(YELLOW + f"\n[*] {label}..." + RESET)
    print("-" * 50)
    try:
        proc = subprocess.Popen(
            [cli_path] + cli_args,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, bufsize=1
        )
        for line in proc.stdout:
            print(line, end='')
        proc.wait()
        print("-" * 50)
        if proc.returncode == 0:
            print(GREEN + f"[+] {label} succeeded." + RESET)
            return True
        else:
            print(RED + f"[!] {label} failed (exit code {proc.returncode})." + RESET)
            return False
    except FileNotFoundError:
        print(RED + f"[!] Could not run arduino-cli at: {cli_path}" + RESET)
        return False
    except KeyboardInterrupt:
        proc.terminate()
        print(RED + "\n[!] Upload cancelled." + RESET)
        return False


def push_payload(port, baud):
    """Compile + upload a .ino sketch via arduino-cli."""
    cli_path = find_arduino_cli()
    if not cli_path:
        print(RED + "[!] arduino-cli not found next to this script or on PATH." + RESET)
        print(YELLOW + "    Drop arduino-cli(.exe) in the same folder as this script, "
                        "or install it and make sure it's on PATH." + RESET)
        return

    print(GREEN + f"[+] Using arduino-cli: {cli_path}" + RESET)

    sketch_path = input("Path to .ino file or sketch folder: ").strip().strip('"')
    if not sketch_path:
        print(YELLOW + "[!] No path given, aborting upload." + RESET)
        return
    if not os.path.exists(sketch_path):
        print(RED + f"[!] Path not found: {sketch_path}" + RESET)
        return

    # arduino-cli wants the sketch *folder*, not the .ino file directly
    sketch_dir = sketch_path
    if os.path.isfile(sketch_path):
        sketch_dir = os.path.dirname(os.path.abspath(sketch_path))

    fqbn = input(f"FQBN [default: {DEFAULT_FQBN}]: ").strip()
    if not fqbn:
        fqbn = DEFAULT_FQBN

    upload_port = input(f"Port [default: {port}]: ").strip()
    if not upload_port:
        upload_port = port

    ok = run_arduino_cli(
        cli_path,
        ["compile", "--fqbn", fqbn, sketch_dir],
        "Compiling payload"
    )
    if not ok:
        return

    run_arduino_cli(
        cli_path,
        ["upload", "-p", upload_port, "--fqbn", fqbn, sketch_dir],
        "Pushing payload to Void Recon"
    )


def get_latest_release_bin():
    """Query the GitHub API for the newest release of GITHUB_REPO and
    return (asset_name, download_url) for a .bin asset, or (None, None)."""
    import urllib.request
    import json

    api_url = f"https://api.github.com/repos/{GITHUB_REPO}/releases/latest"
    req = urllib.request.Request(
        api_url,
        headers={"Accept": "application/vnd.github+json", "User-Agent": "SubRabbit-Toolkit"}
    )
    with urllib.request.urlopen(req, timeout=15) as resp:
        data = json.loads(resp.read().decode())

    assets = data.get("assets", [])
    bin_assets = [a for a in assets if a.get("name", "").endswith(".bin")]

    if not bin_assets:
        return None, None

    if len(bin_assets) == 1:
        chosen = bin_assets[0]
    else:
        print(YELLOW + "[?] " + RESET + f"Multiple .bin assets found in {data.get('tag_name', 'latest')}:")
        for i, a in enumerate(bin_assets):
            print(f"    [{i}] {a['name']}")
        idx = input("Pick one: ").strip()
        try:
            chosen = bin_assets[int(idx)]
        except (ValueError, IndexError):
            print(RED + "[!] Invalid selection." + RESET)
            return None, None

    return chosen["name"], chosen["browser_download_url"]


def upgrade_firmware(port):
    """Pull the latest release .bin from GITHUB_REPO and flash it with
    arduino-cli's precompiled-binary upload (no local compile step)."""
    cli_path = find_arduino_cli()
    if not cli_path:
        print(RED + "[!] arduino-cli not found next to this script or on PATH." + RESET)
        print(YELLOW + "    Drop arduino-cli(.exe) in the same folder as this script, "
                        "or install it and make sure it's on PATH." + RESET)
        return

    print(YELLOW + "[*] " + RESET + f"Checking {GITHUB_REPO} for the latest release..." + RESET)
    try:
        name, url = get_latest_release_bin()
    except Exception as e:
        print(RED + f"[!] Failed to reach GitHub: {e}" + RESET)
        return

    if not url:
        print(RED + "[!] No .bin asset found in the latest release." + RESET)
        return

    print(GREEN + f"[+] Found: {name}" + RESET)

    import urllib.request
    import tempfile
    dest_dir = tempfile.mkdtemp(prefix="SubRabbit_fw_")
    dest_path = os.path.join(dest_dir, name)
    try:
        print(YELLOW + "[*] " + RESET + f"Downloading {name}..." + RESET)
        urllib.request.urlretrieve(url, dest_path)
        print(GREEN + f"[+] Saved to: {dest_path}" + RESET)
    except Exception as e:
        print(RED + f"[!] Download failed: {e}" + RESET)
        return

    fqbn = input(f"FQBN [default: {DEFAULT_FQBN}]: ").strip()
    if not fqbn:
        fqbn = DEFAULT_FQBN

    upload_port = input(f"Port [default: {port}]: ").strip()
    if not upload_port:
        upload_port = port

    run_arduino_cli(
        cli_path,
        ["upload", "-p", upload_port, "--fqbn", fqbn, "-i", dest_path],
        "Flashing firmware"
    )


# === SERIAL TERMINAL LOGIC ===
def initial_boot_message_logic():
    global should_exit
    try:
        print(YELLOW + "\r[?] " + RESET + "Consuming first boot messages . . .", end='', flush=True)
        start_time = time.time()
        timeout = 1
        while time.time() - start_time < timeout and not should_exit:
            try:
                if ser and ser.is_open and ser.in_waiting > 0:
                    ser.read(ser.in_waiting)
                    start_time = time.time()
            except Exception:
                pass
            time.sleep(0.01)
        print('\r' + ' ' * 50 + '\r', end='', flush=True)
    except Exception:
        print('\r' + ' ' * 50 + '\r', end='', flush=True)


def serial_reconnection():
    global ser, is_reconnecting, should_exit, exit_pressed

    if is_reconnecting or should_exit:
        return

    is_reconnecting = True
    try:
        if ser and ser.is_open:
            ser.close()
    except Exception:
        pass

    start_time = time.time()
    while time.time() - start_time < TIMEOUT_DURATION and not should_exit and not exit_pressed:
        try:
            remaining = int(TIMEOUT_DURATION - (time.time() - start_time))
            print(YELLOW + "\r[?] " + RESET + f"Reconnection timeout {remaining} [PRESS ENTER TO EXIT] . . .", end='', flush=True)
            ser = serial.Serial(PORT, BAUD, timeout=0.1)
            print("\r" + " " * 80 + "\r", end="", flush=True)
            print(BLUE + "\r[*] " + RESET + f"Reconnected [{PORT}, {BAUD}]", flush=True)
            time.sleep(0.5)
            initial_boot_message_logic()
            is_reconnecting = False
            return True
        except serial.SerialException:
            time.sleep(0.5)
        except Exception:
            time.sleep(0.5)

    if not should_exit:
        print("\r" + " " * 80 + "\r", end="", flush=True)
        print(RED + "\r[!] " + RESET + f"Failed to reconnect after {TIMEOUT_DURATION}s")
        should_exit = True

    is_reconnecting = False
    exit_pressed = True
    return False


def read_from_serial(buffer):
    global should_exit, is_reconnecting
    initial_boot_message_logic()
    while not should_exit:
        try:
            if ser and ser.is_open and ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode(errors='ignore')
                if data:
                    print(data, end='', flush=True)
                    if buffer is not None:
                        buffer.append(data)
            time.sleep(0.01)
        except (serial.SerialException, AttributeError, OSError, PermissionError):
            if not should_exit and not is_reconnecting:
                print(MAGENTA + "\n[^] " + RESET + "Serial connection lost.")
                serial_reconnection()
            time.sleep(0.5)
        except Exception:
            break


def write_to_serial():
    """Live terminal input loop. Type 'quit' or 'exit' to close the program."""
    global should_exit, exit_pressed, log_buffer

    while not should_exit and not exit_pressed:
        try:
            if sys.platform == 'win32':
                import msvcrt
                if msvcrt.kbhit():
                    line = sys.stdin.readline()
                else:
                    time.sleep(0.1)
                    continue
            else:
                line = sys.stdin.readline()

            if not line:
                break

            stripped = line.strip()

            if stripped == "clear":
                os.system("clear||cls")
                continue
            elif stripped == "banner":
                ascii_banner()
                continue
            elif stripped in ("quit", "exit"):
                should_exit = True
                exit_pressed = True
                break
            elif stripped == "session":
                print(f"{MAGENTA}[^]{RESET} Session ID: {BLUE}{SESSION_ID}{RESET}")
                continue
            elif stripped == "help":
                if ser and ser.is_open:
                    ser.write(line.encode())
                    if log_buffer is not None:
                        log_buffer.append(f"{get_timestamp()} >>> {stripped}")
            else:
                if is_command_executable(stripped):
                    execute_system_command(stripped)
                    if log_buffer is not None:
                        log_buffer.append(f"{get_timestamp()} >>> [SYSTEM] {stripped}")
                else:
                    if ser and ser.is_open:
                        ser.write(line.encode())
                        if log_buffer is not None:
                            log_buffer.append(f"{get_timestamp()} >>> {stripped}")
        except KeyboardInterrupt:
            should_exit = True
            exit_pressed = True
            break
        except (serial.serialutil.PortNotOpenError, ValueError, AttributeError):
            if not should_exit:
                print(YELLOW + "\n[!] Not connected. Type 'quit' to exit." + RESET)
            continue
        except Exception as e:
            if not should_exit:
                print(YELLOW + f"\n[!] Error: {e}" + RESET)
            continue


def connect_serial(port, baud):
    """Open the port and run the interactive terminal (read + write threads)
    until the user quits or the link dies."""
    global ser, should_exit, exit_pressed, is_reconnecting, log_buffer

    should_exit = False
    exit_pressed = False
    is_reconnecting = False

    print(YELLOW + "\r[?] " + RESET + f"Connecting to [{port}, {baud}] . . .", end='', flush=True)
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
        print("\r" + " " * 60 + "\r", end="", flush=True)
        print(f"{MAGENTA}[^]{RESET} Session ID: {BLUE}{SESSION_ID}{RESET}")
    except serial.SerialException:
        print("\r" + " " * 80 + "\r", end="", flush=True)
        print(RED + "[!] " + RESET + f"Port {port} hasn't been found.")
        return

    t1 = threading.Thread(target=read_from_serial, args=(log_buffer,), daemon=True)
    t2 = threading.Thread(target=write_to_serial, daemon=False)
    t1.start()
    t2.start()

    try:
        t2.join()
    except KeyboardInterrupt:
        should_exit = True
        exit_pressed = True

    try:
        if ser and ser.is_open:
            ser.close()
    except Exception:
        pass

    time.sleep(0.3)

    if args.output:
        save_log_automatically(log_buffer)

    if should_exit:
        # user asked for a full quit, not just "back to menu"
        cleanup_and_exit()


def get_single_keypress():
    """Block until one key is pressed, no Enter required. Returns lowercase char."""
    if sys.platform == 'win32':
        import msvcrt
        ch = msvcrt.getwch()
        return ch.lower()
    else:
        import tty
        import termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch.lower()


def cleanup_and_exit():
    try:
        if ser and ser.is_open:
            ser.close()
    except Exception:
        pass
    if args.output:
        save_log_automatically(log_buffer)
    print(MAGENTA + "\n[^] " + RESET + "Session closed.")
    sys.exit(0)


def print_menu():
    print("[C]onnect  - get a shell over your Sub Rabbit")
    print("[U]pgrade your firmware")
    print("[P]ush payload to Void Recon")
    print("[Q]uit")
    print()


def main_menu():
    while True:
        print_menu()
        key = get_single_keypress()

        os.system("clear||cls")
        ascii_banner()

        if key == 'c':
            connect_serial(PORT, BAUD)
        elif key == 'u':
            upgrade_firmware(PORT)
        elif key == 'p':
            push_payload(PORT, BAUD)
        elif key == 'q':
            cleanup_and_exit()
        elif key in ('\x03',):  # Ctrl+C caught as raw byte in unix raw mode
            cleanup_and_exit()
        else:
            print(RED + f"[!] Unknown option: {key}" + RESET)


if __name__ == "__main__":
    os.system("clear||cls")
    SESSION_ID = f"{uuid.uuid4().hex[:4]}-{uuid.uuid4().hex[:4]}-{uuid.uuid4().hex[:4]}"
    ascii_banner()
    try:
        main_menu()
    except KeyboardInterrupt:
        cleanup_and_exit()
