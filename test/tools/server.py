#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import json
import zipfile
import hashlib
import http.server

NEW_FILENAME = 'new_client'
TARGET_FILENAME = 'client'
if sys.platform == 'win32':
    NEW_FILENAME += '.exe'
    TARGET_FILENAME += '.exe'
PACKAGE_FILE = 'package.zip'
PACKAGE_INFO_FILE = 'package_info.json'


def make_package():
    with zipfile.ZipFile(PACKAGE_FILE, 'w') as zip:
        zip.write(NEW_FILENAME, TARGET_FILENAME)
    package_file_size = os.stat(PACKAGE_FILE).st_size
    sha256 = hashlib.sha256()
    with open(PACKAGE_FILE, 'rb') as f:
        BLOCK_SIZE = 1024 * 1024
        while True:
            buffer = f.read(BLOCK_SIZE)
            if buffer is None or len(buffer) == 0:
                break
            sha256.update(buffer)
    sha256_hash = sha256.hexdigest().lower()

    package_info = {
        'package_name': 'selfupdate',
        'has_new_version': True,
        'package_version': '1.0',
        'package_url': 'http://localhost:8080/download',
        'package_size': package_file_size,
        'package_format': 'zip',
        'package_hash': {
            "sha256": sha256_hash,
        },
        'update_title': 'SelfUpdate 1.0',
        'update_description': 'This upgrade is very important!',
    }
    with open(PACKAGE_INFO_FILE, 'w') as f:
        f.write(json.dumps(package_info))


class WebServer(http.server.BaseHTTPRequestHandler):
    def do_REQUEST(self):
        if self.path == '/query':
            self.send_response(200)
            self.end_headers()
            if self.command != 'HEAD':
                with open(PACKAGE_INFO_FILE, 'rb') as f:
                    self.wfile.write(f.read())
        elif self.path == '/download':
            self.send_response(200)
            self.send_header("Content-Length",
                             str(os.stat(PACKAGE_FILE).st_size))
            self.end_headers()
            if self.command != 'HEAD':
                with open(PACKAGE_FILE, 'rb') as f:
                    self.wfile.write(f.read())
        else:
            self.send_error(404)

    do_HEAD = do_REQUEST
    do_GET = do_REQUEST
    do_POST = do_REQUEST


def run_server():
    httpd = http.server.HTTPServer(('localhost', 8080), WebServer)
    httpd.serve_forever()


def main():
    make_package()
    run_server()


if __name__ == '__main__':
    os.chdir(os.path.dirname(os.path.realpath(__file__)))
    main()
