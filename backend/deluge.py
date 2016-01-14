import requests
import json

class Deluge:
    def __init__(self, url, password):
        self.url = url
        self.password = password
        self.headers = {'content-type': 'application/json', 'Accept': 'application/json'}
        self.cookies = None

    def invoke(self, method, params=[]):
        is_auth = (method == "auth.login")
        payload = {
            "method": method,
            "params": params,
            "jsonrpc": "2.0",
            "id": 1,
        }
        response = requests.post(self.url, data=json.dumps(payload), headers=self.headers, cookies=self.cookies)
        data = response.json()
        if data["error"]:
            if data["error"]["code"] == 1 and not is_auth:
                self.auth()
                return self.invoke(method, params)
            else:
                raise Exception(data["error"]["message"])
        else:
            if is_auth:
                self.cookies = response.cookies
        return data["result"]

    def auth(self):
        return self.invoke("auth.login", [self.password])

    def torrents(self):
        return self.invoke("core.get_torrents_status", [{}, ["name", "save_path", "progress", "label", "time_added"]])

    def labels(self):
        return self.invoke("label.get_labels")
