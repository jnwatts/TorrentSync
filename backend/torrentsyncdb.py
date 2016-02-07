import mysql.connector
import os.path
import datetime
from enum import IntEnum
from .log import Log

class TorrentSyncDb:
    def __init__(self, **params):
        self.db_params = params;

    def get_db(self):
        return mysql.connector.connect(**self.db_params)

    def set_labels(self, labels):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        db_labels = {}
        cur.execute('SELECT `id`, `label` FROM `labels`')
        for row in cur:
            db_labels[row['id']] = row['label']

        labels = [label for label in labels if label != '']
        labels.insert(0, 'unlabeled')

        deleted_labels = []
        for id in db_labels:
            if db_labels[id] not in labels:
                deleted_labels.append(id)

        new_labels = []
        for label in labels:
            if label not in db_labels.values():
                new_labels.append(label)

        if new_labels:
            sql = 'INSERT INTO `labels` (`label`) VALUES ' + ','.join(["(%s)"] * len(new_labels))
            cur.execute(sql, (new_labels))
            db.commit()

        if deleted_labels:
            sql = 'DELETE FROM `labels` WHERE `id` IN (' + ','.join(['%s'] * len(deleted_labels)) + ')'
            cur.execute(sql, (deleted_labels))
            db.commit()

        db.close()

    def update_torrent(self, torrent):
        self.update_torrents({torrent['hash']: torrent})

    def update_torrents(self, torrents):
        db = self.get_db()
        cur = db.cursor()

        cur.execute('SELECT NOW()')
        now = cur.fetchone()[0]

        for t in torrents:
            torrent = torrents[t]

            if (len(torrent['label']) == 0):
                torrent['label'] = 'unlabeled'
            sql = """REPLACE INTO `torrents` (
                    `hash`, `name`, `save_path`, `progress`, `label_id`, `time_added`, `updated_at`, `total_wanted`, `local_size`
                ) VALUES (
                    %s, %s, %s, %s, (SELECT `id` FROM `labels` WHERE `label` = %s), %s, NOW(), %s, %s
                )"""
            cur.execute(sql, (t, torrent['name'], torrent['save_path'], torrent['progress'], torrent['label'], datetime.datetime.fromtimestamp(torrent['time_added']), torrent['total_wanted'], torrent['local_size']))
        db.commit()

        sql = 'DELETE FROM `torrents` WHERE `updated_at` < %s'
        cur.execute(sql, [now])
        db.commit()

        db.close()

    def get_torrent(self, hash):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = 'SELECT * FROM `torrents` WHERE `hash` = %s'
        cur.execute(sql, [hash])
        result = cur.fetchone()
        if result is None:
            raise Exception('Hash not found: %s' % hash)

        result = Torrent(self, result)

        db.close()

        return result

    def oldest_task(self):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = """SELECT `id`, `hash`, `command`+0 AS `command`, `state`+0 AS `state`, `pid`
                    FROM `tasks`
                    WHERE `state` != 'complete'
                    ORDER BY `created_at` ASC LIMIT 1"""
        cur.execute(sql)

        task = None
        for row in cur:
            task = Task(tdb=self, **row)
            break

        db.close()

        return task

class Torrent:
    def __init__(self, tdb, row):
        self.tdb = tdb
        self.fields = []
        for f in row:
            if f != 'hash':
                # Don't try to update hash column
                self.fields.append(f)
            setattr(self, f, row[f])

    def __setattr__(self, name, value):
        if hasattr(self, 'fields') and hasattr(self, 'hash'):
            if name in self.fields:
                db = self.tdb.get_db()
                cur = db.cursor()

                sql = 'UPDATE `torrents` SET `' + name + '` = %s, `updated_at` = NOW() WHERE `hash` = %s'
                cur.execute(sql, (value, self.hash))
                db.commit()

                db.close()
        return super(Torrent, self).__setattr__(name, value)

class Task:
    def __init__(self, **params):
        self.tdb = params['tdb']
        self.id = params['id']
        self.pid = params['pid']
        self.hash = params['hash']
        self.command = TaskCommand(params['command'])
        self.state = TaskState(params['state'])

    def set_state(self, state):
        self.state = state

        db = self.tdb.get_db()
        cur = db.cursor()

        sql = 'UPDATE `tasks` SET `state` = %s, `updated_at` = NOW() WHERE `id` = %s'
        Log('TASK %s %s' % (self.state.name, self.hash));
        cur.execute(sql, (self.state.name, self.id))
        db.commit()

        db.close()

    def set_pid(self, pid):
        self.pid = pid

        db = self.tdb.get_db()
        cur = db.cursor()

        sql = 'UPDATE `tasks` SET `pid` = %s, `updated_at` = NOW() WHERE `id` = %s'
        cur.execute(sql, (self.pid, self.id))
        db.commit()

        db.close()

    def __str__(self):
        return "Task(%s, %s, %s, %s)" % (self.hash, self.command, self.state, self.pid)

class TaskState(IntEnum):
    init = 1
    syncing = 2
    complete = 3

class TaskCommand(IntEnum):
    start = 1
    idle = 2
    stop = 3
