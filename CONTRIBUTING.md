# Contributing to Disman

 - [Logging and Debugging](#logging-and-debugging)
     - [Runtime logging](#runtime-logging)
     - [Disman's D-Bus backend service](#dismans-d-bus-backend-service)
 - [Submission Guideline](#submission-guideline)
 - [Commit Message Guideline](#commit-message-guideline)
 - [Contact](#contact)

## Logging and Debugging
The first step in contributing to the project
by either providing meaningful feedback
or by sending in patches
is always the analysis of the runtime behavior of a program
that makes use of Disman.
This means studying the debug log
of the program.

### Runtime logging
*KDisplay* with its daemon module and KCM is a program that makes use of Disman.
For further information on how to log and debug KDisplay and by that also Disman
see the respective chapter [Logging and Debugging][kdisplay-log-debug]
in KDisplay's Contributing document.

### Disman's D-Bus backend service
Disman is primarily a library that other programs like KDisplay can link against to make use
of additional functionality.

But Disman also contains a second component that runs on its own:
a D-Bus service that acts as a backend launcher.
Under normal operation this service is always launched *once per session*
when an arbitrary program that uses Disman is executed.
By this the service provides a single shared backend connection
for all programs that make use of Disman.

To view the log of the service you must find the location of the service binary file.
This is normally found at some path like `/usr/lib/libexec/disman-launcher` and then you
can simply kill and restart the service with the command:

    killall -9 disman-launcher ; /usr/lib/libexec/disman-launcher

Note that afterwards you need to restart all programs that made use of the old
instance of the service to be able to connect to the new one.
When you start another program that makes use of Disman
it will automatically connect to this new service instance.

## Submission Guideline
Code contributions to Disman are very welcome but follow a strict process that is layed out in
detail in Wrapland's [Contributing document][wrapland-submissions].

*Summarizing the main points:*

* Use [merge requests][merge-request] directly for smaller contributions, but create
  [issue tickets][issue] *beforehand* for [larger changes][wrapland-large-changes].
* Adhere to the [KDE Frameworks Coding Style][frameworks-style].
* Merge requests have to be posted against master or a feature branch. Commits to the stable branch
  are only cherry-picked from the master branch after some testing on the master branch.

## Commit Message Guideline
The [Conventional Commits 1.0.0][conventional-commits] specification is applied with the following
amendments:

* Only the following types are allowed:
  * build: changes to the CMake build system, dependencies or other build-related tooling
  * ci: changes to CI configuration files and scripts
  * docs: documentation only changes to overall project or code
  * feat: new feature
  * fix: bug fix
  * perf: performance improvement
  * refactor: rewrite of code logic that neither fixes a bug nor adds a feature
  * style: improvements to code style without logic change
  * test: addition of a new test or correction of an existing one
* Only the following optional scopes are allowed:
  * ctl
  * lib
  * qscreen
  * randr
  * wayland
* Angular's [Revert][angular-revert] and [Subject][angular-subject] policies are applied.

### Example

    fix(lib): provide correct return value

    For function exampleFunction the return value was incorrect.
    Instead provide the correct value A by changing B to C.

### Tooling
See [Wrapland's documentation][wrapland-tooling] for available tooling.

## Contact
See [Wrapland's documentation][wrapland-contact] for contact information.

[angular-revert]: https://github.com/angular/angular/blob/3cf2005a936bec2058610b0786dd0671dae3d358/CONTRIBUTING.md#revert
[angular-subject]: https://github.com/angular/angular/blob/3cf2005a936bec2058610b0786dd0671dae3d358/CONTRIBUTING.md#subject
[conventional-commits]: https://www.conventionalcommits.org/en/v1.0.0/#specification
[frameworks-style]: https://community.kde.org/Policies/Frameworks_Coding_Style
[issue]: https://gitlab.com/kwinft/disman/-/issues
[kdisplay-log-debug]: https://gitlab.com/kwinft/kdisplay/-/blob/master/CONTRIBUTING.md#logging-and-debugging
[merge-request]: https://gitlab.com/kwinft/disman/-/merge_requests
[plasma-schedule]: https://community.kde.org/Schedules/Plasma_5
[wrapland-contact]: https://gitlab.com/kwinft/wrapland/-/blob/master/CONTRIBUTING.md#contact
[wrapland-large-changes]: https://gitlab.com/kwinft/wrapland/-/blob/master/CONTRIBUTING.md#issues-for-large-changes
[wrapland-submissions]: https://gitlab.com/kwinft/wrapland/-/blob/master/CONTRIBUTING.md#submission-guideline
[wrapland-tooling]: https://gitlab.com/kwinft/wrapland/-/blob/master/CONTRIBUTING.md#tooling
