# Disman

Disman is a **display management** service and library.

The service can communicate with the **X11** windowing system
and a multitude of **Wayland** compositors.
The library provides convenient objects and methods
for writing frontend GUI applications
that interact with the service.

Additionally the command line utility *dismanctl* is provided
to query and modify display settings directly from the command line.

## Installation
### Packages
Your distribution might provide Disman already as a package:
* Arch(AUR): [disman-kwinft][aur-package]
* Manjaro: `sudo pacman -S disman`

### From Source
#### Dependencies
Disman can also be compiled from source with following dependencies:
* CMake
* Extra CMake Modules
* Qt
* KCoreAddons
* Wrapland - optional for the Wayland backend plugin
* XCB - optional for the X11 backend plugin

#### Target Selection
Disman can be installed like any other CMake based projects through

```sh
cmake -B build-dir -S source-dir
cmake --build build-dir
cmake --install build-dir
```

This builds and installs *all* of Disman's CMake targets,
what then als relies on all optional dependencies being available.
But as Disman comes with multiple targets
that can be built and installed independently
we can also build and install only the targets we really need.

It is recommended to make at least use of the group target *disman* though.
It combines the basic
libraries, plugins, control utility and service
what usually always is used in some way or another.
On the other side for example the target *disman-wayland*
only builds the backend necessary to communicate with a Wayland server.

A typical build and install workflow would look like the following for that plugin:

```sh
cmake -B build-dir -S source-dir
cmake --build build-dir --target disman disman-wayland
cmake --install build-dir --component disman
cmake --install build-dir --component disman-wayland
```

**In an overview the following targets are available:**

| Target/Component | Description                                         |
|------------------|-----------------------------------------------------|
| disman           | Group target bundling core functionality            |
| disman-lib       | Main library being used by frontends and backends   |
| dismanctl        | Command line utility to interact with Disman        |
| disman-launcher  | Service that starts a backend on D-Bus requests     |
| disman-wayland   | Plugin-loader for Wayland                           |
| disman-randr     | Plugin for X11/RandR                                |

#### Packaging
Distro packagers should create multiple packages from these targets
such that these packages depend on each other naturally
and come with minimal dependencies.

It is recommended to at least provide a "disman" package
with the disman group target
and separate disman-randr, disman-kwayland, etc. packages for the backend plugins.

## Usage
### Independent
Disman can be used independently of any frontend just with dismanctl.
To autostart the D-Bus service on system start *at least one* client connection must be initiated.
This can be done by calling once `dismanctl -o` or `dismanctl -w`
from a startup script or systemd unit.

### Frontends
#### KDisplay
The Qt app [KDisplay][kdisplay] is a frontend client for Disman.
It provides a modern UI to modify the display configuration.
It also provides a KDE daemon
that autostarts the Disman D-Bus service in a KDE Plasma session.
So in a KDE Plasma session it is not necessary
to call e.g. `dismanctl -o` from a startup script
as described above.

### Session restarts

When using a display manager
(for example [SDDM][sddm])
session restarts should be unproblematic.

In case a graphical session is started directly from a virtual terminal
make sure the D-Bus environment is reset on a restart.
Otherwise a restart of the D-Bus service might fail
due to outdated environment variables
which are inherited by D-Bus services.

One way to ensure such a reset is to start your desktop session
with `dbus-run-session`.
For example, in the case of KDE Plasma,
if you use `startx` to run Plasma X11 sessions,
you should have the following line in `~/.xinitrc`:

```sh
dbus-run-session startplasma-x11
```

Similarly, if you run a Plasma Wayland session from a terminal, do it like this:

```sh
dbus-run-session startplasma-wayland
```

### Reporting issues
#### Disman or frontend
If you hit bugs while using dismanctl that have not yet been [reported][issues]
please create a new issue ticket.

If you hit the bug while using a frontend that makes use of Disman
decide for yourself if that is likely a bug in Disman itself or rather a bug in the frontend.
As a rule of thumb errors when applying configurations point at Disman being the culprit
while presentation errors in the UI point at the frontend.

#### Good reports
For meaningful logs and backtraces see the [respective section][disman-log-debug]
in the CONTRIBUTING document.

Furthermore to reproduce an issue from a clean state
it is recommended to move Disman's control files temporarily
and after a system restart provide the newly generated files in the bug report.

Disman's control files are usually saved to `$HOME/.local/share/disman`.

## Developers
If you want to use Disman in one of your applications
please be aware
that Disman is currently still in active development.
This includes the API that will go through some more changes for general improvements
and to remove the Qt bits from it in order to make it a C++ only library.

If you still want to start writing a frontend already now
you can take a look at [KDisplay's code][kdisplay-config] for an example.
Disman is released according to the [Plasma release schedule][plasma-schedule]
so you can align your releases with that as well to prevent regressions for your users.

## Contributing
The development of Disman happens in the open.
Technical plans are proposed and discussed in normal GitLab [issue tickets][issues].
For more comprehensive long-term plans special [issue boards][boards] might be created.

See the [CONTRIBUTING.md](CONTRIBUTING.md) file on how to start contributing.

[boards]: https://gitlab.com/kwinft/disman/-/boards
[aur-package]: https://aur.archlinux.org/packages/disman-kwinft
[disman-log-debug]: https://gitlab.com/kwinft/disman/-/blob/master/CONTRIBUTING.md#logging-and-debugging
[issues]: https://gitlab.com/kwinft/disman/-/issues
[kdisplay]: https://gitlab.com/kwinft/kdisplay
[kdisplay-config]: https://gitlab.com/kwinft/kdisplay/-/blob/master/kcm/config_handler.cpp
[plasma-schedule]: https://community.kde.org/Schedules/Plasma_5
[sddm]: https://en.wikipedia.org/wiki/Simple_Desktop_Display_Manager
