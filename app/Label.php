<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Label extends Model
{
    protected $table = 'labels';
    public $timestamps = false;

    public static function emptyId()
    {
        return Label::unlabeled()->id;
    }

    public static function unlabeled()
    {
        $label = Label::where('label', 'unlabeled')->get()->first();
        if ($label == null) {
            $label = new Label;
            $label->label = 'unlabeled';
            $label->save();
        }
        return $label;
    }
}
