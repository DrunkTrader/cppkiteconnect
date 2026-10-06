"""Generate synthetic, local-only TLS fixtures in the selected build tree."""

import pathlib
import subprocess
import sys

root = pathlib.Path(sys.argv[1])
root.mkdir(parents=True, exist_ok=True)


def openssl(*args):
    subprocess.run(["openssl", *map(str, args)], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "2",
        "-subj", "/CN=cppkiteconnect test CA", "-keyout", root / "ca.key",
        "-out", root / "ca.pem")

for name, host, days in [("trusted", "localhost", "2"),
                         ("wronghost", "wrong.invalid", "2"),
                         ("expired", "localhost", "-1")]:
    extensions = root / f"{name}.ext"
    extensions.write_text(f"subjectAltName=DNS:{host}\n")
    openssl("req", "-newkey", "rsa:2048", "-nodes", "-subj", f"/CN={host}",
            "-keyout", root / f"{name}.key", "-out", root / f"{name}.csr")
    openssl("x509", "-req", "-in", root / f"{name}.csr", "-CA", root / "ca.pem",
            "-CAkey", root / "ca.key", "-CAcreateserial", "-days", days,
            "-extfile", extensions, "-out", root / f"{name}.pem")
