from .setinterval import setInterval
from .deluge import Deluge
from .torrentsyncdb import TorrentSyncDb, Task, TaskState, TaskCommand
from flask import Flask
from jsonrpc.backend.flask import api
import os
import signal
from subprocess import Popen
import time

class Backend:
    instance = None

    def __init__(self, **params):
        self.params = params['backend']
        self.deluge = Deluge(**params['deluge'])
        self.torrentsyncdb = TorrentSyncDb(**params['torrentsyncdb'])
        self.procs = {}
        self.last_check = time.time()
        self.stopTimeout = None
        Backend.instance = self

    def cleanup(self):
        if self.stopTimeout:
            self.stopTimeout.set()

    @setInterval(10)
    def timeout(self):
        self.check_tasks()

    @api.dispatcher.add_method
    def signal(self = None):
        if not self:
            self = Backend.instance
        if self:
            self.check_tasks()
        return True

    def run(self):
        self.stopTimeout = self.timeout()
        self.app = Flask(__name__)
        self.app.register_blueprint(api.as_blueprint())
        self.app.run(**self.params['rpc'])
        
    def check_tasks(self):
        if time.time() - self.last_check < self.params['min_interval']:
            return
        self.last_check = time.time()
        task = self.torrentsyncdb.oldest_task()
        if task:
            if len(task.hash) == 0:
                task.set_state(TaskState.syncing)
                self.refresh_torrents()
                task.set_state(TaskState.complete)
                self.current_task = None
            else:
                self.check_task(task)

    def refresh_torrents(self):
        labels = self.deluge.labels()
        labels = [v for v in labels if v not in self.params['ignored_labels']]
        self.torrentsyncdb.set_labels(labels)

        torrents = self.deluge.torrents()
        torrents = {k:v for (k,v) in torrents.items() if v['label'] not in self.params['ignored_labels']}
        self.torrentsyncdb.update_torrents(torrents)

    def check_task(self, task):
        if task.state == TaskState.init:
            if task.command == TaskCommand.start:
                print('Started %s' % task.hash)
                pid = self.spawn_rsync(task.hash)
                task.set_pid(pid)
                task.set_state(TaskState.syncing)
            else:
                task.set_state(TaskState.complete)
        elif task.state == TaskState.syncing:
            is_alive = False
            if task.pid in self.procs:
                p = self.procs[task.pid]
                if p.poll():
                    is_alive = True
            if task.command == TaskCommand.stop and is_alive:
                os.kill(task.pid, signal.SIGINT)
                is_alive = False
            if not is_alive:
                print('Completed %s' % task.hash)
                task.set_state(TaskState.complete)
                del self.procs[task.pid]

    def spawn_rsync(self, hash):
        torrent = self.torrentsyncdb.get_torrent(hash)
        rsync_params = self.params['rsync']
        src = "%s:'%s/%s'" % (rsync_params['remote_host'], torrent['save_path'], torrent['name'])
        dst = "%s/" % (rsync_params['local_dst'])
        args = ['rsync', '-a', src, dst]
        p = Popen(args)
        self.procs[p.pid] = p
        return p.pid

