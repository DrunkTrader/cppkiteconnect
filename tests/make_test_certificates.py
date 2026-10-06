"""Generate synthetic, local-only TLS fixtures in the selected build tree."""

from datetime import datetime, timedelta, timezone
import pathlib
import subprocess
import sys

root = pathlib.Path(sys.argv[1])
root.mkdir(parents=True, exist_ok=True)


def openssl(*args):
    result = subprocess.run(["openssl", *map(str, args)], text=True,
                            capture_output=True)
    if result.returncode != 0:
        command = " ".join(["openssl", *map(str, args)])
        raise RuntimeError(
            "OpenSSL command failed:\n"
            f"  {command}\n"
            f"stdout: {result.stdout}\n"
            f"stderr: {result.stderr}"
        )
    return result


def openssl_date(value):
    return value.strftime("%Y%m%d%H%M%SZ")


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
    if name != "expired":
        openssl("x509", "-req", "-in", root / f"{name}.csr", "-CA", root / "ca.pem",
                "-CAkey", root / "ca.key", "-CAcreateserial", "-days", days,
                "-extfile", extensions, "-out", root / f"{name}.pem")
        continue

    # OpenSSL 3 rejects the old x509 -req form with a non-positive -days value.
    # Use a temporary CA database and explicit historical dates instead, while
    # retaining the same CA, key, CSR, certificate and hostname semantics.
    index = root / "expired.index.txt"
    serial = root / "expired.serial"
    new_certs = root / "expired-newcerts"
    config = root / "expired-ca.cnf"
    index.touch()
    serial.write_text("1000\n")
    new_certs.mkdir(exist_ok=True)
    config.write_text(f"""
[ ca ]
default_ca = test_ca

[ test_ca ]
database = {index}
serial = {serial}
new_certs_dir = {new_certs}
certificate = {root / "ca.pem"}
private_key = {root / "ca.key"}
default_md = sha256
default_days = 1
policy = policy_any
x509_extensions = expired_cert
copy_extensions = copy

[ policy_any ]
commonName = supplied

[ expired_cert ]
subjectAltName = DNS:{host}
""")
    now = datetime.now(timezone.utc)
    openssl("ca", "-batch", "-config", config,
            "-in", root / "expired.csr",
            "-out", root / "expired.pem",
            "-startdate", openssl_date(now - timedelta(days=30)),
            "-enddate", openssl_date(now - timedelta(days=1)))
