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


#include "mkeyoverridedata.h"

#include <QDebug>

namespace
{
    inline bool keyOverrideLessThan(const QSharedPointer<MKeyOverride> &k1, const QSharedPointer<MKeyOverride> &k2)
    {
        return (k1->keyId() < k2->keyId());
    };
}

MKeyOverrideData::MKeyOverrideData()
{
}

MKeyOverrideData::~MKeyOverrideData()
{
}

QList<QSharedPointer<MKeyOverride> > MKeyOverrideData::keyOverrides() const
{
    QList<QSharedPointer<MKeyOverride> > results = mKeyOverrides.values();
    qSort(results.begin(), results.end(), keyOverrideLessThan);
    return results;
}

bool MKeyOverrideData::createKeyOverride(const QString &keyId)
{
    qDebug() << __PRETTY_FUNCTION__;
    if (!mKeyOverrides.contains(keyId)) {
        QSharedPointer<MKeyOverride> keyOverride;
        keyOverride = QSharedPointer<MKeyOverride>(new MKeyOverride(keyId));
        mKeyOverrides.insert(keyId, keyOverride);
        return true;
    }
    return false;
}

QSharedPointer<MKeyOverride> MKeyOverrideData::keyOverride(const QString &keyId) const
{
    return mKeyOverrides.value(keyId);
}
