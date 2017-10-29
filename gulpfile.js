var elixir = require('laravel-elixir');

/*
 |--------------------------------------------------------------------------
 | Elixir Asset Management
 |--------------------------------------------------------------------------
 |
 | Elixir provides a clean, fluent API for defining some basic Gulp tasks
 | for your Laravel application. By default, we are compiling the Sass
 | file for our application, as well as publishing vendor resources.
 |
 */

elixir(function(mix) {
    var bowerPath = 'vendor/bower_dl';
    var jqueryPath = bowerPath + '/jquery/dist';
    var bootstrapPath = 'node_modules/bootstrap-sass/assets';
    var jsonrpcPath = 'node_modules/jsonrpc-lite';
    mix.sass('app.scss')
        .copy(bootstrapPath + '/fonts', 'public/fonts')
        .copy(bootstrapPath + '/javascripts/bootstrap.min.js', 'public/js')
        .copy(jqueryPath + '/jquery.js', 'public/js')
        .copy(jsonrpcPath + '/jsonrpc.js', 'public/js')
        .copy(bowerPath + '/filesize/lib/filesize.js', 'public/js');
});
