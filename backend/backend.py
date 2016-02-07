from .setinterval import setInterval
from .deluge import Deluge
from .torrentsyncdb import TorrentSyncDb, Task, TaskState, TaskCommand
from .log import Log
from .rsync import Rsync
import logging
from flask import Flask
from jsonrpc.backend.flask import api
import os
import signal
from subprocess import Popen
import time

logging.disable(logging.ERROR)

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
            if task.hash == "refresh":
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
        if self.params['debug']['enabled']:
            for (k,v) in self.params['debug']['tasks'].items():
                try:
                    torrents[k] = v
                except Exception as e:
                    Log('Debug task failed %s' % e.args);

        for (h,t) in torrents.items():
            t['local_size'] = self.get_local_size(self.get_local_path(t['name']))

        self.torrentsyncdb.update_torrents(torrents)

    def get_local_path(self, name):
        return "%s/%s" % (self.params['rsync']['local_dst'], name)

    def get_local_size(self, path):
        size = 0
        if os.path.exists(path):
            size = os.stat(path).st_size
            if os.path.isdir(path):
                size += sum([e.stat().st_size for e in os.scandir(path) if not e.is_dir()])
        return size

    def check_task(self, task):
        old_state = task.state
        if task.state == TaskState.init:
            if task.command == TaskCommand.start:
                try:
                    pid = self.spawn_rsync(task.hash)
                    task.set_pid(pid)
                    task.set_state(TaskState.syncing)
                except Exception as e:
                    #TODO: Completion with error message?
                    Log('Failed to spawn task: %s' % e.args)
                    task.set_state(TaskState.complete)
            else:
                task.set_state(TaskState.complete)
        elif task.state == TaskState.syncing:
            # Update local size
            torrent = self.torrentsyncdb.get_torrent(task.hash)
            torrent.local_size = self.get_local_size(self.get_local_path(torrent.name))

            is_alive = False
            if task.pid in self.procs:
                p = self.procs[task.pid]
                if p.poll() is None:
                    is_alive = True
            if task.command == TaskCommand.stop and is_alive:
                os.kill(task.pid, signal.SIGINT)
                is_alive = False
            if not is_alive:
                task.set_state(TaskState.complete)
                if task.pid in self.procs:
                    del self.procs[task.pid]

    def spawn_rsync(self, hash):
        if len(hash) > 10:
            torrent = self.torrentsyncdb.get_torrent(hash)
            rsync_params = self.params['rsync']
            src = "%s:'%s/%s'" % (rsync_params['remote_host'], torrent.save_path, torrent.name)
            dst = "%s" % (rsync_params['local_dst'])
            args = ['rsync', '-av', src, dst]
            Log(args)
        else:
            # Debug task
            args = ['sleep', self.params['debug']['tasks'][hash]['duration']]
        p = Popen(args)
        self.procs[p.pid] = p
        return p.pid

