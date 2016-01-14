<?php

namespace App\Http\Controllers;

use App\Torrent;
use App\Label;
use App\Task;

use Illuminate\Http\Request;

use App\Http\Requests;
use App\Http\Controllers\Controller;

class TorrentController extends Controller
{
    public function __construct()
    {
        $this->middleware('web');
    }

    public function index(Request $request)
    {
        $label = null;
        $label_id = $request->input('label', null);
        if ($label_id != null) {
            $label = Label::find((int)($label_id));
        }
        if ($label == null) {
            $label = Label::unlabeled();
        }

        $_labels = Label::orderBy('id', 'asc')->get();
        $labels = array();
        foreach ($_labels as $l) {
            $labels[$l->id] = $l;
        }

        return view('index', [
                'torrents' => Torrent::orderBy('time_added', 'desc')->where('label_id', $label->id)->get(),
                'labels' => $labels,
                'label' => $label,
        ]);
    }

    public function transfer($hash)
    {
        $deleted = array();
        $busy = array();
        $created = array();

        $old_tasks = Task::where('hash', $hash)->where('state', 'complete')->get();
        foreach ($old_tasks as $task) {
            $deleted[] = $task;
            $task->delete();
        }
        $task = Task::where('hash', $hash)->where('state', '!=', 'complete')->get()->first();
        if ($task) {
            $busy[] = $task;
        } else {
            $task = new Task();
            $task->hash = $hash;
            $task->save();
            $created[] = $task;
        }

        $this->signal();

        return back();
    }

    public function refresh()
    {
        return $this->transfer('');
    }

    public function clear($hash = '')
    {
        $tasks = Task::where('state', 'complete');
        if (strlen($hash) > 0) {
            $tasks = $tasks->where('hash', $hash);
        }
        $tasks->delete();
        return back();
    }

    public function stop($hash)
    {
        $task = Task::where('hash', $hash)->where('state', 'syncing')->get()->first();
        if ($task) {
            $task->command = 'stop';
        }
        $this->signal();
        return back();
    }

    private function signal()
    {
        $client = new \JsonRPC\Client('http://localhost:4000');
        $result = $client->signal();
    }
}
