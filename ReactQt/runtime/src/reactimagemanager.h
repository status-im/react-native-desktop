
/**
 * Copyright (C) 2016, Canonical Ltd.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * Author: Justin McPherson <justin.mcpherson@canonical.com>
 *
 */

#ifndef REACTIMAGEMANAGER_H
#define REACTIMAGEMANAGER_H

#include "reactviewmanager.h"

class ReactPropertyHandler;
class ReactImageManagerPrivate;
class ReactImageManager : public ReactViewManager {
    Q_OBJECT
    Q_INTERFACES(ReactModuleInterface)
    Q_DECLARE_PRIVATE(ReactImageManager)

public:
    ReactImageManager(QObject* parent = 0);
    virtual ~ReactImageManager();

    ReactViewManager* viewManager() override;
    ReactPropertyHandler* propertyHandler(QObject* object) override;

    QString moduleName() override;
    QList<ReactModuleMethod*> methodsToExport() override;
    QVariantMap constantsToExport() override;

    QStringList customDirectEventTypes() override;

public slots:
    void manageSource(const QVariantMap& imageSource, QObject* image);

private:
    virtual void configureView(QQuickItem* view) const;
    virtual QString qmlComponentFile() const;

private:
    QScopedPointer<ReactImageManagerPrivate> d_ptr;
};

#endif // REACTIMAGEMANAGER_H
