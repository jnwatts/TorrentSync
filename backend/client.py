import json

from datetime import date, datetime
def json_serial(obj):
    if isinstance(obj, (datetime, date)):
        return obj.isoformat()
    else:
        toJson = getattr(obj, 'toJson', None)
        if toJson:
            return toJson()
    raise TypeError ("Type %s not serializable" % type(obj))

class Client():
    __last_id = 0

    def __init__(self, websocket):
        self.id = Client.next_id()
        self.websocket = websocket
        self.label_id = None
        print('New client', self)

    def __str__(self):
        return str(self.id) + ':' + str(self.websocket.remote_address)

    @classmethod
    def next_id(cls):
        new_id = cls.__last_id
        cls.__last_id += 1
        return new_id

    def filter_torrents(self, torrents):
        if self.label_id:
            return [t for t in torrents if t.label_id == self.label_id]
        else:
            return torrents

    async def set_state(self, torrents):
        torrents = self.filter_torrents(torrents)
        await self.event('set_state', torrents)

    async def update_task(self, task):
        await self.event('update_task', task)

    async def event(self, event_type, params):
        msg = self.serialize({'event': event_type, 'params': params})
        await self.send(msg)

    @staticmethod
    def serialize(obj):
        return json.dumps(obj, default=json_serial)

    def send(self, msg):
        return self.websocket.send(msg)