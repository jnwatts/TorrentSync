from .setinterval import setInterval
from .deluge import Deluge
from .torrentsyncdb import TorrentSyncDb, Task, TaskState, TaskCommand
from .log import Log
from .rsync import Rsync
from .server import Server
import logging
from flask import Flask
from jsonrpc.backend.flask import api
import os
import signal
from subprocess import Popen
import time
import asyncio
import websockets
from jsonrpcserver.aio import methods
from jsonrpcserver.response import NotificationResponse

logging.disable(logging.ERROR)

class Backend:
    instance = None

    def __init__(self, **params):
        self.params = params['backend']
        self.deluge = Deluge(**params['deluge'])
        self.torrentsyncdb = TorrentSyncDb(**params['torrentsyncdb'])
        self.procs = {}
        self.last_check = time.time()
        self.update_task = None
        self.current_task = None

        Backend.instance = self

    def cleanup(self):
        if self.update_task:
            self.update_task.cancel()

    async def update_task_func(self):
        while True:
            time_since_check = time.time() - self.last_check
            if time_since_check > self.params['min_interval']:
                try:
                    await self.check_tasks()
                except Exception as e:
                    print(e)
            await asyncio.sleep(time_since_check % self.params['min_interval'])

    def run(self):
        # self.stopTimeout = self.timeout()
        print('Starting update task')
        self.update_task = asyncio.Task(self.update_task_func())
        print('Starting websocket')
        self.server = Server(self)
        asyncio.get_event_loop().run_until_complete(self.server.run('127.0.0.1', 9000))
        print('Enter main loop')
        asyncio.get_event_loop().run_forever()

    async def check_tasks(self):
        if time.time() - self.last_check < self.params['min_interval']:
            return
        self.last_check = time.time()

        task = self.current_task
        if task is None:
            task = self.torrentsyncdb.oldest_task()

        if task:
            self.current_task = task
            if task.hash == "refresh":
                task.state = TaskState.syncing
                self.refresh_torrents()
                task.state = TaskState.complete
            else:
                await self.check_task(task)

            if task.state == TaskState.complete:
                self.current_task = None

    async def refresh_torrents(self):
        Log('Syncronizing labels')
        labels = self.deluge.labels()
        labels = [v for v in labels if v not in self.params['ignored_labels']]
        self.torrentsyncdb.set_labels(labels)

        Log('Synchronizing torrents')
        torrents = self.deluge.torrents()
        torrents = {k:v for (k,v) in torrents.items() if v['label'] not in self.params['ignored_labels']}
        if self.params['debug']['enabled']:
            for (k,v) in self.params['debug']['tasks'].items():
                try:
                    torrents[k] = v
                except Exception as e:
                    Log('Debug task failed %s' % e.args);

        self.torrentsyncdb.update_torrents(torrents)

        await self.server.set_state(self.torrentsyncdb.get_torrents_by_label())

    def log(self, msg, *args):
        Log(msg, *args)

    def get_local_path(self, name):
        return "%s/%s" % (self.params['rsync']['local_dst'], name)

    def get_local_size(self, path):
        size = 0
        if os.path.exists(path):
            size = os.stat(path).st_size
            if os.path.isdir(path):
                size += sum([e.stat().st_size for e in os.scandir(path) if not e.is_dir()])
        return size

    async def get_torrent(self, hash):
        # delete matching tasks w/ state == complete
        self.torrentsyncdb.clear_completed_tasks(hash)

        # look for task w/ state != complete
        task = self.torrentsyncdb.get_incomplete_task(hash)
        if task:
            # TODO: return busy, throw exception, return error?
            print("Task is busy", task)
            return None
        else:
            # start task
            self.torrentsyncdb.start_task(hash)
            await self.check_tasks()

    async def check_task(self, task):
        old_updated = task.updated_at

        if task.state == TaskState.init:
            if task.command == TaskCommand.start:
                try:
                    self.spawn_rsync(task)
                    task.state = TaskState.syncing
                except Exception as e:
                    #TODO: Store exception in task table
                    Log('Failed to spawn task: %s' % e.args)
                    task.state = TaskState.complete
            else:
                task.state = TaskState.complete
        elif task.state == TaskState.syncing:
            # Update local size
            complete = True
            if task.pid in self.procs:
                rsync = self.procs[task.pid]
                if task.progress != rsync.progress:
                    task.progress = rsync.progress
                complete = rsync.done()
                if task.command == TaskCommand.stop:
                    rsync.kill()

            if complete:
                task.state = TaskState.complete
                if task.pid in self.procs:
                    del self.procs[task.pid]

        if task.updated_at != old_updated:
            Log(task)
            await self.server.update_task([task])

    def spawn_rsync(self, task):
        torrent = self.torrentsyncdb.get_torrent(task.hash)
        rsync_params = self.params['rsync']
        src = "%s:'%s/%s'" % (rsync_params['remote_host'], torrent.save_path, torrent.name)
        dst = "%s" % (rsync_params['local_dst'])
        rsync = Rsync(src, dst)
        self.procs[rsync.proc.pid] = rsync
        task.pid = rsync.proc.pid
