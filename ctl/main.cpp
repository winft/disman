/*
    SPDX-FileCopyrightText: 2014, 2015 by Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#include "doctor.h"

#include <QCommandLineParser>
#include <QGuiApplication>

/** Usage example:
 * dismanctl --set output.0.disable output.1.mode.1 output.1.enable"
 *
 * Error codes:
 * 2 : general parse error
 * 3 : output id parse error
 * 4 : mode id parse error
 * 5 : position parse error
 *
 * 8 : invalid output id
 * 9 : invalid mode id
 *
 */

int main(int argc, char** argv)
{
    const QString desc = QStringLiteral(
        "dismanctl allows to change the screen setup from the command-line.\n"
        "\n"
        "Setting the output configuration is done in an atomic fashion, all settings\n"
        "are applied in a single command.\n"
        "dismanctl can be used to enable and disable outputs, to position screens,\n"
        "change resolution (mode setting), etc.. You should put all your options into \n"
        "a single invocation of dismanctl, so they can all be applied at once.\n"
        "\n"
        "Usage examples:\n\n"
        "   Show output information:\n"
        "   $ dismanctl -o\n"
        "   Output: 1 eDP-1 enabled connected Panel Modes: Modes: 1:800x600@60 [...] Geometry: 0,0 "
        "1280x800\n"
        "   Output: 70 HDMI-2 enabled connected  HDMI Modes: 1:800x600@60 [...] Geometry: 1280,0 "
        "1920x1080\n"
        "\n   Disable the hdmi output, enable the laptop panel and set it to a specific mode\n"
        "   $ dismanctl output.HDMI-2.disable output.eDP-1.mode.1 output.eDP-1.enable\n"
        "\n   Position the hdmi monitor on the right of the laptop panel\n"
        "   $ dismanctl output.HDMI-2.position.0,1280 output.eDP-1.position.0,0\n"
        "\n   Set resolution mode\n"
        "   $ dismanctl output.HDMI-2.mode.1920x1080@60 \n"
        "\n   Set scale (note: fractional scaling is only supported on wayland)\n"
        "   $ dismanctl output.HDMI-2.scale.2 \n"
        "\n   Set rotation (possible values: none, left, right, inverted)\n"
        "   $ dismanctl output.HDMI-2.rotation.left \n");
    /*
        "\nError codes:\n"
        "   2 : general parse error\n"
        "   3 : output id parse error\n"
        "   4 : mode id parse error\n"
        "   5 : position parse error\n"

        "   8 : invalid output id\n"
        "   9 : invalid mode id\n";
    */
    const QString syntax = QStringLiteral(
        "Specific output settings are separated by spaces, each setting is in the form of\n"
        "output.<name>.<setting>[.<value>]\n"
        "For example:\n"
        "   $ dismanctl output.HDMI-2.enable \\\n"
        "               output.eDP-1.mode.4  \\\n"
        "               output.eDP-1.position.1280,0\n"
        "Multiple settings are passed in order to have dismanctl apply these settings in one "
        "go.\n");

    QGuiApplication app(argc, argv);

    QCommandLineOption info
        = QCommandLineOption(QStringList() << QStringLiteral("i") << QStringLiteral("info"),
                             QStringLiteral("Show runtime information: backends, logging, etc."));
    QCommandLineOption outputs
        = QCommandLineOption(QStringList() << QStringLiteral("o") << QStringLiteral("outputs"),
                             QStringLiteral("Show outputs"));
    QCommandLineOption json
        = QCommandLineOption(QStringList() << QStringLiteral("j") << QStringLiteral("json"),
                             QStringLiteral("Show configuration in JSON format"));
    QCommandLineOption log
        = QCommandLineOption(QStringList() << QStringLiteral("l") << QStringLiteral("log"),
                             QStringLiteral("Write a comment to the log file"),
                             QStringLiteral("comment"));
    QCommandLineOption watch
        = QCommandLineOption(QStringList() << QStringLiteral("w") << QStringLiteral("watch"),
                             QStringLiteral("Watch for changes and print them to stdout."));

    QCommandLineParser parser;
    parser.setApplicationDescription(desc);
    parser.addPositionalArgument(
        QStringLiteral("config"),
        syntax,
        QStringLiteral("[output.<name>.<setting> output.<name>.setting [...]]"));
    parser.addHelpOption();
    parser.addOption(info);
    parser.addOption(json);
    parser.addOption(outputs);
    parser.addOption(log);
    parser.addOption(watch);
    parser.process(app);

    Disman::Ctl::Doctor server(&parser);

    return app.exec();
}
