<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Task extends Model
{
    protected $table = "tasks";
    protected $attributes = array(
        'hash' => '',
        'command' => 'start',
        'state' => 'init',
        'pid' => 0,
    );
}
