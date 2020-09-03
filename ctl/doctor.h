/*
    SPDX-FileCopyrightText: 2015 by Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#ifndef DISMAN_DOCTOR_H
#define DISMAN_DOCTOR_H

#include "output.h"
#include "types.h"
#include "watcher.h"

#include <QObject>
#include <memory>

class QCommandLineParser;

namespace Disman
{
class ConfigOperation;

namespace Ctl
{

class Doctor : public QObject
{
    Q_OBJECT

public:
    explicit Doctor(QCommandLineParser* parser, QObject* parent = nullptr);

    void configReceived(Disman::ConfigOperation* op);

    void showBackends() const;
    static void showOutputs(Disman::ConfigPtr const& config);
    void showJson() const;

    bool setEnabled(int id, bool enabled);
    bool setPosition(int id, const QPoint& pos);
    bool setMode(int id, const QString& mode_id);
    bool setScale(int id, qreal scale);
    bool setRotation(int id, Disman::Output::Rotation rot);

private:
    void applyConfig();
    void parsePositionalArgs();
    int parseInt(const QString& str, bool& ok) const;

    Disman::ConfigPtr m_config;
    std::unique_ptr<Watcher> m_watcher;

    QCommandLineParser* m_parser;
    bool m_changed;
    QStringList m_positionalArgs;
};

}
}

#endif
