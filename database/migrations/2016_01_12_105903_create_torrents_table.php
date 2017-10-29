<?php

use Illuminate\Database\Schema\Blueprint;
use Illuminate\Database\Migrations\Migration;

class CreateTorrentsTable extends Migration
{
    /**
     * Run the migrations.
     *
     * @return void
     */
    public function up()
    {
        Schema::create('torrents', function (Blueprint $table) {
            $table->string('hash', 40);
            $table->text('name');
            $table->text('save_path');
            $table->integer('progress');
            $table->integer('total_wanted');
            $table->integer('label_id')->unsigned();
            $table->date('time_added');
            $table->timestamps();

            $table->primary('hash');
            $table->index('time_added');

            $table->foreign('label_id')->references('id')->on('labels')->onDelete('cascade');
        });
    }

    /**
     * Reverse the migrations.
     *
     * @return void
     */
    public function down()
    {
        Schema::drop('torrents');
    }
}
