dcmon
=====

A log viewer for docker-compose.

dcmon monitors the status and logs for containers managed by docker-compose. Each container's logs are displayed in a
separate tab in realtime. Log entries are automatically collapsed based on indentation for easier browsing. dcmon also
offers quick start, stop, and restart commands for monitored containers.


Building
--------

Dependencies:
* Docker
* docker-compose
* Qt (5.12 has been tested, other versions may work)

Build instructions:
```sh
qmake
make
```

Running
-------

On Windows and Linux, invoke `dcmon` with the path to your `docker-compose.yml` file, or launch it from the directory containing it.

On macOS, use `open /path/to/dcmon.app --args /path/to/docker-compose.yml`.


Roadmap
-------

In no particular order:

* Prompt the user for a `docker-compose.yml` file if one is not automatically detected.
* Lua scripting support for configuration, log filtering, and status reporting.
* Regular-expression search.
* Filter views.
* Desktop notifications for status changes and monitored keywords.
* UI polish.
* Documentation.


License
-------

dcmon is copyright &copy; 2021 Flight Centre Travel Group.

dcmon is written and maintained by Adam Higerd (chighland@gmail.com).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
[GNU General Public License](LICENSE.md) for more details.
