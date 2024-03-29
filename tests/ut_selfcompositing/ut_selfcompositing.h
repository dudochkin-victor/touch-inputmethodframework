/* * This file is part of meego-im-framework *
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef UT_SELFCOMPOSITING_H
#define UT_SELFCOMPOSITING_H

#include <QtTest/QtTest>
#include <QtCore>
#include <QtGui>

class MIMApplication;

class Ut_SelfCompositing : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testSelfCompositing_data();
    void testSelfCompositing();

private:
    MIMApplication *app;
};

#endif // UT_SELFCOMPOSITING_H
