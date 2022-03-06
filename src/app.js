"use strict";

import $ from 'jquery';
import jsonrpc from 'npm:jsonrpc-lite/jsonrpc.js';
import moment from 'npm:moment/moment.js';
import filesize from './filesize';
import "@fortawesome/fontawesome-free/css/all.css";

(function (global) {
    window.label = null;
    window.labels = [];
    window.torrents = new Map();
    window.tasks = new Map();

    var Task = {
        INIT: 'INIT',
        QUEUED: 'QUEUED',
        RUNNING: 'RUNNING',
        COMPLETE: 'COMPLETE',
        FAILED: 'FAILED',
    };

    window.glyph = (type) => {
        var g = $('<span />');
        g.addClass('fa');
        if (type) {
            g.addClass('fa-' + type);
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

    window.initState = (label) => {
        window.label = findLabel(label);
        setLabel(window.label);

        var state = window.label.state;
        window.history.replaceState(state, state.title, state.href.replace(/#.*/,''));
        window.onpopstate = (data) => {
            if (data.state)
                setLabel(data.state.label);
            else
                console.warn("State is null??");
        };
    }

    window.jumpLabel = (l) => {
        var state = l.state;
        window.history.pushState(state, state.title, state.href);
        setLabel(l);
    }

    window.setLabel = (l) => {
        document.title = l.state.title;
        $('#labels-current').text(l.label);
        if (window.label != l) {
            window.label = l;
            torrentsync.getState(l.label);
        }
    }

    window.refreshLabels = (labels) => {
        window.labels = labels.map((label) => {
            var l = {};
            l.label = label ? label : "unlabeled";
            l.route = torrentsync.route('core.get_state', [l.label]);
            l.state = {
                title: 'TorrentSync - ' + l.label,
                href: l.route,
                label: l,
            };
            populateLabel(l);
            return l;
        });
    }

    window.updateTorrents = (torrents) => {
        window.torrents = new Map([...torrents.entries()].sort((a, b) => {
            a = a[1];
            b = b[1];
            if (a.time_added == b.time_added) {
                a = a.name;
                b = b.name;
            } else {
                a = a.time_added;
                b = b.time_added;
            }
            return (a < b ? 1 : (a > b ? -1 : 0));
        }));
        $("tbody#torrents").empty();
        window.torrents.forEach((torrent, hash) => {
            torrent.hash = hash;
            if (!torrent.task)
                torrent.task = window.tasks.get(hash);
            populateTorrent(torrent);
        });
    }

    window.updateTasks = (tasks) => {
        window.tasks = tasks;
        window.tasks.forEach((task, hash) => {
            var torrent = window.torrents.get(hash);
            if (torrent) {
                torrent.task = task;
                populateTorrent(torrent);
            }
        });
    }

    window.updateCore = (params) => {
        if ('refreshing' in params) {
            if (params.refreshing) {
                $('#refresh').children('.fa').addClass('spinning');
            } else {
                $('#refresh').children('.fa').removeClass('spinning');
            }
        }
    }

    window.populateLabel = (l) => {
        var list = $('#labels-list');
        var item = list.children('.dropdown-item#label-' + l.label);
        if (item.length <= 0) {
            item = $('<a />');
            item.attr('id', 'label-' + l.label);
            item.addClass('dropdown-item');
            item.attr('href', l.state.href);
            item.text(l.label);
            item.click((e) => {
                jumpLabel(l);
                $('#labels.open').find('button[data-toggle=dropdown]').dropdown('toggle');
                return false;
            });
            list.append(item);
        }
    }
    window.findLabel = (label) => {
        var l = window.labels.find((l) => {
            return (l.label == label);
        });
        if (l == undefined)
            return window.labels[0];
        else
            return l;
    }
    window.idToLabel = (id) => {
        return window.labels.find((l) => {
            return (l.id == id);
        });
    }

    window.idToTask = (id) => {
        return window.tasks.find((t) => {
            return (t.id == id);
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

        var vertProgress = (value) => {
            var outer = $('<div class="progress progress-bar-vertical">');
            var inner = $('<div class="progress-bar" role="progressbar" aria-valuenow="' + value + '" aria-valuemin="0" aria-valuemax="100" style="height: ' + value + '%;">');
            if (value < 100) {
                inner.addClass('active');
            }
            inner.append($('<span class="sr-only">' + value + '% Complete</span>'));
            outer.append(inner);
            return outer;
        };

        var addProgress = (remote_progress, local_progress) => {
            var td = $('<td />');
            td.addClass('task-progress');
            td.append(vertProgress(remote_progress));
            td.append(vertProgress(local_progress));
            tr.append(td);
        };

        var addAction = (name, icon, action) => {
            var e;
            if (action) {
                e = $('<a />');
                e.attr('href', torrentsync.route('torrent.' + action, [t.hash]));
                e.click(torrentsync.action);
            } else {
                e = $('<span />');
            }
            if (name) {
                e.attr('title', name);
            }
            e.addClass('w-50');
            e.addClass('text-center');
            e.addClass('badge');
            if (icon) {
                e.append(glyph(icon));
            }
            actions.append(e);
        };

        var task = t.task;
        var remote_progress = Math.floor(t.progress);
        var local_progress = 0;
        if (task) {
            if (task.state == Task.COMPLETE)
                local_progress = 100;
            else
                local_progress = Math.floor(task.progress);
        }

        // console.log(t);

        addField('name', t.name.replace(/\./g, ".\u200B"));
        addProgress(remote_progress, local_progress);
        addField('size', filesize(t.total_wanted));
        addField('added', moment.unix(t.time_added).format('YYYY\u2011MM\u2011DD HH:mm:ss'));
        // addField('label', t.label ? t.label : "unlabeled");
        var actions = addField('actions', '');
        actions.attr('class', 'badge-group');
        actions.attr('role', 'group');
        actions.empty();

        if (task) {
            if (task.state == Task.COMPLETE) {
                addAction('Complete', 'check');
                addAction('Retransfer', 'arrow-rotate-right', 'transfer');
            } else if (task.state == Task.QUEUED) {
                addAction('Queued', 'stopwatch');
                addAction('Abort', 'ban', 'abort');
            } else if (task.state == Task.RUNNING) {
                addAction('Transferring', 'arrow-right-arrow-left');
                addAction('Abort', 'ban', 'abort');
            } else if (task.state == Task.FAILED) {
                addAction('Failed', 'exclamation');
                addAction('Retransfer', 'arrow-rotate-right', 'transfer');
            } else if (remote_progress >= 100) {
                addAction('Transfer', 'download', 'transfer');
            }
        } else if (remote_progress >= 100) {
            addAction('Transfer', 'download', 'transfer');
        }
    }

    window.torrentsync = {
        retries: 0,
        max_retries: 60,
        last_id: 0,
        socket: null,
        response: $.Callbacks('unique stopOnFalse'),
        event: $.Callbacks('unique stopOnFalse'),
        messageHandler: (e) => {
            if (typeof e.data == 'string') {
                var msg = jsonrpc.parse(e.data);
                if (msg.type == 'success' || msg.type == 'error') {
                    torrentsync.response.fire(msg);
                } else if (msg.type == 'notification') {
                    var notification = msg.payload;
                    torrentsync.event.fire(notification);
                }
            }
        },
        open: () => {
            if (process.env.NODE_ENV !== 'production') {
                return new Promise((resolve, reject) => {
                    resolve();
                });
            }
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
                    torrentsync.retries = 0;
                    resolve();
                };
                socket.onclose = () => {
                    torrentsync.socket = null;
                };
                socket.onerror = (e) => {
                    if (torrentsync.retries++ < torrentsync.max_retries)
                        setTimeout(torrentsync.open, 1000);
                    reject(e);
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
                            if (typeof msg.payload.result == "string")
                                resolve(JSON.parse(msg.payload.result));
                            else
                                resolve(msg.payload.result);
                        } else {
                            reject(msg.payload.error);
                        }
                        torrentsync.response.remove(response_handler);
                    };
                    torrentsync.response.add(response_handler);
                    s.send(jsonrpc.request(id, method, params));
                }
            }).catch((e) => {
                if (e.message == "Not connected") {
                    torrentsync.open().then(() =>  {
                        console.log("Re-opened");
                    });
                }
            });
        },
        getState: (l = null) => {
            if (l == null) {
                var url = new URL(window.location);
                if (url.searchParams.has('label'))
                    l = url.searchParams.get('label');
                else
                    l = "";
            }
            torrentsync.request('core.get_state', [l]).then((state) => {
                if (state !== undefined) {
                    refreshLabels(state.labels);
                    initState(state.label);
                }
            });
        },
        getLabels: () => {
            return torrentsync.request('labels');
        },
        getTorrents: (label_id = null) => {
            return torrentsync.request('index', [label_id]);
        },
        route: (method, params = null) => {
            var href = '';
            if (method == 'core.get_state') {
                if (params != null) {
                    href += '?label=' + params[0];
                }
            } else {
                var href = '#' + method;
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
                method: null,
                params: null,
            };
            if (!hash)
                return;
            if (hash.indexOf('#') < 0)
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
            request.method = hash;
            torrentsync.request(request.method, request.params);
            return false;
        },
        refresh: () => {
            return torrentsync.request('core.refresh');
        }
    };

    $(() => {
        if (process.env.NODE_ENV !== 'production') {
            refreshLabels(["test1", "test2"]);
            initState("test1");
            m = new Map();
            m.set(
                "1",
                {"label":"unlabeled","name":"Test COMPLETE","progress":100,"save_path":"x","time_added":1639102220,"total_wanted":100,"hash":"1",
                    "task": {"progress":0,"state":"COMPLETE","type":"transfer"}
                }
            );
            m.set(
                "2",
                {"label":"unlabeled","name":"Test QUEUED","progress":0,"save_path":"x","time_added":1639102220,"total_wanted":100,"hash":"2",
                    "task": {"progress":0,"state":"QUEUED","type":"transfer"}
                }
            );
            m.set(
                "3",
                {"label":"unlabeled","name":"Test RUNNING","progress":100,"save_path":"x","time_added":1639102220,"total_wanted":100,"hash":"3",
                    "task": {"progress":50,"state":"RUNNING","type":"transfer"}
                }
            );
            m.set(
                "4",
                {"label":"unlabeled","name":"Test FAILED","progress":100,"save_path":"x","time_added":1639102220,"total_wanted":100,"hash":"4",
                    "task": {"progress":0,"state":"FAILED","type":"transfer"}
                }
            );
            m.set(
                "5",
                {"label":"unlabeled","name":"Test INIT","progress":100,"save_path":"x","time_added":1639102220,"total_wanted":100,"hash":"5",
                    "task": {"progress":0,"state":"INIT","type":"transfer"}
                }
            );
            m.set(
                "6",
                {"label":"unlabeled","name":"Test NULL","progress":50,"save_path":"x","time_added":1639102220,"total_wanted":100,"hash":"6",
                    "task": null
                }
            );
            m.set(
                "7",
                {"label":"unlabeled","name":"Test NULL (complete)","progress":100,"save_path":"x","time_added":1639102220,"total_wanted":100,"hash":"7",
                    "task": null
                }
            );
            updateTorrents(m);
            return;
        }

        torrentsync.event.add((e) => {
            if (e.method == 'labels.update') {
                refreshLabels(e.params);
                setLabel(findLabel(window.label.label));
            } else if (e.method == 'torrents.update') {
                updateTorrents(new Map(Object.entries(e.params)));
            } else if (e.method == 'tasks.update') {
                updateTasks(new Map(Object.entries(e.params)));
            } else if (e.method == 'core.update') {
                updateCore(e.params);
            } else if (e.method == 'core.message') {
                //TODO: Banner critical & warning messages
                console.log(e.params);
            } else if (e.method == 'debug.queue') {
                $('#debug_queue').text(JSON.stringify(e.params,null,'  '));
            } else {
                console.warn('Unhandled event:', e);
            }
        });
        torrentsync.open().then(() => {
            torrentsync.getState();
        });

        $('#refresh').click((e) => {
            return torrentsync.action(e);
        });
    });
})(typeof window !== "undefined" ? window : global);