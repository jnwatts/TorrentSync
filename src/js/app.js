"use strict";

(function (global) {
    window.label = null;
    window.labels = [];
    window.torrents = [];

    var Task = {
        INIT: 'init',
        SYNCING: 'syncing',
        COMPLETE: 'complete',
    };

    window.glyph = (type) => {
        var g = $('<span />');
        g.addClass('glyphicon');
        if (type) {
            g.addClass('glyphicon-' + type);
        }
        g.attr('aria-hidden', 'true');
        return g;
    }

    window.a_button = (type) => {
        var b = $('<a />');
        b.addClass('btn');
        if (type) {
            b.addClass('btn-' + type);
        }
        b.attr('role', 'button');
        return b;
    }

    window.initState = () => {
        var label;
        var url = new URL(window.location);
        if (url.searchParams.has('label')) {
            label = idToLabel(url.searchParams.get('label'));
        } else {
            label = window.labels[0];
        }
        setLabel(label);
        var state = label.state;
        window.history.replaceState(state, state.title, state.href.replace(/#.*/,''));
        window.onpopstate = (data) => {
            setLabel(data.state.label);
        };
    }

    window.jumpLabel = (l) => {
        var state = l.state;
        window.history.pushState(state, state.title, state.href);
        setLabel(l);
    }

    window.setLabel = (l) => {
        if (window.label != l) {
            window.label = l;
            $('#labels-current').text(l.label);
            document.title = l.state.title;
            torrentsync.getTorrents(l.id).then((torrents) => {
                window.torrents = torrents;
                refreshTorrents();
            });
        }
    }

    window.refreshLabels = () => {
        window.labels.forEach((l) => {
            l.route = torrentsync.route('index', [l.id]);
            l.state = {
                title: 'TorrentSync - ' + l.label,
                href: l.route,
                label: l,
            };
            populateLabel(l);
        });
    }

    window.refreshTorrents = () => {
        window.torrents.sort((a, b) => {
            a = a.time_added;
            b = b.time_added;
            return (a < b ? 1 : (a > b ? -1 : 0));
        });
        $("tbody#torrents").empty();
        window.torrents.forEach(populateTorrent);
    }

    window.updateTasks = (tasks) => {
        tasks.forEach((task) => {
            var i = hashToIndex(task.hash);
            if (i >= 0) {
                torrents[i].task = task;
                populateTorrent(torrents[i]);
            }
        });
    }

    window.populateLabel = (l) => {
        var list = $('#labels-list');
        var item = list.children('.label-item#label-' + l.id);
        if (item.length <= 0) {
            item = $('<li />');
            item.attr('id', 'label-' + l.id);
            item.addClass('label-item');
            list.append(item);
        }
        var a = $('<a />')
        a.attr('href', l.state.href);
        a.text(l.label);
        a.click((e) => {
            jumpLabel(l);
            $('#labels.open').find('button[data-toggle=dropdown]').dropdown('toggle');
            return false;
        });
        item.html(a);
    }

    window.idToLabel = (id) => {
        return window.labels.find((l) => {
            return (l.id == id);
        });
    }

    window.hashToIndex = (hash) => {
        return torrents.findIndex((t) => {
            return (t.hash == hash);
        });
    }

    window.populateTorrent = (t) => {
        var tbody = $("tbody#torrents");
        var tr = $('tr#'+t.hash);
        if (tr.length <= 0) {
            tr = $('<tr />');
            tr.attr('id', t.hash);
            tbody.append(tr);
        }

        tr.empty();

        var addField = (name, value) => {
            var td = $('<td />');
            td.addClass('task-'+name);
            td.addClass('table-text');
            td.text(value);
            tr.append(td);
            return td;
        };

        addField('name', t.name);
        addField('progress', t.progress + "% / " + (t.task.progress ? t.task.progress : 0) + "%");
        addField('size', filesize(t.total_wanted));
        addField('added', t.time_added);
        addField('label', t.label ? t.label.label : 'INVALID');
        var actions = addField('actions', '');
        actions.empty();
        if (t.task.state) {
            if (t.task.state == Task.COMPLETE) {
                actions.append(glyph('saved'));
                var a = a_button('sm');
                a.attr('href', torrentsync.route('torrent.clear', [t.hash]));
                a.click(torrentsync.action);
                a.click(() => {
                    t.task = null;
                    populateTorrent(t);
                });
                a.append(glyph('erase'));
                actions.append(a);
            } else {
                var a = a_button('sm');
                a.attr('href', torrentsync.route('torrent.stop', [t.hash]));
                a.click(torrentsync.action);
                a.append(glyph('transfer'));
                actions.append(a);
            }
        } else {
            var a = a_button('sm');
            a.attr('href', torrentsync.route('torrent.transfer', [t.hash]));
            a.click(torrentsync.action);
            a.append(glyph('save'));
            actions.append(a);
        }
    }

    window.torrentsync = {
        last_id: 0,
        socket: null,
        response: $.Callbacks('unique stopOnFalse'),
        event: $.Callbacks('unique stopOnFalse'),
        messageHandler: (e) => {
            if (typeof e.data == 'string') {
                var msg = jsonrpc.parse(e.data);
                if (msg.type == 'success' || msg.type == 'error') {
                    torrentsync.response.fire(msg);
                } else if (msg.type == 'invalid') {
                    msg = msg.payload.data;
                    if (typeof msg.event == 'string') {
                        torrentsync.event.fire(msg);
                    }
                }
            }
        },
        open: () => {
            return new Promise((resolve, reject) => {
                var socket = torrentsync.socket;
                if (socket) {
                    socket.close();
                }
                var url = new URL(window.location);
                if (!url.pathname.endsWith('/'))
                    url.pathname += '/';
                socket = new WebSocket('wss://' + url.hostname + url.pathname + 'ws/');
                socket.binaryType = 'arraybuffer';
                socket.onmessage = torrentsync.messageHandler;
                socket.onopen = () => {
                    resolve();
                };
                socket.onclose = () => {
                    torrentsync.socket = null;
                };
                torrentsync.socket = socket;
            });
        },
        request: (method, params = []) => {
            return new Promise((resolve, reject) => {
                var s = torrentsync.socket;
                if (!s) {
                    reject(new Error('Not connected'));
                } else {
                    var id = torrentsync.last_id + 1;
                    torrentsync.last_id = id;
                    var response_handler = (msg) => {
                        if (msg.payload.id != id) {
                            // Not our response, move along
                            return;
                        }
                        if (msg.type == 'success') {
                            resolve(JSON.parse(msg.payload.result));
                        } else {
                            reject(msg.payload.error);
                        }
                        torrentsync.response.remove(response_handler);
                    };
                    torrentsync.response.add(response_handler);
                    s.send(jsonrpc.request(id, method, params));
                }
            });
        },
        getLabels: () => {
            return torrentsync.request('labels');
        },
        getTorrents: (label_id = null) => {
            return torrentsync.request('index', [label_id]);
        },
        route: (target, params) => {
            var href = '';
            if (target == 'index') {
                if (params) {
                    href += '?label=' + params[0];
                }
            } else {
                var href = '#' + target;
                if (params) {
                    href += '?';
                    href += Object.keys(params).map((k,i) => {
                        return 'arg' + k + '=' + params[k];
                    }).join('&');
                }
            }
            return href;
        },
        action: (e) => {
            var hash = e.currentTarget.href;
            var request = {
                target: null,
                method: null,
                params: null,
            };
            if (!hash)
                return;
            if (hash.indexOf('#') < 0)
                return;
            if (hash.indexOf('.') < 0)
                return;
            hash = hash.substr(hash.indexOf('#') + 1);
            if (hash.indexOf('?') > -1) {
                request.params = hash.substr(hash.indexOf('?') + 1).split('&').map((v) => {
                    var p = v.split('=');
                    if (p[1])
                        return p[1];
                    return null;
                });
                hash = hash.substr(0, hash.indexOf('?'));
            } else {
                request.params = [];
            }
            [request.target, request.method] = hash.split('.');
            if (request.target == 'torrent') {
                torrentsync.request(request.method, request.params);
            } else {
                console.error('Invalid target', request);
            }
            return false;
        },
        refresh: () => {
            return torrentsync.request('refresh');
        }
    };

    $(() => {
        torrentsync.event.add((e) => {
            if (e.event == 'set_state') {
                $('#refresh').children('.glyphicon').removeClass('spinning');
                window.torrents = e.params;
                refreshTorrents();
            } else if (e.event == 'update_task') {
                updateTasks(e.params);
            } else {
                console.warn('Unhandled event:', e);
            }
        });
        torrentsync.open().then(() => {
            torrentsync.getLabels().then((labels) => {
                window.labels = labels;
                refreshLabels();
                initState();
            });
        });

        $('#refresh').click((e) => {
            $('#refresh').children('.glyphicon').addClass('spinning');
            return torrentsync.action(e);
        });
    });
})(typeof window !== "undefined" ? window : global);