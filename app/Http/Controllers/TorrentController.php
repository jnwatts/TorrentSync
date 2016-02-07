<?php

namespace App\Http\Controllers;

use JavaScript;
use Log;
use Session;
use Config;

use App\Torrent;
use App\Label;
use App\Task;

use Illuminate\Http\Request;
use Illuminate\Http\Response;

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

        $torrents = Torrent::orderBy('time_added', 'desc')->where('label_id', $label->id)->get();
        
        $response_data = [
            'torrents' => $torrents,
            'labels' => $labels,
            'label' => $label,
        ];

        if (strstr($request->header('Accept'), 'application/json')) {
            // Return data as JSON object
            return response()->json($response_data);
        } else {
            $this->flushClientEvents();
            JavaScript::put($response_data);
            return view('index', ['label' => $label]);
        }
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
        $response = null;
        if ($task) {
            $busy[] = $task;
            $response = response('Task is busy: '.$hash, 400);
        } else {
            $task = new Task();
            $task->hash = $hash;
            $task->save();
            $created[] = $task;
            $response = back();
        }

        $this->signal();

        return back();
    }

    public function refresh()
    {
        return $this->transfer('refresh');
    }

    public function clear($hash = '')
    {
        $tasks = Task::where('state', 'complete');
        if (strlen($hash) > 0) {
            $tasks = $tasks->where('hash', $hash);
        }
        $tasks->delete();
        return $tasks->get();
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

    public function formatEvent($params)
    {
        $fields = array('id', 'event', 'data');
        $event = "";
        foreach ($fields as $f) {
            if (isset($params[$f]))
                $event .= $f . ':' . $params[$f]."\n";
        }
        $event .= "\n";
        return $event;
    }

    public function flushClientEvents($date = null)
    {
        Session::put('last_event', $date ? $date : date('Y-m-d H:i:s'));
        Session::save();
    }

    public function events(Request $request)
    {
        $last_event = date('Y-m-d');
        if (Session::has('last_event')) {
            $last_event = Session::get('last_event');
        }
    
        $response = "";

        $result = Task::where('updated_at', '>=', $last_event)->get();
        if ($result->count() > 0) {
            $response .= $this->formatEvent(array(
                'event' => 'task',
                'data' => $result->toJson(),
            ));
        }

        $result = Torrent::where('updated_at', '>=', $last_event)->get();
        if ($result->count() > 0) {
            $response .= $this->formatEvent(array(
                'event' => 'torrent',
                'data' => $result->toJson(),
            ));
        }

        $this->flushClientEvents();
        return (new Response($response, 200))->header('Content-Type', 'text/event-stream');
    }

    private function signal()
    {
        $client = new \JsonRPC\Client(Config::get('app.backend_url')); //'http://localhost:4000');
        $result = $client->signal();
    }
}
