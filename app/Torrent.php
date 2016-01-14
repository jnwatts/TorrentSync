<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Torrent extends Model
{
    public $parimaryKey = 'hash';
    protected $table = 'torrents';
    public $timestamps = false;
    public $incrementing = false;

    public function label()
    {
        return $this->belongsTo('App\Label');
    }

    public function task()
    {
        return $this->hasOne('App\Task', 'hash', 'hash');
    }
}
