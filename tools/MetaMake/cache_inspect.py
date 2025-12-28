#!/usr/bin/env python3
"""Inspect MetaMake mmake.cache contents."""

from __future__ import annotations

import argparse
import dataclasses
import os
import struct
import sys
from typing import BinaryIO, Dict, List, Optional

CACHE_ID_MAJOR = 0
CACHE_ID_MINOR = 9
CACHE_ID_REVISION = 0
CACHE_ID_MASK = 0xFFFF0000
CACHE_ID = (CACHE_ID_MAJOR << 24) | (CACHE_ID_MINOR << 16) | CACHE_ID_REVISION


@dataclasses.dataclass
class Target:
    name: str
    virtual: int
    deps: List[str]


@dataclasses.dataclass
class Makefile:
    name: str
    time: int
    generated: int
    targets: List[Target]


@dataclasses.dataclass
class DirNode:
    name: str
    time: int
    makefiles: List[Makefile]
    subdirs: List["DirNode"]


def read_exact(fh: BinaryIO, size: int) -> bytes:
    data = fh.read(size)
    if len(data) != size:
        raise EOFError("Unexpected end of file")
    return data


def read_int32(fh: BinaryIO) -> int:
    return struct.unpack(">i", read_exact(fh, 4))[0]


def read_uint32(fh: BinaryIO) -> int:
    return struct.unpack(">I", read_exact(fh, 4))[0]


def read_string(fh: BinaryIO) -> Optional[str]:
    length = read_int32(fh)
    if length < 0:
        return None
    if length == 0:
        return ""
    data = read_exact(fh, length)
    return data.decode("utf-8", errors="replace")


def read_makefile(fh: BinaryIO) -> Optional[Makefile]:
    name = read_string(fh)
    if name is None:
        return None
    time = read_uint32(fh)
    generated = read_uint32(fh)
    targets: List[Target] = []
    while True:
        target_name = read_string(fh)
        if target_name is None:
            break
        virtual = read_int32(fh)
        deps: List[str] = []
        while True:
            dep = read_string(fh)
            if dep is None:
                break
            deps.append(dep)
        targets.append(Target(target_name, virtual, deps))
    return Makefile(name, time, generated, targets)


def read_cachedir(fh: BinaryIO) -> Optional[DirNode]:
    name = read_string(fh)
    if name is None:
        return None
    time = read_uint32(fh)
    makefiles: List[Makefile] = []
    while True:
        makefile = read_makefile(fh)
        if makefile is None:
            break
        makefiles.append(makefile)
    subdirs: List[DirNode] = []
    while True:
        subdir = read_cachedir(fh)
        if subdir is None:
            break
        subdirs.append(subdir)
    return DirNode(name, time, makefiles, subdirs)


def parse_cache(path: str) -> tuple[List[str], DirNode, int]:
    with open(path, "rb") as fh:
        cache_id = read_uint32(fh)
        added_files: List[str] = []
        while True:
            name = read_string(fh)
            if name is None:
                break
            added_files.append(name)
        topdir = read_cachedir(fh)
        if topdir is None:
            raise ValueError("Cache did not contain a directory tree")
    return added_files, topdir, cache_id


def collect_stats(topdir: DirNode) -> dict:
    stats = {
        "dir_count": 0,
        "makefile_count": 0,
        "target_count": 0,
        "virtual_target_count": 0,
        "dep_count": 0,
        "unique_deps": set(),
        "targets": {},
    }

    def walk(node: DirNode, path_parts: List[str]) -> None:
        stats["dir_count"] += 1
        current_parts = path_parts + ([node.name] if node.name else [])
        dir_path = "/".join(part for part in current_parts if part)
        for makefile in node.makefiles:
            stats["makefile_count"] += 1
            makefile_path = f"{dir_path}/{makefile.name}" if dir_path else makefile.name
            for target in makefile.targets:
                stats["target_count"] += 1
                if target.virtual:
                    stats["virtual_target_count"] += 1
                stats["dep_count"] += len(target.deps)
                stats["unique_deps"].update(target.deps)
                target_info = stats["targets"].setdefault(
                    target.name,
                    {"deps": set(), "makefiles": set(), "virtual": False},
                )
                target_info["deps"].update(target.deps)
                target_info["makefiles"].add(makefile_path)
                if target.virtual:
                    target_info["virtual"] = True
        for subdir in node.subdirs:
            walk(subdir, current_parts)

    walk(topdir, [])
    return stats


def format_target_report(targets: Dict[str, dict]) -> str:
    lines: List[str] = []
    for name in sorted(targets.keys()):
        info = targets[name]
        virtual_suffix = " (virtual)" if info["virtual"] else ""
        lines.append(f"- {name}{virtual_suffix}")
        lines.append("  makefiles:")
        for makefile in sorted(info["makefiles"]):
            lines.append(f"    - {makefile}")
        deps = sorted(info["deps"])
        if deps:
            lines.append("  deps:")
            for dep in deps:
                lines.append(f"    - {dep}")
        else:
            lines.append("  deps: (none)")
    return "\n".join(lines)


def parse_actions(actions: List[str]) -> tuple[bool, Optional[str], Optional[str]]:
    show_list = False
    target_name: Optional[str] = None
    dep_name: Optional[str] = None
    for action in actions:
        if action == "list":
            show_list = True
            continue
        if action.startswith("target="):
            show_list = True
            target_name = action.split("=", 1)[1]
            continue
        if action.startswith("dep="):
            show_list = True
            dep_name = action.split("=", 1)[1]
            continue
        raise ValueError(f"unknown action: {action}")
    return show_list, target_name, dep_name


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Inspect a MetaMake mmake.cache file and report statistics.",
    )
    parser.add_argument(
        "--cache",
        dest="cache_path",
        default=None,
        help="Path to mmake.cache (default: ./mmake.cache)",
    )
    parser.add_argument(
        "args",
        nargs="*",
        help="Optional cache path and actions: list, target=<name>, dep=<name>",
    )
    args = parser.parse_args()

    raw_args = list(args.args)
    cache_path = args.cache_path
    if cache_path is None and raw_args:
        candidate = raw_args[0]
        if os.path.isfile(candidate):
            cache_path = candidate
            raw_args = raw_args[1:]
        elif candidate == "list" or candidate.startswith("target=") or candidate.startswith("dep="):
            cache_path = "mmake.cache"
        else:
            cache_path = candidate
            raw_args = raw_args[1:]

    if cache_path is None:
        cache_path = "mmake.cache"

    try:
        show_list, target_name, dep_name = parse_actions(raw_args)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    if not os.path.isfile(cache_path):
        print(f"error: cache file not found: {cache_path}", file=sys.stderr)
        return 2

    try:
        added_files, topdir, cache_id = parse_cache(cache_path)
    except (OSError, EOFError, ValueError) as exc:
        print(f"error: failed to parse cache: {exc}", file=sys.stderr)
        return 2

    if (cache_id & CACHE_ID_MASK) != (CACHE_ID & CACHE_ID_MASK):
        print(
            f"warning: cache ID 0x{cache_id:08x} does not match expected 0x{CACHE_ID:08x}",
            file=sys.stderr,
        )

    stats = collect_stats(topdir)

    unique_targets = len(stats["targets"])
    print("Cache statistics:")
    print(f"  directories: {stats['dir_count']}")
    print(f"  makefiles: {stats['makefile_count']}")
    print(
        f"  targets: {stats['target_count']} (unique: {unique_targets})"
    )
    print(f"  virtual targets: {stats['virtual_target_count']}")
    print(
        f"  dependencies: {stats['dep_count']} (unique: {len(stats['unique_deps'])})"
    )

    if show_list:
        print("\nMeta-target relationships:")
        targets = stats["targets"]
        if unique_targets == 0:
            print("  (none)")
        else:
            filtered = targets
            if target_name:
                if target_name in targets:
                    filtered = {target_name: targets[target_name]}
                else:
                    filtered = {}
            if dep_name:
                filtered = {
                    name: info
                    for name, info in filtered.items()
                    if dep_name in info["deps"]
                }
            if filtered:
                print(format_target_report(filtered))
            elif target_name and target_name not in targets:
                print(f"  (target not found: {target_name})")
            elif dep_name:
                print(f"  (no targets depend on: {dep_name})")
            else:
                print("  (none)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
