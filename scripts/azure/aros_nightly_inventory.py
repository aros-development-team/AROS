#!/usr/bin/env python3
"""Generate a deterministic, read-only AROS nightly build inventory."""

#
# When --require-archive-parity is used, the archive is streamed and each
# member is checked against the observed package tree before publication.
# Unsupported formats and unrepresentable metadata fail closed.
#
# Build-host tool only. Not a signed update manifest, installer, or a
# complete description of AROS filesystem metadata after extraction.
#
# Canonical output v0 uses ASCII, LF, tab separators and hex-encoded raw POSIX
# path bytes. No timestamps, traversal-order dependencies or host user IDs.
# All SHA-256 digests are lower-case hex. The generation ID hashes the canonical
# body with domain separation; the ID line itself is excluded.
#
# This tool requires exclusive access to a quiescent *unpacked package tree*.
# It does not claim immunity against a malicious concurrent process replacing
# ancestor directories or modifying archives during packaging.

import argparse
import hashlib
import os
from pathlib import Path
import re
import stat
# Fixed-argument Git query, never a shell command.
import subprocess  # nosec B404
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


def _checked_link_target(rawpath, target):
    """Reject symbolic links that would escape the package tree."""
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
    return target.hex()


def _inventory_entry(entry, rawpath):
    """Encode one filesystem entry and indicate whether it needs traversal."""
    path = Path(entry.path)
    st = entry.stat(follow_symlinks=False)
    mode = f"{stat.S_IMODE(st.st_mode):04o}"
    name_hex = rawpath.hex()
    if stat.S_ISDIR(st.st_mode):
        return f"D\t{name_hex}\t{mode}\n", True
    if stat.S_ISREG(st.st_mode):
        size, digest = sha_file(path)
        return f"F\t{name_hex}\t{mode}\t{size}\t{digest}\n", False
    if stat.S_ISLNK(st.st_mode):
        target = os.fsencode(os.readlink(path))
        target_hex = _checked_link_target(rawpath, target)
        return f"L\t{name_hex}\t{mode}\t{target_hex}\n", False
    raise InventoryError(f"unsupported special file: {rawpath!r}")


def _accept_inventory_path(rawpath, seen, casefolded):
    """Reject duplicate paths and conservative Unicode case collisions."""
    if rawpath in seen:
        raise InventoryError("duplicate path")
    # Conservative collision check, not a claim about AROS FS case rules.
    canonical_case = os.fsdecode(rawpath).casefold()
    if canonical_case in casefolded:
        raise InventoryError(f"case-insensitive path collision: {rawpath!r}")
    casefolded.add(canonical_case)
    seen.add(rawpath)


def _visit_inventory(directory, rel, entries, seen, casefolded):
    """Enumerate an unpacked tree without following symbolic links."""
    try:
        children = sorted(os.scandir(directory), key=lambda e: os.fsencode(e.name))
        for entry in children:
            rawname = os.fsencode(entry.name)
            if rawname in (b"", b".", b"..") or b"\x00" in rawname:
                raise InventoryError("invalid filename")
            rawpath = rel + (b"/" if rel else b"") + rawname
            _accept_inventory_path(rawpath, seen, casefolded)
            record, descend = _inventory_entry(entry, rawpath)
            entries.append((rawpath, record))
            if descend:
                _visit_inventory(Path(entry.path), rawpath, entries, seen, casefolded)
            if len(entries) > MAX_ENTRIES:
                raise InventoryError("entry limit exceeded")
    except OSError as e:
        raise InventoryError(f"directory walk failed: {directory}: {e}") from e


def inventory(root):
    """Return strictly sorted (raw path, typed record) tuples for the tree."""
    if root.is_symlink() or not root.is_dir():
        raise InventoryError("package tree must be a real directory")
    entries = []
    _visit_inventory(root, b"", entries, set(), set())
    return sorted(entries, key=lambda pair: pair[0])



def _archive_format(archive):
    """Resolve the exact package root and supported archive format."""
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
    return kind, prefix


def _archive_member_path(path, typ):
    """Validate raw archive spelling before looking up a tree record."""
    raw = os.fsencode(path)
    if raw.endswith(b"/"):
        if typ != "D":
            raise InventoryError(f"non-directory archive member has trailing slash: {path!r}")
        raw = raw[:-1]
    if not raw or b"\\" in raw or any(seg in (b"", b".", b"..") for seg in raw.split(b"/")):
        raise InventoryError(f"noncanonical archive path: {path!r}")
    return raw


def _archive_file_matches(path, stream, size, recorded):
    """Stream a regular member and compare its exact size and digest."""
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


class _ArchiveMembers:
    """Track package root, exact archive membership and tree records."""

    def __init__(self, prefix, rows, root_mode):
        self.prefix = prefix
        self.wanted = {p: line for p, line in rows}
        self.received = set()
        self.root_mode = root_mode
        self.root_seen = False

    def check_root(self, typ, mode):
        """Check one explicit package-root directory entry."""
        if self.root_seen or typ != "D":
            raise InventoryError("missing or duplicate archive root directory")
        if mode != self.root_mode:
            raise InventoryError("archive root mode differs from tree")
        self.root_seen = True

    def check_record(self, path, rel, typ, mode, stream, size, target):
        """Verify one non-root member against the expected tree row."""
        if rel in self.received:
            raise InventoryError(f"duplicate archive member: {path!r}")
        self.received.add(rel)
        if rel not in self.wanted:
            raise InventoryError(f"unlisted archive member: {path!r}")
        recorded = self.wanted[rel].rstrip("\n").split("\t")
        if recorded[0] != typ or int(recorded[2], 8) != mode:
            raise InventoryError(f"archive type/mode differs from tree: {path!r}")
        if typ == "L":
            if target is None or os.fsencode(target).hex() != recorded[3]:
                raise InventoryError(f"archive link differs from tree: {path!r}")
        elif typ == "F":
            _archive_file_matches(path, stream, size, recorded)

    def check_member(self, path, typ, mode, stream=None, size=None, target=None):
        """Accept one member only in its exact expected location."""
        raw = _archive_member_path(path, typ)
        if raw == self.prefix:
            self.check_root(typ, mode)
            return
        if not raw.startswith(self.prefix + b"/"):
            raise InventoryError(f"archive member outside package root: {path!r}")
        rel = raw[len(self.prefix) + 1:]
        self.check_record(path, rel, typ, mode, stream, size, target)

    def complete(self):
        """Reject incomplete or rootless archives."""
        if not self.root_seen:
            raise InventoryError("archive top-level directory missing")
        missing = self.wanted.keys() - self.received
        if missing:
            raise InventoryError(f"archive missing {len(missing)} tree entries")


def _check_tar_archive(archive, verifier):
    """Stream and validate members from the AROS tar.bz2 archive."""
    with tarfile.open(archive, mode="r:bz2") as tf:
        for info in tf:
            if info.isdir():
                verifier.check_member(info.name, "D", info.mode & 0o7777)
            elif info.isfile():
                with tf.extractfile(info) as source:
                    if source is None:
                        raise InventoryError(f"unreadable archive member: {info.name!r}")
                    verifier.check_member(info.name, "F", info.mode & 0o7777,
                                          source, info.size)
            elif info.issym():
                verifier.check_member(info.name, "L", info.mode & 0o7777,
                                      target=info.linkname)
            else:
                raise InventoryError(f"unsupported tar member type: {info.name!r}")


def _check_zip_member(zf, info, verifier, mode, kind_field):
    """Check ZIP type-specific metadata and pass through content streams."""
    if info.is_dir():
        if kind_field not in (0, stat.S_IFDIR):
            raise InventoryError(f"ZIP directory type mismatch: {info.filename!r}")
        verifier.check_member(info.filename, "D", mode)
    elif kind_field == stat.S_IFLNK:
        with zf.open(info, "r") as stream:
            # Never interpret ZIP link payload as a path to extract.
            link = stream.read(4097)
        if len(link) > 4096 or len(link) != info.file_size:
            raise InventoryError(f"oversize ZIP symlink: {info.filename!r}")
        verifier.check_member(info.filename, "L", mode, target=os.fsdecode(link))
    elif kind_field in (0, stat.S_IFREG):
        with zf.open(info, "r") as stream:
            verifier.check_member(info.filename, "F", mode, stream, info.file_size)
    else:
        raise InventoryError(f"unsupported ZIP member type: {info.filename!r}")


def _check_zip_archive(archive, verifier):
    """Stream and validate members from a UNIX-produced ZIP archive."""
    with zipfile.ZipFile(archive, mode="r") as zf:
        for info in zf.infolist():
            # The AROS Azure zip -r9 command emits UNIX mode metadata.
            if info.create_system != 3:
                raise InventoryError(f"ZIP missing UNIX mode metadata: {info.filename!r}")
            attrs = info.external_attr >> 16
            _check_zip_member(zf, info, verifier, stat.S_IMODE(attrs), stat.S_IFMT(attrs))



def verify_archive_parity(archive, rows, root_mode):
    """Check archive members against this exact observed package tree."""
    # Read-only: members are streamed, never extracted.
    # Supported: AROS Azure base .tar.bz2 and UNIX-produced .zip; any LHA/other
    # archive fails closed. This checks the packaging contract, not the target's
    # AROS protection bits or runtime filesystem behavior.
    kind, prefix = _archive_format(archive)
    verifier = _ArchiveMembers(prefix, rows, root_mode)
    try:
        if kind == "tar.bz2":
            _check_tar_archive(archive, verifier)
        else:
            _check_zip_archive(archive, verifier)
    except (tarfile.TarError, zipfile.BadZipFile, EOFError, OSError,
            RuntimeError, ValueError, UnicodeError) as exc:
        raise InventoryError(f"archive cannot be verified: {exc}") from exc
    verifier.complete()



def source_revision(repo):
    """Bind the origin revision to a real, clean local Git worktree."""
    if repo.is_symlink() or not repo.is_dir():
        raise InventoryError("source repo must be a real directory")
    try:
        def git(*args):
            # All call sites supply fixed Git subcommands.  The source path is
            # one argv element; neither shell parsing nor command expansion occurs.
            return subprocess.check_output(  # nosec B603
                ["git", "-C", str(repo), *args], stderr=subprocess.PIPE).decode("ascii").strip()
        commit = git("rev-parse", "--verify", "HEAD")
        if not COMMIT_RE.fullmatch(commit):
            raise InventoryError("source repository has no full commit identity")
        if git("status", "--porcelain", "--untracked-files=all"):
            raise InventoryError("source repository is not clean")
        return commit
    except (subprocess.CalledProcessError, UnicodeDecodeError, FileNotFoundError) as exc:
        raise InventoryError(f"source repository validation failed: {exc}") from exc


def _validated_abi(tree, abi):
    """Use the build-generated ABI unless a fixture supplies an override."""
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
    return abi


def _validate_emit_inputs(tree, archive, output, target, component, source_commit, build_label):
    """Validate identity strings and the input/output containment contract."""
    for name, value in (("target", target), ("component", component), ("build label", build_label)):
        if not ID_RE.fullmatch(value):
            raise InventoryError(f"invalid {name}: {value!r}")
    if not COMMIT_RE.fullmatch(source_commit):
        raise InventoryError("source commit must be a full lower-case Git object ID")
    _validate_io_containment(tree, archive, output)


def _validate_io_containment(tree, archive, output):
    """Keep output outside the source tree and reject any clobbering."""
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



def _validate_archive_tree_name(tree, archive):
    """Require the package tree name to match the root archived by Azure."""
    _, archived_root = _archive_format(archive)
    if os.fsencode(tree.name) != archived_root:
        raise InventoryError("archive top-level name differs from package tree")


def _archive_stamp(path):
    """Capture the metadata used to detect archive mutation during reading."""
    st = path.lstat()
    return st.st_dev, st.st_ino, st.st_size, st.st_mtime_ns, st.st_ctime_ns


def _inventory_payload(tree, archive, rows, root_mode, parity, abi, source_commit,
                       target, component, build_label):
    """Render the immutable v0/v1 wire format and its generation identity."""
    records = "".join(record for _, record in rows).encode("ascii")
    tree_body = records if not parity else f"ROOT_MODE\t{root_mode:04o}\n".encode("ascii") + records
    tree_domain = b"AROS-nightly-observed-tree-v1\0" if parity else b"AROS-nightly-observed-tree-v0\0"
    tree_sha = hashlib.sha256(tree_domain + tree_body).hexdigest()
    archive_size, archive_sha = sha_file(archive)
    format_id = "AROS-OBSERVED-TREE-V1" if parity else "AROS-OBSERVED-TREE-V0"
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
        f"ARCHIVE_PARITY\t{'VERIFIED' if parity else 'UNVERIFIED'}\n" if parity else "",
        f"ROOT_MODE\t{root_mode:04o}\n" if parity else "",
        f"ENTRY_COUNT\t{len(rows)}\n",
        f"TREE_SHA256\t{tree_sha}\n",
        "RECORDS_BEGIN\n",
    ]
    body.append(records.decode("ascii"))
    body.append("RECORDS_END\n")
    data = "".join(body).encode("ascii")
    domain = b"AROS-nightly-observed-generation-v1\0" if parity else DOMAIN
    identity = hashlib.sha256(domain + data).hexdigest()
    return data + f"GENERATION_SHA256\t{identity}\n".encode("ascii"), identity


def _publish_inventory(output, payload):
    """Atomically publish a new sidecar without overwriting an existing one."""
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


def emit(tree, archive, output, target, component, abi, source_commit, build_label,
         require_archive_parity=False):
    """Generate a frozen, unsigned inventory and optional archive parity proof."""
    _validate_emit_inputs(tree, archive, output, target, component, source_commit, build_label)
    # AROS generates Prefs/Env-Archive/ABI at build time (rom/aros).
    abi = _validated_abi(tree, abi)
    rows = inventory(tree)
    if require_archive_parity:
        _validate_archive_tree_name(tree, archive)
    # The archive root directory has its own mode, not a relative tree row.
    root_mode = stat.S_IMODE(tree.lstat().st_mode) if require_archive_parity else None
    archive_before = _archive_stamp(archive)
    if require_archive_parity:
        verify_archive_parity(archive, rows, root_mode)
        if archive_before != _archive_stamp(archive):
            raise InventoryError("archive changed during parity verification")
    payload, identity = _inventory_payload(tree, archive, rows, root_mode,
                                           require_archive_parity, abi, source_commit,
                                           target, component, build_label)
    if require_archive_parity and archive_before != _archive_stamp(archive):
        raise InventoryError("archive changed between parity verification and hash")
    _publish_inventory(output, payload)
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
