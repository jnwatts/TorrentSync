import mysql.connector
import os.path
import datetime
from enum import Enum, IntEnum
from .log import Log

class TorrentSyncDb:
    def __init__(self, **params):
        self.db_params = params;
        self.db_refcount = 0;
        self.db = None

    def get_db(self):
        if self.db_refcount == 0:
            self.db = mysql.connector.connect(**self.db_params)
        self.db_refcount += 1
        return self.db

    def put_db(self):
        if self.db_refcount > 0:
            self.db_refcount -= 1
            if self.db_refcount == 0:
                self.db.close()
                self.db = None

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

        self.put_db()

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
                    `hash`, `name`, `save_path`, `progress`, `label_id`, `time_added`, `updated_at`, `total_wanted`
                ) VALUES (
                    %s, %s, %s, %s, (SELECT `id` FROM `labels` WHERE `label` = %s), %s, NOW(), %s
                )"""
            cur.execute(sql, (t, torrent['name'], torrent['save_path'], torrent['progress'], torrent['label'], datetime.datetime.fromtimestamp(torrent['time_added']), torrent['total_wanted']))
        db.commit()

        sql = 'DELETE FROM `torrents` WHERE `updated_at` < %s'
        cur.execute(sql, [now])
        db.commit()

        self.put_db()

    def get_torrent(self, hash):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = 'SELECT * FROM `torrents` WHERE `hash` = %s'
        cur.execute(sql, [hash])
        result = cur.fetchone()
        if result is None:
            raise Exception('Hash not found: %s' % hash)

        result = Torrent(self, result)

        self.put_db()

        return result

    def get_labels(self):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = 'SELECT * FROM `labels`'
        cur.execute(sql)
        result = []
        if cur:
            result = cur.fetchall()

        self.put_db()

        return result

    def get_torrents_by_label(self, label_id = None):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        tables = {
            'torrents': [
                'hash',
                'name',
                'progress',
                'progress',
                'total_wanted',
                'time_added',
            ],
            'labels': [
                'label',
            ],
            'tasks': [
                'state',
                'progress',
            ],
        };

        rows = [];
        for t,rs in tables.items():
            rows += ['{0}.{1} AS \'{0}.{1}\''.format(t,r) for r in rs]
        sql = 'SELECT ' + ','.join(rows)
        sql += ' FROM torrents JOIN labels on torrents.label_id = labels.id LEFT JOIN tasks ON torrents.hash = tasks.hash'
        if label_id:
            sql += ' WHERE torrents.label_id = %s'
            cur.execute(sql, [label_id])
        else:
            cur.execute(sql)

        result = []
        if cur:
            rows = cur.fetchall()
            for row in rows:
                o = {}
                for c,val in row.items():
                    [t,p] = c.split('.')
                    if not t in o:
                        o[t] = {}
                    o[t][p] = val
                result.append(o)
        result = [Torrent_ng(self, r) for r in result]

        self.put_db();

        return result

    def oldest_task(self):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = """SELECT `id`, `hash`, `command`+0 AS `command`, `state`+0 AS `state`, `pid`, `progress`, `updated_at`
                    FROM `tasks`
                    WHERE `state` != 'complete'
                    ORDER BY `created_at` ASC LIMIT 1"""
        cur.execute(sql)

        row = cur.fetchone()
        if row:
            task = Task(self, row)
        else:
            task = None

        self.put_db()

        return task

    def clear_completed_tasks(self, hash = None):
        print("STUB: clear_completed_tasks")

    def get_incomplete_task(self, hash):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = """SELECT `id`, `hash`, `command`+0 AS `command`, `state`+0 AS `state`, `pid`, `progress`, `updated_at`
                    FROM `tasks`
                    WHERE `state` != 'complete' AND `hash` = %s
                    ORDER BY `updated_at` ASC LIMIT 1"""
        cur.execute(sql, [hash])

        row = cur.fetchone()
        if row:
            task = Task(self, row)
        else:
            task = None

        self.put_db()

        return task

    def get_task(self, hash):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = """SELECT `id`, `hash`, `command`+0 AS `command`, `state`+0 AS `state`, `pid`, `progress`, `updated_at`
                    FROM `tasks`
                    WHERE `hash` = %s
                    ORDER BY `updated_at` DESC LIMIT 1"""
        cur.execute(sql, [hash])

        row = cur.fetchone()
        if row:
            task = Task(self, row)
        else:
            task = None

        self.put_db()

        return task

    def get_label(self, label_id):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = 'SELECT * FROM `labels` WHERE `id` = %s'
        cur.execute(sql, [label_id])
        
        row = cur.fetchone()
        if row:
            result = row

        self.put_db()

        return result

    def start_task(self, hash):
        db = self.get_db()
        cur = db.cursor(dictionary=True)

        sql = "INSERT INTO `tasks` (`hash`, `command`, `state`) VALUES (%s, %s, %s)"
        cur.execute(sql, [hash, 'start', 'init'])

        sql = """SELECT `id`, `hash`, `command`+0 AS `command`, `state`+0 AS `state`, `pid`, `progress`, `updated_at`
                    FROM `tasks`
                    WHERE `state` != 'complete' AND `hash` = %s
                    ORDER BY `created_at` ASC LIMIT 1"""
        cur.execute(sql, [hash])

        task = None
        if cur:
            task = Task(self, cur.fetchone())

        if task:
            db.commit()

        self.put_db()

        return task

class TdbObject:
    def __init__(self, tdb, row, **params):
        self._tdb = tdb
        self._table = params['table']
        self._ids = params['ids']
        self._refs = params['refs']
        self._fields = list(row.keys())
        for r in self._refs:
            self.__setattr__(r, None)
        for f in row:
            super(TdbObject, self).__setattr__(f, self.__fromsql__(f, row[f]));

    def __setattr__(self, attr, value):
        if not attr.startswith('_') and not attr in self._ids + self._refs:
            db = self._tdb.get_db()
            cur = db.cursor()

            ids = self._ids

            sql = 'UPDATE `' + self._table + '` SET `' + attr + '` = %s, `updated_at` = NOW() WHERE '
            sql += ' AND '.join(['`' + id + '` = %s' for id in ids])
            params = (self.__tosql__(attr, value),) + tuple([getattr(self, id) for id in ids])

            # print("Invoking", sql, params)

            cur.execute(sql, params)
            db.commit()

            sql = 'SELECT `updated_at` FROM `' + self._table + '` WHERE '
            sql += ' AND '.join(['`' + id + '` = %s' for id in ids])
            params = tuple([getattr(self, id) for id in ids])
            cur.execute(sql, params)
            super(TdbObject, self).__setattr__('updated_at', cur.fetchone()[0])

            self._tdb.put_db()
        return super(TdbObject, self).__setattr__(attr, value)

    def toJson(self):
        return {f: getattr(self, f) for f in self._fields + self._refs}

class Torrent_ng(TdbObject):
    def __init__(self, tdb, obj):
        super(Torrent_ng, self).__init__(
                tdb,
                obj['torrents'],
                table='torrents',
                ids = ['hash'],
                refs = ['task', 'label'])
        self.task = obj['tasks']
        self.label = obj['labels']

    def __fromsql__(self, attr, value):
        return value

    def __tosql__(self, attr, value):
        return value

class Torrent(TdbObject):
    def __init__(self, tdb, row):
        super(Torrent, self).__init__(
                tdb,
                row,
                table='torrents',
                ids = ['hash'],
                refs = ['task', 'label']);
        self.task = tdb.get_task(self.hash)
        self.label = tdb.get_label(self.label_id)

    def __fromsql__(self, attr, value):
        return value

    def __tosql__(self, attr, value):
        return value

class Task(TdbObject):
    def __init__(self, tdb, row):
        super(Task, self).__init__(
                tdb,
                row,
                table='tasks',
                ids = ['id', 'hash'],
                refs = []);

    def __tosql__(self, attr, value):
        if isinstance(value, IntEnum):
            value = value.name
        return value

    def __fromsql__(self, attr, value):
        if attr == 'command':
            return TaskCommand(value)
        elif attr == 'state':
            return TaskState(value)
        else:
            return value

    def __str__(self):
        return "Task(%s, %s, %s, %s, %s)" % (self.hash, self.command, self.state, self.progress, self.pid)

class TaskState(IntEnum):
    init = 1
    syncing = 2
    complete = 3

class TaskCommand(IntEnum):
    start = 1
    idle = 2
    stop = 3
