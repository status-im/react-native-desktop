
/**
 * Copyright (c) 2017-present, Status Research and Development GmbH.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickItem>
#include <QString>
#include <QVariant>

#include <QDebug>

#include "reactattachedproperties.h"
#include "reactbridge.h"
#include "reactbuttonmanager.h"
#include "reactevents.h"
#include "reactflexlayout.h"
#include "reactpropertyhandler.h"

const QString EVENT_ONPRESSED = "onPress";

class ReactButtonManagerPrivate {};

ReactButtonManager::ReactButtonManager(QObject* parent)
    : ReactViewManager(parent), d_ptr(new ReactButtonManagerPrivate) {}

ReactButtonManager::~ReactButtonManager() {}

ReactViewManager* ReactButtonManager::viewManager() {
    return this;
}

QString ReactButtonManager::moduleName() {
    return "RCTButtonViewManager";
}

QStringList ReactButtonManager::customDirectEventTypes() {
    return QStringList{
        normalizeInputEventName(EVENT_ONPRESSED),
    };
}

void ReactButtonManager::sendPressedNotificationToJs(QQuickItem* button) {
    int reactTag = ReactAttachedProperties::get(button)->tag();
    bridge()->enqueueJSCall(
        "RCTEventEmitter", "receiveEvent", QVariantList{reactTag, normalizeInputEventName(EVENT_ONPRESSED), {}});
}

QString ReactButtonManager::qmlComponentFile() const {
    return ":/qml/ReactButton.qml";
}

void ReactButtonManager::configureView(QQuickItem* view) const {
    ReactViewManager::configureView(view);
    view->setProperty("buttonManager", QVariant::fromValue((QObject*)this));
    if (shouldLayout()) {
        // In React Native <Button> should be visible even if we didn't
        // specify height and width. So we tell flex layout system to take
        // into account implicit values in ReactButton.qml
        ReactFlexLayout::get(view)->setQmlImplicitWidth(true);
        ReactFlexLayout::get(view)->setQmlImplicitHeight(true);
    }
}

#include "reactbuttonmanager.moc"
