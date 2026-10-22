"""Generate copy/literal deltas against the matching user's original resources.

Run locally after building the overlay. Do not publish source or output LMPs.
"""
from pathlib import Path
import hashlib
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'host/assets/res'
OUTPUT = ROOT / 'patches/ui'


def digest(data):
    return hashlib.sha256(data).digest()


def delta(source, target):
    # A bounded set of candidate matches avoids quadratic behavior in zero padding.
    index = {}
    for offset in range(0, len(source) - 15, 8):
        key = source[offset:offset + 16]
        bucket = index.setdefault(key, [])
        if len(bucket) < 8:
            bucket.append(offset)
    commands, pending = bytearray(), bytearray()

    def flush():
        if pending:
            commands.extend(b'\x01' + struct.pack('<I', len(pending)) + pending)
            pending.clear()

    pos = 0
    while pos < len(target):
        best, start = 0, 0
        for candidate in index.get(target[pos:pos + 16], ()):
            size = 16
            limit = min(len(source) - candidate, len(target) - pos)
            while size + 64 <= limit and source[candidate+size:candidate+size+64] == target[pos+size:pos+size+64]:
                size += 64
            while size < limit and source[candidate+size] == target[pos+size]:
                size += 1
            if size > best:
                best, start = size, candidate
        if best >= 16:
            flush()
            commands.extend(b'\x00' + struct.pack('<II', start, best))
            pos += best
        else:
            pending.append(target[pos])
            pos += 1
    flush()
    compressor = zlib.compressobj(9, zlib.DEFLATED, -15)
    return compressor.compress(commands) + compressor.flush()


def build():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for file in sorted((ROOT / 'host/ui').rglob('*')):
        if not file.is_file():
            continue
        relative = file.relative_to(ROOT / 'host/ui').as_posix()
        names = ['font.lmp', 'frontend.lmp', 'hud.lmp'] if file.suffix == '.bin' else [file.name]
        sources = [(name, (SOURCE / name).read_bytes()) for name in names]
        target = file.read_bytes()
        header = bytearray(b'JLD1' + struct.pack('<I', len(sources)))
        for name, original in sources:
            encoded = name.encode('utf-8')
            header.extend(struct.pack('<H', len(encoded)) + encoded + digest(original))
        header.extend(struct.pack('<I', len(target)) + digest(target))
        patch = header + delta(b''.join(data for _, data in sources), target)
        path = OUTPUT / (relative + '.jldp')
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(patch)
        print(f'{relative}: {len(target)} -> {len(patch)} bytes')


if __name__ == '__main__':
    build()
