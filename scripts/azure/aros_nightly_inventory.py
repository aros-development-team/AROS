#!/usr/bin/env python3
"""Generate a deterministic, read-only AROS nightly build inventory.

When --require-archive-parity is used, the archive is streamed and each
member is checked against the observed package tree before publication.
Unsupported formats and unrepresentable metadata fail closed.

Build-host tool only. Not a signed update manifest, installer, or a
complete description of AROS filesystem metadata after extraction.

Canonical output v0 uses ASCII, LF, tab separators and hex-encoded raw POSIX
path bytes. No timestamps, traversal-order dependencies or host user IDs.
All SHA-256 digests are lower-case hex. The generation ID hashes the canonical
body with domain separation; the ID line itself is excluded.

This tool requires exclusive access to a quiescent *unpacked package tree*.
It does not claim immunity against a malicious concurrent process replacing
ancestor directories or modifying archives during packaging.
"""

import argparse
import hashlib
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
import tarfile
import tempfile
import zipfile

BLOCK = 1024 * 1024
ID_RE = re.compile(r"[a-zA-Z0-9][a-zA-Z0-9._-]{0,95}\Z", re.ASCII)
COMMIT_RE = re.compile(r"[0-9a-f]{40}(?:[0-9a-f]{24})?\Z", re.ASCII)
DOMAIN = b"AROS-nightly-observed-generation-v0\0"
MAX_ENTRIES = 1000000


class InventoryError(Exception):
    """Input is not a safely enumerable build product."""


def sha_file(path):
    """Read without following final symlink; detect a changed file on return."""
    try:
        before = path.lstat()
        if not stat.S_ISREG(before.st_mode):
            raise InventoryError(f"not a regular file: {path}")
        flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
        fd = os.open(path, flags)
        try:
            start = os.fstat(fd)
            if not stat.S_ISREG(start.st_mode) or (start.st_dev, start.st_ino) != (before.st_dev, before.st_ino):
                raise InventoryError(f"file changed before reading: {path}")
            h = hashlib.sha256()
            while True:
                chunk = os.read(fd, BLOCK)
                if not chunk:
                    break
                h.update(chunk)
            end = os.fstat(fd)
        finally:
            os.close(fd)
        finish = path.lstat()
        for a, b in [(start, end), (start, finish)]:
            if (a.st_dev, a.st_ino, a.st_mode, a.st_size, a.st_mtime_ns, a.st_ctime_ns) != \
                    (b.st_dev, b.st_ino, b.st_mode, b.st_size, b.st_mtime_ns, b.st_ctime_ns):
                raise InventoryError(f"file changed while reading: {path}")
        return start.st_size, h.hexdigest()
    except OSError as e:
        raise InventoryError(f"file read failed: {path}: {e}") from e


def inventory(root):
    """Return strictly sorted (raw path, typed record) tuples for the tree."""
    if root.is_symlink() or not root.is_dir():
        raise InventoryError("package tree must be a real directory")
    entries = []
    seen = set()
    casefolded = set()

    def visit(directory, rel):
        try:
            children = sorted(os.scandir(directory), key=lambda e: os.fsencode(e.name))
            for entry in children:
                rawname = os.fsencode(entry.name)
                if rawname in (b"", b".", b"..") or b"\x00" in rawname:
                    raise InventoryError("invalid filename")
                rawpath = rel + (b"/" if rel else b"") + rawname
                if rawpath in seen:
                    raise InventoryError("duplicate path")
                # Conservative collision check, not a claim about AROS FS case rules.
                canonical_case = os.fsdecode(rawpath).casefold()
                if canonical_case in casefolded:
                    raise InventoryError(f"case-insensitive path collision: {rawpath!r}")
                casefolded.add(canonical_case)
                seen.add(rawpath)
                path = Path(entry.path)
                st = entry.stat(follow_symlinks=False)
                mode = f"{stat.S_IMODE(st.st_mode):04o}"
                name_hex = rawpath.hex()
                if stat.S_ISDIR(st.st_mode):
                    entries.append((rawpath, f"D\t{name_hex}\t{mode}\n"))
                    visit(path, rawpath)
                elif stat.S_ISREG(st.st_mode):
                    size, digest = sha_file(path)
                    entries.append((rawpath, f"F\t{name_hex}\t{mode}\t{size}\t{digest}\n"))
                elif stat.S_ISLNK(st.st_mode):
                    target = os.fsencode(os.readlink(path))
                    # A trustworthy future extractor must never escape package root.
                    if not target or target.startswith(b"/"):
                        raise InventoryError(f"unsafe symbolic link: {rawpath!r}")
                    components = rawpath.split(b"/")[:-1]
                    for part in target.split(b"/"):
                        if part == b"..":
                            if not components:
                                raise InventoryError(f"escaping symbolic link: {rawpath!r}")
                            components.pop()
                        elif part not in (b"", b"."):
                            components.append(part)
                    entries.append((rawpath, f"L\t{name_hex}\t{mode}\t{target.hex()}\n"))
                else:
                    raise InventoryError(f"unsupported special file: {rawpath!r}")
                if len(entries) > MAX_ENTRIES:
                    raise InventoryError("entry limit exceeded")
        except OSError as e:
            raise InventoryError(f"directory walk failed: {directory}: {e}") from e

    visit(root, b"")
    return sorted(entries, key=lambda pair: pair[0])




def verify_archive_parity(archive, rows, root_mode):
    """Check archive members against this exact observed package tree.

    Read-only: members are streamed, never extracted.
    Supported: AROS Azure base .tar.bz2 and UNIX-produced .zip; any LHA/other
    archive fails closed. This checks the packaging contract, not the target's
    AROS protection bits or runtime filesystem behavior.
    """
    name = os.fsencode(archive.name)
    if name.endswith(b".tar.bz2"):
        prefix = name[:-len(b".tar.bz2")]
        kind = "tar.bz2"
    elif name.endswith(b".zip"):
        prefix = name[:-len(b".zip")]
        kind = "zip"
    else:
        raise InventoryError("archive parity unsupported format (requires tar.bz2 or zip)")
    if not prefix or b"/" in prefix or prefix in (b".", b".."):
        raise InventoryError("invalid archive top-level name")
    wanted = {p: line for p, line in rows}
    received = set()
    root_seen = False

    def check_member(path, typ, mode, stream=None, size=None, target=None):
        nonlocal root_seen
        raw = os.fsencode(path)
        # Only directory members may carry a trailing separator. Accepting it
        # on a file or link makes the archive extraction name ambiguous.
        if raw.endswith(b"/"):
            if typ != "D":
                raise InventoryError(f"non-directory archive member has trailing slash: {path!r}")
            raw = raw[:-1]
        # Fail on aliases, absolute paths, traversal, duplicate separators.
        if not raw or b"\\" in raw or any(seg in (b"", b".", b"..") for seg in raw.split(b"/")):
            raise InventoryError(f"noncanonical archive path: {path!r}")
        if raw == prefix:
            if root_seen or typ != "D":
                raise InventoryError("missing or duplicate archive root directory")
            if mode != root_mode:
                raise InventoryError("archive root mode differs from tree")
            root_seen = True
            return
        if not raw.startswith(prefix + b"/"):
            raise InventoryError(f"archive member outside package root: {path!r}")
        rel = raw[len(prefix) + 1:]
        if rel in received:
            raise InventoryError(f"duplicate archive member: {path!r}")
        received.add(rel)
        if rel not in wanted:
            raise InventoryError(f"unlisted archive member: {path!r}")
        recorded = wanted[rel].rstrip("\n").split("\t")
        if recorded[0] != typ or int(recorded[2], 8) != mode:
            raise InventoryError(f"archive type/mode differs from tree: {path!r}")
        if typ == "L":
            if target is None or os.fsencode(target).hex() != recorded[3]:
                raise InventoryError(f"archive link differs from tree: {path!r}")
        elif typ == "F":
            if size is None or str(size) != recorded[3] or stream is None:
                raise InventoryError(f"archive size differs from tree: {path!r}")
            digest = hashlib.sha256()
            length = 0
            while True:
                block = stream.read(BLOCK)
                if not block:
                    break
                length += len(block)
                if length > size:
                    raise InventoryError(f"archive file exceeds declared size: {path!r}")
                digest.update(block)
            if length != size or digest.hexdigest() != recorded[4]:
                raise InventoryError(f"archive content differs from tree: {path!r}")

    try:
        if kind == "tar.bz2":
            with tarfile.open(archive, mode="r:bz2") as tf:
                for info in tf:
                    if info.isdir():
                        check_member(info.name, "D", info.mode & 0o7777)
                    elif info.isfile():
                        with tf.extractfile(info) as source:
                            if source is None:
                                raise InventoryError(f"unreadable archive member: {info.name!r}")
                            check_member(info.name, "F", info.mode & 0o7777,
                                         source, info.size)
                    elif info.issym():
                        check_member(info.name, "L", info.mode & 0o7777,
                                     target=info.linkname)
                    else:
                        raise InventoryError(f"unsupported tar member type: {info.name!r}")
        else:
            with zipfile.ZipFile(archive, mode="r") as zf:
                for info in zf.infolist():
                    # The AROS Azure zip -r9 command emits UNIX mode metadata.
                    if info.create_system != 3:
                        raise InventoryError(f"ZIP missing UNIX mode metadata: {info.filename!r}")
                    attrs = info.external_attr >> 16
                    mode = stat.S_IMODE(attrs)
                    kind_field = stat.S_IFMT(attrs)
                    if info.is_dir():
                        if kind_field not in (0, stat.S_IFDIR):
                            raise InventoryError(f"ZIP directory type mismatch: {info.filename!r}")
                        check_member(info.filename, "D", mode)
                    elif kind_field == stat.S_IFLNK:
                        with zf.open(info, "r") as stream:
                            # Never interpret ZIP link payload as a path to extract.
                            link = stream.read(4097)
                        if len(link) > 4096 or len(link) != info.file_size:
                            raise InventoryError(f"oversize ZIP symlink: {info.filename!r}")
                        check_member(info.filename, "L", mode, target=os.fsdecode(link))
                    elif kind_field in (0, stat.S_IFREG):
                        with zf.open(info, "r") as stream:
                            check_member(info.filename, "F", mode, stream, info.file_size)
                    else:
                        raise InventoryError(f"unsupported ZIP member type: {info.filename!r}")
    except (tarfile.TarError, zipfile.BadZipFile, EOFError, OSError,
            RuntimeError, ValueError, UnicodeError) as exc:
        raise InventoryError(f"archive cannot be verified: {exc}") from exc
    if not root_seen:
        raise InventoryError("archive top-level directory missing")
    missing = wanted.keys() - received
    if missing:
        raise InventoryError(f"archive missing {len(missing)} tree entries")


def source_revision(repo):
    """Bind the origin revision to a real, clean local Git worktree."""
    if repo.is_symlink() or not repo.is_dir():
        raise InventoryError("source repo must be a real directory")
    try:
        def git(*args):
            return subprocess.check_output(
                ["git", "-C", str(repo), *args], stderr=subprocess.PIPE).decode("ascii").strip()
        commit = git("rev-parse", "--verify", "HEAD")
        if not COMMIT_RE.fullmatch(commit):
            raise InventoryError("source repository has no full commit identity")
        if git("status", "--porcelain", "--untracked-files=all"):
            raise InventoryError("source repository is not clean")
        return commit
    except (subprocess.CalledProcessError, UnicodeDecodeError, FileNotFoundError) as exc:
        raise InventoryError(f"source repository validation failed: {exc}") from exc


def emit(tree, archive, output, target, component, abi, source_commit, build_label,
         require_archive_parity=False):
    for name, value in (("target", target), ("component", component), ("build label", build_label)):
        if not ID_RE.fullmatch(value):
            raise InventoryError(f"invalid {name}: {value!r}")
    # AROS already generates Prefs/Env-Archive/ABI at build time (rom/aros).
    # Production callers should use that fact rather than a guessed ABI.
    if abi is None:
        abi_path = tree / "Prefs" / "Env-Archive" / "ABI"
        if abi_path.is_symlink() or not abi_path.is_file():
            raise InventoryError("generated Prefs/Env-Archive/ABI is missing")
        abi_bytes = abi_path.read_bytes()
        if not re.fullmatch(rb"-?[0-9]+\n?", abi_bytes, re.ASCII):
            raise InventoryError("invalid generated ABI file")
        abi = abi_bytes.decode("ascii").strip()
    if not re.fullmatch(r"-?[0-9]+", abi, re.ASCII):
        raise InventoryError("ABI must be an integer")
    if not COMMIT_RE.fullmatch(source_commit):
        raise InventoryError("source commit must be a full lower-case Git object ID")
    if archive.is_symlink() or not archive.is_file():
        raise InventoryError("archive must be a real regular file")
    if output.exists() or output.is_symlink():
        raise InventoryError("output already exists; refusing to overwrite")
    resolved_tree = tree.resolve(strict=True)
    resolved_archive = archive.resolve(strict=True)
    resolved_output_parent = output.parent.resolve(strict=True)
    if resolved_tree == resolved_output_parent or resolved_tree in resolved_output_parent.parents:
        raise InventoryError("output directory must be outside the input tree")
    if resolved_tree in resolved_archive.parents:
        raise InventoryError("archive must be outside the input tree")

    rows = inventory(tree)
    if require_archive_parity:
        # The installed tree is not just an unordered set of children: its
        # package-root name is an AROS distribution/archive identity. Verify it
        # before streaming the archive, never infer it from the contents.
        archive_name = os.fsencode(archive.name)
        if archive_name.endswith(b".tar.bz2"):
            archived_root = archive_name[:-len(b".tar.bz2")]
        elif archive_name.endswith(b".zip"):
            archived_root = archive_name[:-len(b".zip")]
        else:
            raise InventoryError("archive parity unsupported format (requires tar.bz2 or zip)")
        if os.fsencode(tree.name) != archived_root:
            raise InventoryError("archive top-level name differs from package tree")
    # The package root is a real archive entry, although it is not a relative
    # entry in the installed tree. V1 must verify and identify its mode.
    root_mode = stat.S_IMODE(tree.lstat().st_mode) if require_archive_parity else None
    archive_before = archive.lstat()
    if require_archive_parity:
        verify_archive_parity(archive, rows, root_mode)
        archive_after = archive.lstat()
        if (archive_before.st_dev, archive_before.st_ino, archive_before.st_size,
            archive_before.st_mtime_ns, archive_before.st_ctime_ns) != \
           (archive_after.st_dev, archive_after.st_ino, archive_after.st_size,
            archive_after.st_mtime_ns, archive_after.st_ctime_ns):
            raise InventoryError("archive changed during parity verification")
    records = "".join(record for _, record in rows).encode("ascii")
    tree_body = records if not require_archive_parity else \
        f"ROOT_MODE\t{root_mode:04o}\n".encode("ascii") + records
    tree_domain = b"AROS-nightly-observed-tree-v1\0" if require_archive_parity else \
        b"AROS-nightly-observed-tree-v0\0"
    tree_sha = hashlib.sha256(tree_domain + tree_body).hexdigest()
    archive_size, archive_sha = sha_file(archive)
    archive_finish = archive.lstat()
    if require_archive_parity and (
        (archive_before.st_dev, archive_before.st_ino, archive_before.st_size,
         archive_before.st_mtime_ns, archive_before.st_ctime_ns) !=
        (archive_finish.st_dev, archive_finish.st_ino, archive_finish.st_size,
         archive_finish.st_mtime_ns, archive_finish.st_ctime_ns)):
        raise InventoryError("archive changed between parity verification and hash")
    format_id = "AROS-OBSERVED-TREE-V1" if require_archive_parity else "AROS-OBSERVED-TREE-V0"
    body = [
        f"FORMAT\t{format_id}\n",
        "ASSURANCE\tUNSIGNED-BUILD-OBSERVATION\n",
        f"TARGET\t{target}\n",
        f"COMPONENT\t{component}\n",
        f"ABI\t{abi}\n",
        f"SOURCE_COMMIT\t{source_commit}\n",
        f"BUILD_LABEL\t{build_label}\n",
        f"ARCHIVE_NAME_HEX\t{os.fsencode(archive.name).hex()}\n",
        f"ARCHIVE_BYTES\t{archive_size}\n",
        f"ARCHIVE_SHA256\t{archive_sha}\n",
        f"ARCHIVE_PARITY\t{'VERIFIED' if require_archive_parity else 'UNVERIFIED'}\n" if require_archive_parity else "",
        f"ROOT_MODE\t{root_mode:04o}\n" if require_archive_parity else "",
        f"ENTRY_COUNT\t{len(rows)}\n",
        f"TREE_SHA256\t{tree_sha}\n",
        "RECORDS_BEGIN\n",
    ]
    body.append(records.decode("ascii"))
    body.append("RECORDS_END\n")
    data = "".join(body).encode("ascii")
    domain = (b"AROS-nightly-observed-generation-v1\0" if require_archive_parity else DOMAIN)
    identity = hashlib.sha256(domain + data).hexdigest()
    payload = data + f"GENERATION_SHA256\t{identity}\n".encode("ascii")
    temp_path = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".aros-inventory-", delete=False) as tmp:
            temp_path = Path(tmp.name)
            tmp.write(payload)
            tmp.flush()
            os.fsync(tmp.fileno())
        # A hard-link publish is atomic and fails if another writer has already
        # created output; unlike replace(), it cannot clobber an existing file.
        try:
            os.link(temp_path, output, follow_symlinks=False)
        except FileExistsError as exc:
            raise InventoryError("output appeared during generation") from exc
        temp_path.unlink()
        temp_path = None
        # Best-effort parent durability for this build-host metadata only.
        if hasattr(os, "O_DIRECTORY"):
            fd = os.open(output.parent, os.O_RDONLY | os.O_DIRECTORY)
            try:
                os.fsync(fd)
            finally:
                os.close(fd)
    finally:
        if temp_path is not None:
            temp_path.unlink(missing_ok=True)
    return identity, len(rows)


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--tree", type=Path, required=True)
    p.add_argument("--archive", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--target", required=True)
    p.add_argument("--component", required=True)
    p.add_argument("--abi", help="Fixture override only; normally read from Prefs/Env-Archive/ABI")
    origin = p.add_mutually_exclusive_group(required=True)
    origin.add_argument("--source-repo", type=Path, help="Preferred: derive commit from clean local Git repository")
    origin.add_argument("--source-commit", help="Explicit revision for isolated fixtures only")
    p.add_argument("--build-label", required=True)
    p.add_argument("--require-archive-parity", action="store_true",
                   help="Fail closed unless archive members match the complete input tree")
    a = p.parse_args(argv)
    try:
        source_commit = source_revision(a.source_repo) if a.source_repo else a.source_commit
        identity, count = emit(a.tree, a.archive, a.output, a.target, a.component,
                               a.abi, source_commit, a.build_label, a.require_archive_parity)
    except (InventoryError, OSError, ValueError) as exc:
        print(f"BLOCKED: {exc}", file=sys.stderr)
        return 2
    print(f"OBSERVATION-ONLY: {count} objects; generation SHA-256 {identity}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
