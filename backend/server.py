import traceback
import json
import asyncio
import websockets
from jsonrpcserver.aio import methods
from jsonrpcserver.response import NotificationResponse
from .client import Client

from jsonrpcserver import config
config.debug = True

class Server():
    def __init__(self, backend):
        self.clients = []
        Server.backend = backend

    def add_client(self, websocket):
        client = Client(websocket)
        self.clients.append(client);
        return client

    def remove_client(self, client):
        if client in self.clients:
            self.clients.remove(client)

    async def set_state(self, torrents):
        await asyncio.wait([client.set_state(torrents) for client in self.clients])

    async def update_task(self, torrents):
        await asyncio.wait([client.update_task(torrents) for client in self.clients])

    @methods.add
    async def index(label_id = None, context = None):
        Server.backend.log('%s: %s(%s)' % (context, 'index', label_id))
        client = context
        if not label_id:
            # Convert 0 to None
            label_id = None
        client.label_id = label_id
        return client.serialize(Server.backend.torrentsyncdb.get_torrents_by_label(label_id))

    @methods.add
    async def labels(context = None):
        Server.backend.log('%s: %s()' % (context, 'labels'))
        return json.dumps(Server.backend.torrentsyncdb.get_labels())

    @methods.add
    async def transfer(hash, context = None):
        Server.backend.log('%s: %s(%s)' % (context, 'transfer', hash))
        await Server.backend.get_torrent(hash);

    @methods.add
    async def refresh(context = None):
        Server.backend.log('%s: %s()' % (context, 'refresh'))
        asyncio.get_event_loop().create_task(Server.backend.refresh_torrents())

    async def handle_request(self, client, path, request):
        try:
            # self.backend.log("%s: %s" % (client, request));
            response = await methods.dispatch(request, client)
            if not response.is_notification:
                await client.send(str(response))
        except Exception as e:
            traceback.print_exc()

    async def main(self, websocket, path):
        client = self.add_client(websocket);
        while True:
            try:
                request = await asyncio.wait_for(websocket.recv(), timeout=20)
            except asyncio.TimeoutError:
                # No data in 20 seconds, check the connection.
                try:
                    await asyncio.wait_for(websocket.ping(), timeout=10)
                except asyncio.TimeoutError:
                    # No response to ping in 10 seconds, disconnect.
                    break
            else:
                asyncio.get_event_loop().create_task(self.handle_request(client, path, request))
        self.remove_client(client);

    async def run(self, host, port):
        await websockets.serve(self.main, host, port)