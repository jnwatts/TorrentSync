import re
import os
import signal
import threading
import subprocess

class Rsync:
    def __init__(self, src, dst, args=None):
        _args = ['rsync', '-a', '--info=progress2']
        if args:
            _args.extend(args)
        _args.extend([src, dst])
        self.proc = subprocess.Popen(_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1)
        self.pid = self.proc.pid
        self.thread = threading.Thread(target=self.read)
        self.thread.daemon = True
        self.filter = re.compile('([0-9]+)%')
        self.progress = 0
        self.result = None

        self.thread.start()

    def read(self):
        p = self.proc
        for line in open(p.stdout.fileno()):
            self.parse(line)
        self.result = p.returncode
        if self.result != 0:
            self.errors = p.stderr.read()

    def parse(self, line):
        m = self.filter.search(line)
        if m:
            self.progress = m.group(1)

    def done(self):
        return not self.proc.poll() is None

    def kill(self):
        os.kill(self.proc.pid, signal.SIGINT)
