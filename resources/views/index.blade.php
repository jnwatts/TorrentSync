@extends('layouts.app')

@section('title', 'TorrentSync - '.$label->label)

@section('content')

<script type='text/javascript'>

function glyph(type) {
    var g = $('<span />');
    g.addClass('glyphicon');
    if (type) {
        g.addClass('glyphicon-' + type);
    }
    g.attr('aria-hidden', 'true');
    return g;
}

function a_button(type) {
    var b = $('<a />');
    b.addClass('btn');
    if (type) {
        b.addClass('btn-' + type);
    }
    b.attr('role', 'button');
    return b;
}

function initState() {
    setLabel(label);
    var state = labelState(label);
    window.history.replaceState(state, state.title, state.href);
    window.onpopstate = function (data) {
        setLabel(data.state.label);
    };
}

function labelState(l) {
    var state = {
        title: 'TorrentSync - ' + l.label,
        href: laroute.route('torrents.index', (l.id == 1 ? undefined : {label: l.id}) ),
        label: l,
    };

    return state;
}

function jumpLabel(l) {
    var state = labelState(l);
    window.history.pushState(state, state.title, state.href);
    setLabel(l);
}

function setLabel(l) {
    if (label != l) {
        var state = labelState(l);
        document.title = state.title;
        label = l;
        refreshIndex();
    }
}

function refreshIndex() {
    var state = labelState(label);
    $.ajax(state.href, {
        accepts: {json: 'application/json'},
        dataType: 'json',
    }).done(function (data) {
        for (k in data) {
            window[k] = data[k];
        };
        clearLabels();
        clearTorrents();
        refreshLabels();
        refreshTorrents();
    });
}

function clearLabels() {
    var list = $('#labels-list');
    var item = list.children('.label-item');
    item.remove();
}

function clearTorrents() {
    var tbody = $("tbody#torrents");
	var tr = tbody.children('tr');
    tr.remove();
}

function refreshLabels() {
    var current = $('#labels-current');
    current.text(label.label);
    for (l in labels) {
        populateLabel(labels[l]);
    };
}
function refreshTorrents() {
    for (t in torrents) {
        populateTorrent(torrents[t]);
    };
}

function populateLabel(l) {
    var list = $('#labels-list');
    var item = list.children('.label-item#label-' + l.id);
    if (item.length <= 0) {
        item = $('<li />');
        item.attr('id', 'label-' + l.id);
        item.addClass('label-item');
        list.append(item);
    }
    var a = $('<a />')
    var state = labelState(l);
    a.attr('href', state.href);
    a.text(l.label);
    a.click(function (e) {
        jumpLabel(l);
        $('#labels.open').find('button[data-toggle=dropdown]').dropdown('toggle');
        return false;
    });
    item.html(a);
}

function refreshAction(e) {
    $('#refresh').children('.glyphicon').addClass('spinning');
	return torrentAction(e);
}

function torrentAction(e) {
    $.ajax(e.currentTarget.href, {
            accepts: {json: 'application/json'},
            dataType: 'json'
    }).fail(function (data) {
        if (e.currentTarget.href.indexOf('refresh') > -1) {
            $('#refresh').children('.glyphicon').removeClass('spinning');
        }
		console.log(data.responseText);
    });
    return false;
}

function hashToIndex(hash) {
    return torrents.findIndex(function (t) {
        return (t.hash == hash);
    });
}

function populateTorrent(t) {
    var tbody = $("tbody#torrents");
	var tr = $('tr#'+t.hash);
    if (tr.length <= 0) {
        tr = $('<tr />');
        tr.attr('id', t.hash);
        tbody.append(tr);
    }

    tr.empty();

    var addField = function(name, value) {
        var td = $('<td />');
        td.addClass('task-'+name);
        td.addClass('table-text');
        td.text(value);
        tr.append(td);
		return td;
    };

    addField('name', t.name);
    addField('progress', t.progress + "% / " + parseInt(t.local_size / t.total_wanted * 100.0) + "%");
    addField('progress', filesize(t.total_wanted));
    addField('time_added', t.time_added);
    addField('label', t.label.label);
    actions = addField('actions', '');
    actions.empty();
	if (t.task) {
        if (t.task.state == 'complete') {
            actions.append(glyph('saved'));
            var a = a_button('sm');
            a.attr('href', laroute.route('torrent.clear', {hash: t.hash}));
            a.click(torrentAction);
            a.click(function() {
                t.task = null;
                populateTorrent(t);
            });
            a.append(glyph('erase'));
            actions.append(a);
        } else {
            var a = a_button('sm');
            a.attr('href', laroute.route('torrent.stop', {hash: t.hash}));
            a.click(torrentAction);
            a.append(glyph('transfer'));
            actions.append(a);
        }
	} else {
        var a = a_button('sm');
        a.attr('href', laroute.route('torrent.transfer', {hash: t.hash}));
        a.click(torrentAction);
        a.append(glyph('save'));
        actions.append(a);
    }
}

function initEvents() {
    var s = new EventSource('{{ URL::route('torrents.events') }}');
    s.addEventListener('task', function(event) {
        tasks = JSON.parse(event.data);
        if (tasks.length > 0) {
			for (t in tasks) {
                var task = tasks[t];
                if (task.hash == 'refresh') {
                    if (task.state == 'syncing') {
                        $('#refresh').children('.glyphicon').addClass('spinning');
                    } else if (task.state == 'complete') {
                        $('#refresh').children('.glyphicon').removeClass('spinning');
                    }
                } else {
                    var t = hashToIndex(task.hash);
                    if (t >= 0) {
                        torrents[t].task = task;
                        populateTorrent(torrents[t]);
                    }
                }
            }
        }
    });

    s.addEventListener('torrent', function(event) {
        var torrents = JSON.parse(event.data);
        if (torrents.length > 0) {
			for (t in torrents) {
                var torrent = torrents[t];
                if (torrent.label.id == label.id) {
                    populateTorrent(torrent);
                }
            }
        }
    });

    s.addEventListener('debug', function (event) {
        console.log('debug:')
        console.log(event.data);
    });
}

$(function () {
    refreshLabels();
    refreshTorrents();

    initEvents();

    initState();

	$('#refresh').click(refreshAction);
});

</script>

<div class="panel panel-default">
    <div class="panel-heading">
        Torrents
        <a class="btn" role="button" id="refresh" href="{{ URL::route('torrents.refresh') }}">
            <span class="glyphicon glyphicon-refresh"></span>
        </a>
        <div class="dropdown" style="display:inline" id="labels">
            <button class="btn dropdown-toggle" type="button" data-toggle="dropdown">
                <span id="labels-current"></span>
                <span class="caret"></span>
            </button>
            <ul class="dropdown-menu" id="labels-list">
            </ul>
        </div>
    </div>

    <div class="panel-body">
        <table class="table table-striped task-table">
            <!-- Table Headings -->
            <thead>
                <th>Name</th>
                <th>Progress (R/L)</th>
                <th>Size</th>
                <th>Time Added</th>
                <th>Label</th>
                <th>&nbsp;</th>
            </thead>

            <!-- Table Body -->
            <tbody id="torrents">
            </tbody>
        </table>
    </div>
</div>

<div class="panel panel-default">
    <div class="panel-heading">
        TODO
    </div>
    <div class="panel-body">
        <ul>
            <li>UI
                <ul>
                    <li>Show current state: size of local vs expected size</li>
                    <li>Table sorting</li>
                    <li>Save sort order in cookies</li>
                    <li>Show active queue?</li>
                </ul>
            </li>
        </ul>
    </div>
</div>

@endsection

