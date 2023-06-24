#!/usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, shutil, zipfile, hashlib
from http.server import HTTPServer, BaseHTTPRequestHandler

if sys.platform == "win32":
    old_filename = "old_client.exe"
    new_filename = "new_client.exe"
    target_client_filename = "client.exe"
    server_filename = "selfupdate_server.exe"
else:
    old_filename = "old_client"
    new_filename = "new_client"
    target_client_filename = "client"
    server_filename = "selfupdate_server"


def copy_files(dir):
    test_dir = os.path.join(dir, "test")
    if os.path.exists(test_dir):
        shutil.rmtree(test_dir)
    os.makedirs(test_dir)
    client_dir = os.path.join(test_dir, "client")
    os.makedirs(client_dir)
    shutil.copy(
        os.path.join(dir, old_filename),
        os.path.join(client_dir, target_client_filename),
    )

    server_dir = os.path.join(test_dir, "server")
    os.makedirs(server_dir)
    shutil.copy(os.path.join(dir, server_filename), server_dir)


def make_package(dir):
    src_file = os.path.join(dir, new_filename)
    server_dir = os.path.join(dir, "test", "server")
    package_file = os.path.join(server_dir, "sample_package.zip")
    with zipfile.ZipFile(package_file, "w") as zip:
        zip.write(src_file, target_client_filename)

    package_file_size = os.stat(package_file).st_size

    sha256 = hashlib.sha256()
    with open(package_file, "rb") as f:
        BLOCK_SIZE = 1024 * 1024
        while True:
            buffer = f.read(BLOCK_SIZE)
            if buffer is None or len(buffer) == 0:
                break
            sha256.update(buffer)
    sha256_hash = sha256.hexdigest().lower()

    yaml = """package: sample_package
versions:
  2.0:
    url: http://localhost:8080/sample_package.zip
    size: %d
    format: zip
    hash:
      sha256: %s
    title: Sample 2.0
    description: This version introduces a lot of new features
policies:
  - matches:
      - '[,2.0)'
    target: 2.0
""" % (
        package_file_size,
        sha256_hash,
    )

    config_file = os.path.join(server_dir, "sample_package.yaml")
    with open(config_file, "w") as f:
        f.write(yaml)


def run_server(dir):
    server_dir = os.path.join(dir, "test", "server")
    server_file = os.path.join(server_dir, server_filename)
    cmd = server_file + " --port=8080 --config=%s --root=%s" % (server_dir, server_dir)

    print(cmd)
    os.system(cmd)


def main():
    dir = sys.argv[1]
    copy_files(dir)
    make_package(dir)
    run_server(dir)


if __name__ == "__main__":
    main()
