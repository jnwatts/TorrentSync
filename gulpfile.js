var gulp = require('gulp');
var terser = require('gulp-terser');

gulp.task('html', function() {
	return gulp.src('src/html/index.html')
		.pipe(gulp.dest('public'));
});

gulp.task('js', function() {
	return gulp.src([
			'src/js/*.js',
			'node_modules/jquery/dist/jquery.js',
			'node_modules/bootstrap/dist/js/bootstrap.js',
			'node_modules/jsonrpc-lite/jsonrpc.js',
			'node_modules/url-parse/dist/url-parse.js',
			'node_modules/moment/moment.js',
			])
		.pipe(terser())
		.pipe(gulp.dest('public/js'));
});

gulp.task('css', function() {
	return gulp.src([
			'src/css/*.css',
			'node_modules/bootstrap/dist/css/bootstrap.css'
			])
		.pipe(gulp.dest('public/css'));
});

gulp.task('fonts', function() {
	return gulp.src([
			'node_modules/bootstrap/dist/fonts/*'
			])
		.pipe(gulp.dest('public/fonts'));
});

gulp.task('default', gulp.series('html', 'js', 'css', 'fonts'));
