<!DOCTYPE html>
<html lang="en">
    <head>
        <title>@yield('title')</title>

        <!-- CSS And JavaScript -->
        <link rel="stylesheet" href="{{ asset('css/app.css') }}" />
        <script type="text/javascript" src="{{ asset('js/jquery.js') }}"></script>
        <script type="text/javascript" src="{{ asset('js/bootstrap.min.js') }}"></script>
        <script type="text/javascript" src="{{ asset('js/laroute.js') }}"></script>
        <script type="text/javascript" src="{{ asset('js/filesize.js') }}"></script>

        <style>
			.glyphicon.spinning {
				animation: spin 1s infinite linear;
				-webkit-animation: spin2 1s infinite linear;
			}

			@keyframes spin {
				from { transform: scale(1) rotate(0deg); }
				to { transform: scale(1) rotate(360deg); }
			}

			@-webkit-keyframes spin2 {
				from { -webkit-transform: rotate(0deg); }
				to { -webkit-transform: rotate(360deg); }
			}
        </style>
    </head>

    <body>
        <div class="container">

        @yield('content')

        </div>

        @include('footer')

    </body>
</html>
