<?php

namespace App;

use Log;
use Illuminate\Database\Eloquent\Model;

class Torrent extends Model
{
    public $parimaryKey = 'hash';
    protected $table = 'torrents';
    public $timestamps = false;
    public $incrementing = false;
    protected $appends = ['task', 'label'];
    protected $dates = ['created_at', 'updated_at', 'deleted_at'];

    public function getLabelAttribute()
    {
        return $this->belongsTo('App\Label', 'label_id', 'id')->first();
    }

    public function getTaskAttribute()
    {
        return $this->hasOne('App\Task', 'hash', 'hash')->first();
    }
}
