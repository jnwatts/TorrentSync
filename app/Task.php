<?php

namespace App;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\SoftDeletes;

class Task extends Model
{
    use SoftDeletes;

    protected $table = "tasks";
    protected $attributes = array(
        'hash' => '',
        'command' => 'start',
        'state' => 'init',
        'pid' => 0,
    );

    protected $dates = ['created_at', 'updated_at', 'deleted_at'];
}
