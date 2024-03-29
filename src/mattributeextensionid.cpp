/* * This file is part of meego-im-framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */


#include "mattributeextensionid.h"

#include <QHash>

namespace {
    const int InvalidId  = -1;
    const int StandardId = -2;
}

MAttributeExtensionId::MAttributeExtensionId()
    : id(InvalidId)
{
}

MAttributeExtensionId::MAttributeExtensionId(int id, const QString &service)
    : id(id),
      m_service(service)
{
}

MAttributeExtensionId MAttributeExtensionId::standardAttributeExtensionId()
{
    return MAttributeExtensionId(StandardId, QString());
}

bool MAttributeExtensionId::isValid() const
{
    return id >= 0 && !m_service.isEmpty();
}

bool MAttributeExtensionId::operator==(const MAttributeExtensionId &other) const
{
    return (id == other.id) && (m_service == other.m_service);
}

bool MAttributeExtensionId::operator!=(const MAttributeExtensionId &other) const
{
    return !operator==(other);
}

QString MAttributeExtensionId::service() const
{
    return m_service;
}

uint qHash(const MAttributeExtensionId &id)
{
    return qHash(QPair<int, QString>(id.id, id.m_service));
}

