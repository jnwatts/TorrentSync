<?php

use Illuminate\Database\Schema\Blueprint;
use Illuminate\Database\Migrations\Migration;

class CreateTasksTable extends Migration
{
    /**
     * Run the migrations.
     *
     * @return void
     */
    public function up()
    {
        $states = ['init', 'syncing', 'complete', 'killed'];
        Schema::create('tasks', function (Blueprint $table) {
            $table->increments('id');
            $table->string('hash', 40);
            $table->enum('command', ['start', 'idle', 'stop']);
            $table->enum('state', ['init', 'syncing', 'complete']);
            $table->integer('pid');
            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     *
     * @return void
     */
    public function down()
    {
        Schema::drop('tasks');
    }
}
