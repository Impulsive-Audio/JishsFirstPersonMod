"""Read-only audit of the native bail-camera collision candidates.

This validates the sphere-query wrapper, not a complete collision implementation.
No game functions are loaded or called. Runtime ownership and initial-overlap
semantics remain prerequisites; a passing audit does not enable the feature.
"""
import argparse
import hashlib
import struct
from pathlib import Path

import pefile

SUPPORTED_SHA256 = "fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9"


def audit(path):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != SUPPORTED_SHA256:
        raise ValueError("Game executable differs from the inspected build")
    image = pefile.PE(str(path), fast_load=True)
    checks = 0

    def check(name, rva, expected):
        nonlocal checks
        expected = bytes.fromhex(expected)
        if image.get_data(rva, len(expected)) != expected:
            raise ValueError(name + " differs from the inspected code")
        checks += 1
        print("PASS:", name)

    check("sphere wrapper: radius at request+0x20, mode at +0x28",
          0x26c102b,
          "418b4028c5fa11442448c4c17a104020c74424400000000089442438")
    check("sphere wrapper: filter at request+0x24",
          0x26c1047, "418b40244d8d43e889442430")
    check("sphere wrapper: ignored body list at request+0x48",
          0x26c0ffb, "498d4048498943d8")
    check("sphere wrapper: allocator at request+0x40",
          0x26c0ff3, "498b4040498943f0")
    check("sphere wrapper: tolerance at +0x2c and flags at +0x30",
          0x26c0fdb, "c4c17a10402c4d8d5010488bda")
    check("sphere wrapper: flags read",
          0x26c1003, "418b403089442468")
    check("shared implementation consumes 16-byte ignore handles' indices at +8",
          0x26650e0,
          "498b0e418bd14803d2458bc141ffc18b54d10842891480")
    for name, call in (("sphere", 0x26c105d), ("ray", 0x26afe4b)):
        code = image.get_data(call, 5)
        if code[0] != 0xe8 or call + 5 + struct.unpack("<i", code[1:])[0] != 0x2664c00:
            raise ValueError(name + " does not call the shared query")
        checks += 1
        print("PASS:", name, "calls shared query")
    print(f"{checks} static checks passed; SHA-256 {digest}")
    print("OUT OF EXPERIMENT SCOPE: complete local-skater/board exclusions and initial penetration.")
    print("GAMEPLAY TEST NEEDED: engine-thread/TLS guard confirmation and native sweep behavior.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game", type=Path)
    audit(parser.parse_args().game)
