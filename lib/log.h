/*************************************************************************************
 *  Copyright 2016 by Sebastian Kügler <sebas@kde.org>                               *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Lesser General Public                       *
 *  License as published by the Free Software Foundation; either                     *
 *  version 2.1 of the License, or (at your option) any later version.               *
 *                                                                                   *
 *  This library is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 *  Lesser General Public License for more details.                                  *
 *                                                                                   *
 *  You should have received a copy of the GNU Lesser General Public                 *
 *  License along with this library; if not, write to the Free Software              *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA       *
 *************************************************************************************/
#ifndef DISMAN_LOG_H
#define DISMAN_LOG_H

#include "disman_export.h"
#include "types.h"

#include <QLoggingCategory>
#include <QObject>

namespace Disman
{

void log(const QString& msg);

/** Disman-internal file logging.
 *
 * The purpose of this class is to allow better debugging of disman. QDebug falls short here, since
 * we need to debug the concert of disman components from different processes.
 *
 * Disman::Log manages access to disman's log file.
 *
 * The following environment variables are considered:
 * - disable logging by setting
 * DISMAN_LOGGING=false
 * - set the log file to a custom path, the default is in ~/.local/share/disman/disman.log
 *
 * Please do not translate messages written to the logs, it's developer information and should be
 * english, independent from the user's locale preferences.
 *
 * @code
 *
 * Log::instance()->set_context("resume");
 * Log::log("Applying detected output configuration.");
 *
 * @endcode
 *
 * @since 5.8
 */
class DISMAN_EXPORT Log
{
public:
    virtual ~Log();

    static Log* instance();

    /** Log a message to a file
     *
     * Call this static method to add a new line to the log.
     *
     * @arg msg The log message to write.
     */
    static void log(const QString& msg, const QString& category = QString());

    /** Context for the logs.
     *
     * The context can be used to indicate what is going on overall, it is used to be able
     * to group log entries into subsequential operations. For example the context can be
     * "handling resume", which is then added to the log messages.
     *
     * @arg msg The log message to write to the file.
     *
     * @see ontext()
     */
    QString context() const;

    /** Set the context for the logs.
     *
     * @see context()
     */
    void set_context(const QString& context);

    /** Logging to file is enabled by environmental var, is it?
     *
     * @arg msg The log message to write to the file.
     * @return Whether logging is enabled.
     */
    bool enabled() const;

    /** Path to the log file
     *
     * This is usually ~/.local/share/disman/disman.log, but can be changed by setting
     * DISMAN_LOGFILE in the environment.
     *
     * @return The path to the log file.
     */
    QString file() const;

private:
    explicit Log();
    class Private;
    Private* const d;

    static Log* sInstance;
    explicit Log(Private* dd);
};

}

#endif
