@extends('layouts.app')

@section('content')

        <div class="panel panel-default">
            <div class="panel-heading">
                Torrents
                <a class="btn" role="button" href="{{ URL::route('torrent.refresh') }}">
                    <span class="glyphicon glyphicon-refresh"></span>
                </a>
                <div class="dropdown" style="display:inline">
					<button class="btn dropdown-toggle" type="button" data-toggle="dropdown">
                        {{ $label->label }}
                        <span class="caret"></span>
                    </button>
					<ul class="dropdown-menu">
                    @foreach ($labels as $l)
                        <li><a href="?label={{ $l->id }}">{{ $l->label }}</a></li>
                    @endforeach
					</ul>
                </div>
            </div>

            <div class="panel-body">
                @if (count($torrents) > 0)
                <table class="table table-striped task-table">

                    <!-- Table Headings -->
                    <thead>
                        <th>Name</th>
                        <th>Progress</th>
                        <th>Time Added</th>
                        <th>Label</th>
                        <th>&nbsp;</th>
                    </thead>

                    <!-- Table Body -->
                    <tbody>
                    @foreach ($torrents as $t)
                        <tr>
                            <!-- Task Name -->
                            <td class="table-text">
                                <div>{{ $t->name }}</div>
                            </td>
                            <td class="table-text">
                                <div>{{ $t->progress }}</div>
                            </td>
                            <td class="table-text">
                                <div>{{ $t->time_added }}</div>
                            </td>
                            <td class="table-text">
                                <div>{{ $t->label->label }}</div>
                            </td>
                            <td class="table-text">
                                <div>
                                @if ($t->task)
                                    @if ($t->task->state == 'complete')
                                        <span class="glyphicon glyphicon-saved" aria-hidden="true"></span>
                                        <a class="btn btn-sm" role="button" href="{{ URL::route('torrent.clear', [$t->hash]) }}">
                                            <span class="glyphicon glyphicon-erase" aria-hidden="true"></span>
                                        </a>
                                    @else
                                        <a class="btn btn-sm" role="button" href="{{ URL::route('torrent.stop', [$t->hash]) }}"><span class="glyphicon glyphicon-transfer" aria-hidden="true"></span></a>
                                    @endif
                                @else
                                    <a class="btn btn-sm" role="button" href="{{ URL::route('torrent.transfer', [$t->hash]) }}"><span class="glyphicon glyphicon-save" aria-hidden="true"></span></a>
                                @endif
                                </div>
                            </td>
                        </tr>
                    @endforeach
                    </tbody>
                </table>
                @endif
            </div>
        </div>

        <div class="panel panel-default">
            <div class="panel-heading">
                TODO
            </div>
            <div class="panel-body">
                <ul>
                    <li>Backend
                        <ul>
                        </ul>
                    </li>
                    <li>UI
                        <ul>
                            <li>Swap redirect() for AJAX</li>
                            <li>Show current state (update on timer? I wonder if I could query remote for size?)</li>
                            <li>Support sort order</li>
                            <li>Cache index view params in cookies (such as current label, sort order, etc)</li>
                            <li>Move Backend URL/port to config</li>
                            <li>Show active queue</li>
                        </ul>
                    </li>
                </ul>
            </div>
        </div>
@endsection

