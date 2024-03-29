/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <QObject>

#include "backend.h"
#include "backendinterface.h"
#include "configoperation.h"

namespace Disman
{
class ConfigOperationPrivate : public QObject
{
    Q_OBJECT

public:
    explicit ConfigOperationPrivate(ConfigOperation* qq);
    ~ConfigOperationPrivate() override;

    // For out-of-process
    void request_backend();
    virtual void backend_ready(org::kwinft::disman::backend* backend);

    // For in-process
    Disman::Backend* loadBackend();

public Q_SLOTS:
    void do_emit_result();

private:
    QString error;
    bool isExec;

protected:
    ConfigOperation* const q_ptr;
    Q_DECLARE_PUBLIC(ConfigOperation)
};

}
