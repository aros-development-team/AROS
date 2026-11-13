#!/usr/bin/env python3
"""
AROS .clangd Configuration Generator

This script generates a .clangd configuration file from the template,
allowing users to customize paths for their AROS build environment.
"""

import os
import sys
import argparse
from pathlib import Path
from typing import Optional, Dict, List


def find_clang_include_paths() -> List[str]:
    """
    Automatically detect clang include paths on the system.
    Returns a list of clang builtin include directories.
    """
    clang_paths = []
    clang_base_dirs = [
        "/usr/lib/clang",
        "/usr/local/lib/clang",
        "/opt/homebrew/lib/clang"  # macOS Homebrew
    ]

    for base_dir in clang_base_dirs:
        if os.path.exists(base_dir):
            try:
                versions = sorted(os.listdir(base_dir), reverse=True)
                for version in versions:
                    version_path = os.path.join(base_dir, version, "include")
                    if os.path.exists(version_path):
                        clang_paths.append(version_path)
            except (OSError, PermissionError):
                continue

    return clang_paths[:5]  # Limit to 5 most recent versions


def validate_directory(path: str, name: str) -> bool:
    """Validate that a directory exists and is readable."""
    if not os.path.exists(path):
        print(f"Warning: {name} directory '{path}' does not exist.")
        return False
    if not os.path.isdir(path):
        print(f"Error: {name} path '{path}' is not a directory.")
        return False
    if not os.access(path, os.R_OK):
        print(f"Error: {name} directory '{path}' is not readable.")
        return False
    return True


def detect_source_directory() -> Optional[str]:
    """Try to auto-detect AROS source directory."""
    current_dir = Path.cwd()

    # Look for common AROS source indicators
    source_indicators = [
        "compiler/include/aros",
        "arch",
        "rom/exec"
    ]

    # Check current directory and parents
    for path in [current_dir] + list(current_dir.parents):
        for indicator in source_indicators:
            if (path / indicator).exists():
                return str(path)

    return None


def detect_build_directory() -> Optional[str]:
    """Try to auto-detect AROS build directory."""
    current_dir = Path.cwd()

    # Look for common AROS build indicators
    build_indicators = [
        "bin/linux-x86_64/AROS",
        "bin/pc-i386/AROS",
        "bin/pc-x86_64/AROS",
        "bin/amiga-m68k/AROS",
        "bin/raspi-armhf/AROS"
    ]

    # First check if we're already in a build directory
    for path in [current_dir] + list(current_dir.parents):
        for indicator in build_indicators:
            if (path / indicator).exists():
                return str(path)

    # If not found, try to find source directory and look for sibling build directories
    source_dir = detect_source_directory()
    if source_dir:
        source_path = Path(source_dir)
        parent_dir = source_path.parent

        # Look for sibling directories that might be build directories
        if parent_dir.exists():
            try:
                for sibling in parent_dir.iterdir():
                    if sibling.is_dir() and sibling != source_path:
                        # Check if this sibling looks like a build directory
                        for indicator in build_indicators:
                            if (sibling / indicator).exists():
                                return str(sibling)

            except (OSError, PermissionError):
                pass

    return None


def print_directory_structure_help():
    """Print detailed help about AROS directory structures."""
    print("AROS Development Directory Structures")
    print("=" * 50)
    print()
    print("AROS development typically uses one of these directory structures:")
    print()
    print("1. SEPARATE SOURCE AND BUILD DIRECTORIES (Recommended)")
    print("   This keeps source code clean and allows multiple build configurations.")
    print()
    print("   Example structure:")
    print("   /home/user/aros/")
    print("   ├── aros-source/              # AROS source code")
    print("   │   ├── configure             # Build configuration script")
    print("   │   ├── mmakefile            # Main makefile")
    print("   │   ├── compiler/")
    print("   │   │   └── include/          # AROS headers")
    print("   │   ├── arch/                 # Architecture-specific code")
    print("   │   └── rom/                  # ROM modules")
    print("   │")
    print("   └── aros-build/               # Build output directory")
    print("       ├── bin/")
    print("       │   └── linux-x86_64/    # Target-specific builds")
    print("       │       └── AROS/")
    print("       │           └── Developer/")
    print("       │               └── include/  # Generated headers")
    print("       └── gen/                 # Generated files")
    print()
    print("2. BUILD WITHIN SOURCE DIRECTORY")
    print("   Everything is contained within the source directory.")
    print()
    print("   Example structure:")
    print("   /home/user/aros-source/")
    print("   ├── configure")
    print("   ├── mmakefile")
    print("   ├── compiler/")
    print("   ├── bin/                     # Build output")
    print("   │   └── linux-x86_64/")
    print("   │       └── AROS/")
    print("   │           └── Developer/")
    print("   │               └── include/")
    print("   └── gen/")
    print()
    print("DETECTION PROCESS:")
    print("- The script first looks for source indicators")
    print("- Then looks for build indicators")
    print("- If source found but no build, checks sibling directories for build")
    print()
    print("USAGE RECOMMENDATIONS:")
    print("- Run this script from your source directory for best auto-detection")
    print("- Use --interactive mode for guided setup")
    print("- The generated .clangd file should be placed in your source root")
    print()


def detect_target_architecture(build_dir: str) -> Optional[str]:
    """Try to auto-detect target architecture from build directory."""
    bin_dir = os.path.join(build_dir, "bin")
    if not os.path.exists(bin_dir):
        return None

    # Common AROS target architectures
    common_targets = [
        "linux-x86_64",
        "pc-x86_64",
        "pc-i386",
        "amiga-m68k",
        "raspi-armhf"
    ]

    try:
        available_targets = os.listdir(bin_dir)
        for target in common_targets:
            if target in available_targets:
                aros_path = os.path.join(bin_dir, target, "AROS")
                if os.path.exists(aros_path):
                    return target
    except (OSError, PermissionError):
        pass

    return None


def get_target_triple(target_arch: str) -> str:
    """Convert AROS target architecture to clang target triple."""
    target_map = {
        "linux-x86_64": "x86_64-unknown-linux-gnu",
        "pc-x86_64": "x86_64-pc-none-elf",
        "pc-i386": "i386-pc-none-elf",
        "amiga-m68k": "m68k-unknown-amigaos",
        "raspi-armhf": "arm-unknown-linux-gnueabihf"
    }
    return target_map.get(target_arch, "x86_64-unknown-linux-gnu")


def generate_config(template_path: str, build_dir: str, target_arch: str,
                   output_path: str, clang_paths: Optional[List[str]] = None) -> bool:
    """Generate .clangd config from template."""

    if not os.path.exists(template_path):
        print(f"Error: Template file '{template_path}' not found.")
        return False

    try:
        with open(template_path, 'r', encoding='utf-8') as f:
            template_content = f.read()
    except (OSError, UnicodeDecodeError) as e:
        print(f"Error reading template file: {e}")
        return False

    # Replace template variables
    config_content = template_content.replace("${AROS_BUILD_DIR}", build_dir)
    config_content = config_content.replace("${AROS_TARGET_ARCH}", target_arch)

    # Update target triple
    target_triple = get_target_triple(target_arch)
    config_content = config_content.replace(
        '"x86_64-unknown-linux-gnu"',
        f'"{target_triple}"'
    )

    # Update clang include paths if provided
    if clang_paths:
        # Find the clang include section and replace it
        clang_section_start = config_content.find("# Clang builtin includes")
        if clang_section_start != -1:
            # Find the end of the clang section (next comment or section)
            lines = config_content[clang_section_start:].split('\n')
            new_clang_lines = ["    # Clang builtin includes (minimal, for compiler intrinsics only)"]

            for path in clang_paths:
                new_clang_lines.extend([
                    "    - \"-isystem\"",
                    f"    - \"{path}\""
                ])

            # Find where to stop replacing (look for next non-clang section)
            end_line = 0
            for i, line in enumerate(lines[1:], 1):
                if line.strip() and not line.startswith("    - ") and not line.startswith("    #"):
                    end_line = i
                    break

            if end_line > 0:
                before = config_content[:clang_section_start]
                after_start = clang_section_start + len('\n'.join(lines[:end_line]))
                after = config_content[after_start:]
                config_content = before + '\n'.join(new_clang_lines) + '\n' + after

    # Remove the template documentation section
    doc_start = config_content.find("\n---\n")
    if doc_start != -1:
        config_content = config_content[:doc_start]

    # Write the generated config
    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(config_content)
        print(f"Generated .clangd configuration: {output_path}")
        return True
    except OSError as e:
        print(f"Error writing config file: {e}")
        return False


def interactive_mode():
    """Interactive mode for gathering user input."""
    print("AROS .clangd Configuration Generator")
    print("=" * 40)

    # Try to auto-detect source and build directories
    detected_source = detect_source_directory()
    detected_build = detect_build_directory()

    if detected_source:
        print(f"✓ Auto-detected AROS source directory: {detected_source}")

    if detected_build:
        print(f"✓ Auto-detected AROS build directory: {detected_build}")
        use_detected = input("Use this build directory? [Y/n]: ").strip().lower()
        if use_detected in ('', 'y', 'yes'):
            build_dir = detected_build
        else:
            build_dir = input("Enter AROS build directory path: ").strip()
    else:
        print("❌ Could not auto-detect AROS build directory.")
        if detected_source:
            source_parent = str(Path(detected_source).parent)
            print(f"💡 Hint: Build directory is often a sibling of source directory.")
            print(f"   Source found at: {detected_source}")
            print(f"   Check parent directory: {source_parent}")
            print(f"   Common build dir names: aros-build, abiv1, build, out")
        print()
        build_dir = input("Enter AROS build directory path: ").strip()

    # Expand user path
    build_dir = os.path.expanduser(build_dir)

    # Validate build directory
    if not validate_directory(build_dir, "AROS build"):
        sys.exit(1)

    # Try to auto-detect target architecture
    detected_target = detect_target_architecture(build_dir)
    if detected_target:
        print(f"Auto-detected target architecture: {detected_target}")
        use_detected_target = input("Use this target? [Y/n]: ").strip().lower()
        if use_detected_target in ('', 'y', 'yes'):
            target_arch = detected_target
        else:
            target_arch = input("Enter target architecture (e.g., linux-x86_64): ").strip()
    else:
        print("Could not auto-detect target architecture.")
        print("Common targets: linux-x86_64, pc-x86_64, pc-i386, amiga-m68k")
        target_arch = input("Enter target architecture: ").strip()

    # Validate target directory
    target_path = os.path.join(build_dir, "bin", target_arch)
    if not validate_directory(target_path, f"Target ({target_arch})"):
        print("Warning: Target directory structure may not be complete.")
        print(f"Expected structure: {build_dir}/bin/{target_arch}/AROS/")

        # Show available targets if bin directory exists
        bin_path = os.path.join(build_dir, "bin")
        if os.path.exists(bin_path):
            try:
                available = [d for d in os.listdir(bin_path)
                           if os.path.isdir(os.path.join(bin_path, d))]
                if available:
                    print(f"Available targets in {bin_path}: {', '.join(available)}")
            except (OSError, PermissionError):
                pass

    # Auto-detect clang paths
    clang_paths = find_clang_include_paths()
    if clang_paths:
        print(f"Found {len(clang_paths)} clang include directories:")
        for path in clang_paths:
            print(f"  - {path}")
        use_clang = input("Use detected clang paths? [Y/n]: ").strip().lower()
        if use_clang not in ('', 'y', 'yes'):
            clang_paths = None
    else:
        print("Could not find clang include directories.")
        clang_paths = None

    # Output file
    default_output = ".clangd"
    output_path = input(f"Output file [{default_output}]: ").strip()
    if not output_path:
        output_path = default_output

    return build_dir, target_arch, output_path, clang_paths


def main():
    parser = argparse.ArgumentParser(
        description="Generate .clangd configuration for AROS development",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Common Directory Structures:
  Structure 1 (Separate directories):
    /home/user/aros/
    ├── aros-source/     # Source code (configure, mmakefile, etc.)
    └── aros-build/      # Build output (bin/, gen/, etc.)

  Structure 2 (Build within source):
    /home/user/aros-source/
    ├── configure        # Source files
    ├── mmakefile
    └── bin/             # Build output

Examples:
  %(prog)s --interactive
  %(prog)s --build-dir /home/user/aros-build --target linux-x86_64
  %(prog)s -b ~/abiv1 -t pc-x86_64 -o custom.clangd
  %(prog)s --help-structure  # Show detailed directory structure info
        """
    )

    parser.add_argument(
        "--interactive", "-i",
        action="store_true",
        help="Run in interactive mode"
    )

    parser.add_argument(
        "--build-dir", "-b",
        help="AROS build directory path"
    )

    parser.add_argument(
        "--target", "-t",
        help="Target architecture (e.g., linux-x86_64, pc-i386)"
    )

    parser.add_argument(
        "--output", "-o",
        default=".clangd",
        help="Output file path (default: .clangd)"
    )

    parser.add_argument(
        "--template",
        default="scripts/.clangd.template",
        help="Template file path (default: .clangd.template)"
    )

    parser.add_argument(
        "--no-auto-clang",
        action="store_true",
        help="Don't auto-detect clang include paths"
    )

    parser.add_argument(
        "--help-structure",
        action="store_true",
        help="Show detailed information about AROS directory structures"
    )

    args = parser.parse_args()

    # Show directory structure help
    if args.help_structure:
        print_directory_structure_help()
        return

    # Interactive mode
    if args.interactive or (not args.build_dir and not args.target):
        build_dir, target_arch, output_path, clang_paths = interactive_mode()
    else:
        if not args.build_dir or not args.target:
            parser.error("--build-dir and --target are required in non-interactive mode")

        build_dir = os.path.expanduser(args.build_dir)
        target_arch = args.target
        output_path = args.output

        # Validate directories
        if not validate_directory(build_dir, "AROS build"):
            print("Build directory validation failed.")
            print("Expected structure:")
            print("  build-directory/")
            print("    bin/")
            print("      <target-arch>/")
            print("        AROS/")
            print("          Developer/")
            print("            include/")
            sys.exit(1)

        target_path = os.path.join(build_dir, "bin", target_arch)
        if not validate_directory(target_path, f"Target ({target_arch})"):
            print("Warning: Target directory structure may not be complete.")
            print(f"Expected: {target_path}/AROS/Developer/include/")

            # Show available targets
            bin_path = os.path.join(build_dir, "bin")
            if os.path.exists(bin_path):
                try:
                    available = [d for d in os.listdir(bin_path)
                               if os.path.isdir(os.path.join(bin_path, d))]
                    if available:
                        print(f"Available targets: {', '.join(available)}")
                except (OSError, PermissionError):
                    pass

        # Auto-detect clang paths unless disabled
        clang_paths = None if args.no_auto_clang else find_clang_include_paths()

    # Generate configuration
    success = generate_config(
        args.template,
        build_dir,
        target_arch,
        output_path,
        clang_paths
    )

    if success:
        print(f"\nConfiguration generated successfully!")
        print(f"Build directory: {build_dir}")
        print(f"Target architecture: {target_arch}")
        print(f"Target triple: {get_target_triple(target_arch)}")
        if clang_paths:
            print(f"Clang includes: {len(clang_paths)} paths detected")

        # Show key paths that will be used
        developer_path = os.path.join(build_dir, "bin", target_arch, "AROS", "Developer", "include")
        if os.path.exists(developer_path):
            print(f"✓ AROS headers found: {developer_path}")
        else:
            print(f"⚠ AROS headers not found at: {developer_path}")

        print(f"\nYou can now use clangd with your AROS project.")
        print("Place the generated .clangd file in your source directory root.")
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
