#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import shutil
import zipfile
import hashlib
import subprocess
import locale
import time


OLD_FILENAME = 'old_client'
TARGET_FILENAME = 'client'
if sys.platform == 'win32':
    OLD_FILENAME += '.exe'
    TARGET_FILENAME += '.exe'
TEST_DIR = 'test'


def copy_files():
    if os.path.exists(TEST_DIR):
        shutil.rmtree(TEST_DIR)
    os.makedirs(TEST_DIR)
    shutil.copy(
        OLD_FILENAME,
        os.path.join(TEST_DIR, TARGET_FILENAME),
    )


def run_server():
    cmd = ['python', 'server.py']
    print(' '.join(cmd))
    return subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)


def cmd(cmd):
    print(cmd)
    process = subprocess.Popen(cmd,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    encoding = locale.getpreferredencoding(False)
    (stdoutdata, _) = process.communicate()
    encoding = locale.getpreferredencoding(False)
    if stdoutdata is not None:
        return stdoutdata.decode(encoding)
    return None


def test():
    client_path = os.path.join(TEST_DIR, TARGET_FILENAME)
    result = cmd(client_path)
    print(result)
    lines = result.splitlines()
    assert lines[0].endswith('old_client launched.')
    assert lines[len(
        lines) - 1].endswith('This is the first launching since upgraded. Force updated: 0')


def main():
    copy_files()
    process = run_server()
    time.sleep(3)
    try:
        test()
    finally:
        process.kill()


if __name__ == '__main__':
    os.chdir(os.path.dirname(os.path.realpath(__file__)))
    main()
