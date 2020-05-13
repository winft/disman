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

#ifndef DISMAN_SETCONFIGOPERATION_H
#define DISMAN_SETCONFIGOPERATION_H

#include "configoperation.h"
#include "types.h"
#include "disman_export.h"

namespace Disman {

class SetConfigOperationPrivate;

class DISMAN_EXPORT SetConfigOperation : public Disman::ConfigOperation
{
    Q_OBJECT
public:
    explicit SetConfigOperation(const Disman::ConfigPtr &config, QObject* parent = nullptr);
    ~SetConfigOperation() override;

    Disman::ConfigPtr config() const override;

protected:
    void start() override;

private:
    Q_DECLARE_PRIVATE(SetConfigOperation)
};

}

#endif // DISMAN_SETCONFIGOPERATION_H
