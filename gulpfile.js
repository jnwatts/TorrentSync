const { task, series, watch, src, dest } = require('gulp');
const terser = require('gulp-terser');
// var gulp = require('gulp');
// var terser = require('gulp-terser');

task('html', function() {
	return src('src/html/index.html')
		.pipe(dest('public'));
});

task('js', function() {
	return src([
			'src/js/*.js',
			'node_modules/jquery/dist/jquery.js',
			'node_modules/bootstrap/dist/js/bootstrap.js',
			'node_modules/jsonrpc-lite/jsonrpc.js',
			'node_modules/url-parse/dist/url-parse.js',
			'node_modules/moment/moment.js',
			])
		.pipe(terser())
		.pipe(dest('public/js'));
});

task('css', function() {
	return src([
			'src/css/*.css',
			'node_modules/bootstrap/dist/css/bootstrap.css'
			])
		.pipe(dest('public/css'));
});

task('fonts', function() {
	return src([
			'node_modules/bootstrap/dist/fonts/*'
			])
		.pipe(dest('public/fonts'));
});

task('default', series('html', 'js', 'css', 'fonts'));

task('watch', function() {
	watch(['src/html/*'], task('html'));
	watch(['src/js/*'], task('js'));
	watch(['src/css/*'], task('css'));
});