% dismanctl(1) Disman 0.20.1
% Roman Gilg
% 2021-01-10

# NAME
dismanctl - allows to change the screen setup from the command-line

# SYNOPSIS
**dismanctl** [options] [output.[name.\<setting\> output.\<name\>.setting (...)]

# DESCRIPTION
**dismanctl** can be used to enable and disable outputs, to position screens, change resolution (mode setting), etc. \
You should put all your options into  a single invocation of dismanctl, so they can all be applied at once. This is because setting the output configuration is done in an atomic fashion, so all settings are applied in a single command.

# OPTIONS
-h, \-\-help
: Displays help on commandline options.

\-\-help-all
: Displays help including Qt specific options.

-i, \-\-info 
: Show runtime information: backends, logging, etc.

-j, \-\-json
: Show configuration in JSON format

-o, \-\-outputs
: Show outputs

-l, \-\-log <comment>
: Write a comment to the log file

-w, \-\-watch
: Watch for changes and print them to stdout.

# ARGUMENTS
Specific output settings are separated by spaces, each setting is in the form of output.\<name\>.\<setting\>[.\<value\>] \
Multiple settings are passed in order to have dismanctl apply these settings in one go.

For example:
: dismanctl output.HDMI-2.enable output.eDP-1.mode.4 output.eDP-1.position.1280,0

# EXAMPLES
**Show output information:**
: dismanctl -o

**Disable the HDMI output, enable the laptop panel and set it to a specific mode:**
: dismanctl output.HDMI-2.disable output.eDP-1.mode.1 output.eDP-1.enable

**Position the HDMI monitor on the right of the laptop panel:**
: dismanctl output.HDMI-2.position.0,1280 output.eDP-1.position.0,0

**Set resolution mode:**
: dismanctl output.HDMI-2.mode.1920x1080@60

**Set scale (note: fractional scaling is only supported on wayland):**
: dismanctl output.HDMI-2.scale.2

**Set rotation (possible values: none, left, right, inverted):**
: dismanctl output.HDMI-2.rotation.left
    
# BUGS
Found a bug? Please report it on the issue tracker at https://gitlab.com/kwinft/disman/  \
Your help finding bugs will be greatly appreciated.

# COPYRIGHT
Copyright (c) 2021 Roman Gilg. Licensed under the LGPLv2.1 or later. \
This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version. \
This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
