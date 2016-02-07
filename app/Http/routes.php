<?php

use App\Torrent;
use Illuminate\Http\Request;

/*
|--------------------------------------------------------------------------
| Routes File
|--------------------------------------------------------------------------
|
| Here is where you will register all of the routes in an application.
| It's a breeze. Simply tell Laravel the URIs it should respond to
| and give it the controller to call when that URI is requested.
|
*/
Route::get('/',                 ['as'=>'torrents.index',     'uses'=>'TorrentController@index']);
Route::get('/events',           ['as'=>'torrents.events',    'uses'=>'TorrentController@events']);
Route::get('/refresh',          ['as'=>'torrents.refresh',   'uses'=>'TorrentController@refresh']);
Route::get('/clear',            ['as'=>'torrents.clear',     'uses'=>'TorrentController@clear']);
Route::get('/{hash}/transfer',  ['as'=>'torrent.transfer',   'uses'=>'TorrentController@transfer']);
Route::get('/{hash}/stop',      ['as'=>'torrent.stop',       'uses'=>'TorrentController@stop']);
Route::get('/{hash}/clear',     ['as'=>'torrent.clear',      'uses'=>'TorrentController@clear']);
