/*
    SPDX-FileCopyrightText: 2015 by Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only
*/
#ifndef DISMAN_DOCTOR_H
#define DISMAN_DOCTOR_H

#include "config.h"
#include "output.h"

#include <QCommandLineParser>
#include <QObject>

namespace Disman
{
class ConfigOperation;

namespace Ctl
{

class Doctor : public QObject
{
    Q_OBJECT

public:
    explicit Doctor(QObject* parent = nullptr);

    void setOptionList(const QStringList& positionalArgs);
    void start(QCommandLineParser* m_parser);
    void configReceived(Disman::ConfigOperation* op);

    void showBackends() const;
    void showOutputs() const;
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
    QCommandLineParser* m_parser;
    bool m_changed;
    QStringList m_positionalArgs;
};

}
}

#endif
