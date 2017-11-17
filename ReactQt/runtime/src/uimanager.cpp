
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

#include <algorithm>
#include <iterator>

#include <QJsonDocument>
#include <QMetaMethod>
#include <QMetaObject>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>

#include <QDebug>

#include "attachedproperties.h"
#include "bridge.h"
#include "componentdata.h"
#include "componentmanagers/viewmanager.h"
#include "layout/flexbox.h"
#include "moduledata.h"
#include "modulemethod.h"
#include "reactitem.h"
#include "uimanager.h"

int UIManager::m_nextRootTag = 1;

void UIManager::removeSubviewsFromContainerWithID(int containerReactTag) {
    // qDebug() << __PRETTY_FUNCTION__;

    QQuickItem* item = m_views.value(containerReactTag);
    if (item == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Attempting to access unknown view";
        return;
    }

    QList<int> childIndexes;
    childIndexes.reserve(item->childItems().size());
    std::iota(childIndexes.begin(), childIndexes.end(), 0);

    manageChildren(containerReactTag, QList<int>(), QList<int>(), QList<int>(), QList<int>(), childIndexes);
}

void UIManager::measure(int reactTag, const ModuleInterface::ListArgumentBlock& callback) {
    // qDebug() << __PRETTY_FUNCTION__;

    QQuickItem* item = m_views.value(reactTag);
    if (item == nullptr) {
        qWarning() << "Attempting to access unknown view";
        callback(m_bridge, QVariantList{});
        return;
    }

    QPointF rvo(item->x(), item->y());
    rvo = item->mapToItem(m_bridge->visualParent(), rvo);

    callback(m_bridge, QVariantList{item->x(), item->y(), item->width(), item->height(), rvo.x(), rvo.y()});
}

void UIManager::updateView(int reactTag, const QString& viewName, const QVariantMap& properties) {
    // qDebug() << __PRETTY_FUNCTION__ << reactTag << viewName << properties;

    QQuickItem* item = m_views.value(reactTag);
    if (item == nullptr) {
        qWarning() << "Attempting to update properties on unknown view; reactTag=" << reactTag
                   << "viewName=" << viewName;
        return;
    }

    Q_ASSERT(AttachedProperties::get(item) != nullptr);
    AttachedProperties::get(item)->applyProperties(properties);

    m_bridge->visualParent()->polish();
}

void UIManager::setChildren(int containerReactTag, const QList<int>& childrenTags) {
    // TODO: This is a simple implementation which fixes a broken example. It's not properly tested and may need
    // revisiting
    QList<int> indices;
    for (int i = 0; i < childrenTags.size(); ++i) {
        indices.append(i);
    }
    manageChildren(containerReactTag, QList<int>(), QList<int>(), childrenTags, indices, QList<int>());
}

void UIManager::removeChildren(QQuickItem* parent, const QList<int>& removeAtIndices) {

    Q_ASSERT(parent != nullptr);

    if (!removeAtIndices.isEmpty()) {

        QList<QQuickItem*> itemsToRemove;
        for (int i : removeAtIndices) {
            itemsToRemove.push_back(parent->childItems().at(i));
        }

        for (QQuickItem* child : itemsToRemove) {
            int childTag = AttachedProperties::get(child)->tag();
            child->setParent(0);
            m_views.remove(childTag);
            child->deleteLater();
        }

        Flexbox::findFlexbox(parent)->removeChilds(removeAtIndices);
    }
}

void UIManager::manageChildren(int containerReactTag,
                               const QList<int>& moveFromIndicies,
                               const QList<int>& moveToIndices,
                               const QList<int>& addChildReactTags,
                               const QList<int>& addAtIndices,
                               const QList<int>& removeAtIndices) {

    QQuickItem* container = m_views[containerReactTag];
    if (container == nullptr) {
        qWarning() << "Attempting to manage children on an unknown container";
        return;
    }

    removeChildren(container, removeAtIndices);

    QList<QQuickItem*> children;
    // XXX: Assumption - addChildReactTags is sorted
    std::transform(addChildReactTags.begin(), addChildReactTags.end(), std::back_inserter(children), [this](int key) {
        return m_views.value(key);
    });

    if (children.size() > 0) {
        // on iOS, order of the subviews implies z-order, implicitly its the same in
        // QML, barring some exceptions. revisit - set zorder appears to be the only
        // exception can probably self order items, but it's not an explicit guarantee
        QList<QQuickItem*>::iterator it = children.begin();
        for (int i : addAtIndices) {
            QQuickItem* child = *it++;

            // Add to visual hierarchy
            ViewManager* vm = AttachedProperties::get(container)->viewManager();
            if (vm != nullptr) {
                vm->addChildItem(container, child, i);
            } else {
                child->setParentItem(container);
            }

            auto containerFlexbox = Flexbox::findFlexbox(container);
            auto childFlexbox = Flexbox::findFlexbox(child);
            if (containerFlexbox && childFlexbox) {
                containerFlexbox->addChild(i, childFlexbox);
            }
        }
    }

    m_bridge->visualParent()->polish();
}

void UIManager::replaceExistingNonRootView(int reactTag, int newReactTag) {
    QQuickItem* oldItem = m_views.value(reactTag);
    if (oldItem == nullptr) {
        qCritical() << __PRETTY_FUNCTION__ << "Attempting to access unknown item";
        return;
    }

    QQuickItem* parent = oldItem->parentItem();
    Q_ASSERT(parent != nullptr);

    int itemIndex = -1;
    itemIndex = parent->childItems().indexOf(oldItem);
    Q_ASSERT(itemIndex >= 0);

    manageChildren(AttachedProperties::get(parent)->tag(),
                   QList<int>(),
                   QList<int>(),
                   QList<int>{newReactTag},
                   QList<int>{itemIndex},
                   QList<int>{itemIndex});
}

void UIManager::measureLayout(int reactTag,
                              int ancestorReactTag,
                              const ModuleInterface::ListArgumentBlock& errorCallback,
                              const ModuleInterface::ListArgumentBlock& callback) {
    QQuickItem* item = m_views.value(reactTag);
    QQuickItem* ancestor = m_views.value(reactTag);
    Q_ASSERT(item != nullptr && ancestor != nullptr);

    int depth = 30; // max depth from ios
    double width = item->width();
    double height = item->height();
    double x = 0;
    double y = 0;

    while (depth > 0 && item != ancestor) {
        x += item->x();
        y += item->y();
        item = item->parentItem();
        --depth;
    }

    if (item != ancestor) {
        callback(m_bridge, QVariantList{0, 0, 0, 0});
    } else {
        callback(m_bridge, QVariantList{x, y, width, height});
    }
}

void UIManager::measureLayoutRelativeToParent(int reactTag,
                                              const ModuleInterface::ListArgumentBlock& errorCallback,
                                              const ModuleInterface::ListArgumentBlock& callback) {
    QQuickItem* item = m_views.value(reactTag);
    Q_ASSERT(item != nullptr);

    AttachedProperties* ap = AttachedProperties::get(item->parentItem());
    if (ap == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "no parent item!";
        return;
    }
    measureLayout(reactTag, ap->tag(), errorCallback, callback);
}

// Reacts version of first responder
void UIManager::setJSResponder(int reactTag, bool blockNativeResponder) {
    Q_UNUSED(reactTag);
    Q_UNUSED(blockNativeResponder);

    // qDebug() << __PRETTY_FUNCTION__;
}

void UIManager::clearJSResponder() {
    // qDebug() << __PRETTY_FUNCTION__;
}

// in iOS, resign first responder (actual)
void UIManager::blur(int reactTag) {
    Q_UNUSED(reactTag);

    // qDebug() << __PRETTY_FUNCTION__;
}

void UIManager::createView(int reactTag, const QString& viewName, int rootTag, const QVariantMap& props) {
    Q_UNUSED(rootTag);

    // qDebug() << __PRETTY_FUNCTION__ << reactTag << viewName << rootTag << props;
    ComponentData* cd = m_componentData.value(viewName);
    if (cd == nullptr) {
        qCritical() << "Attempt to create unknown view of type" << viewName;
        return;
    }

    QQuickItem* item = cd->createView(reactTag, props);
    if (item == nullptr) {
        qWarning() << "Failed to create view of type" << viewName;
        return;
    }

    AttachedProperties* ap = AttachedProperties::get(item);

    // TODO: move to createView?
    if (!props.isEmpty()) {
        ap->applyProperties(props);
    }

    m_views.insert(reactTag, item);
}

void UIManager::findSubviewIn(int reactTag, const QPointF& point, const ModuleInterface::ListArgumentBlock& callback) {
    QQuickItem* item = m_views.value(reactTag);
    if (item == nullptr) {
        qWarning() << "Attempting to access unknown view";
        callback(m_bridge, QVariantList{});
        return;
    }

    // Find the deepest match
    QQuickItem* target = nullptr;
    QQuickItem* next = item;
    QPointF local = point;
    forever {
        target = next;
        next = target->childAt(local.x(), local.y());
        if (next == nullptr || !next->isEnabled())
            break;
        local = target->mapToItem(next, local);
    }

    // XXX: should climb back up to a matching react target?
    AttachedProperties* properties = AttachedProperties::get(target, false);
    if (properties == nullptr) {
        qWarning() << "Found target on a non react view";
        callback(m_bridge, QVariantList{});
        return;
    }

    QRectF frame = item->mapRectFromItem(target, QRectF(target->x(), target->y(), target->width(), target->height()));
    callback(m_bridge, QVariantList{properties->tag(), frame.x(), frame.y(), frame.width(), frame.height()});
}

void UIManager::dispatchViewManagerCommand(int reactTag, int commandID, const QVariantList& commandArgs) {
    // qDebug() << __PRETTY_FUNCTION__ << reactTag << commandID << commandArgs;
    QQuickItem* item = m_views.value(reactTag);
    if (item == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Attempting to access unknown view";
        return;
    }
    QString moduleName = AttachedProperties::get(item)->viewManager()->moduleName();
    // XXX:
    int mi = moduleName.indexOf("Manager");
    if (mi != -1)
        moduleName = moduleName.left(mi);
    ComponentData* cd = m_componentData[moduleName];
    if (cd == nullptr) {
        qWarning() << __PRETTY_FUNCTION__ << "Could not find valid module information";
        return;
    }
    ModuleMethod* mm = cd->method(commandID);
    Q_ASSERT(mm != nullptr);
    QVariantList args = QVariantList{reactTag};
    args.append(commandArgs);
    mm->invoke(args);
}

void UIManager::takeSnapshot(const QString& target,
                             const QVariantMap& options,
                             const ModuleInterface::ListArgumentBlock& resolve,
                             const ModuleInterface::ListArgumentBlock& reject) {
    // qDebug() << __PRETTY_FUNCTION__ << target << options;
    QString imageFormat = options.value("format").toString();
    if (imageFormat.isEmpty()) {
        imageFormat = "png";
    }
    QSize imageSize;
    if (options.contains("width") && options.contains("height")) {
        imageSize = QSize(options.value("width").toInt(), options.value("height").toInt());
    }
    int quality = 100;
    if (options.contains("quality")) {
        quality = options.value("quality").toDouble() * 100;
    }

    auto callback = [=](const QImage& image) {
        QTemporaryFile* imageFile = new QTemporaryFile(m_bridge);
        imageFile->setFileTemplate(QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                                   QString("/XXXXXX.") + imageFormat);
        if (imageFile->open()) {
            QImage saveImage = image;
            if (imageSize.isValid())
                saveImage = image.scaled(imageSize);

            if (saveImage.save(imageFile, imageFormat.toLocal8Bit().constData(), quality)) {
                resolve(m_bridge, QVariantList{QUrl::fromLocalFile(imageFile->fileName())});
                return;
            }
        }

        reject(m_bridge, QVariantList{QVariantMap{{"error", "Unable to save image to file"}}});
    };

    if (target == "window") {
        callback(m_bridge->visualParent()->window()->grabWindow());
    } else {
        QQuickItem* item = m_views.value(target.toInt());
        if (item == nullptr) {
            reject(m_bridge, QVariantList{QVariantMap{{"error", "Could not find view"}}});
            return;
        }
        QSharedPointer<QQuickItemGrabResult> grabResult = item->grabToImage(imageSize);
        connect(grabResult.data(), &QQuickItemGrabResult::ready, [=] { callback(grabResult->image()); });
    }
}

UIManager::UIManager() : m_bridge(nullptr) {}

UIManager::~UIManager() {
    // reset();
}

void UIManager::reset() {
    // Avoid to delete root view on reset
    QQuickItem* rootView = nullptr;
    if (m_rootTag != -1 && m_views.contains(m_rootTag)) {
        rootView = m_views[m_rootTag];
        if (rootView) {
            m_views.remove(m_rootTag);
        }
    }

    for (auto& v : m_views) {
        v->setParentItem(nullptr);
        v->deleteLater();
    }
    m_views.clear();

    if (rootView) {
        m_views.insert(m_rootTag, rootView);
    }
    m_bridge->visualParent()->polish();
}

void UIManager::setBridge(Bridge* bridge) {
    // qDebug() << __PRETTY_FUNCTION__;
    if (m_bridge != nullptr) {
        qCritical() << "Bridge already set, UIManager already initialised?";
        return;
    }

    m_bridge = bridge;

    for (ModuleData* data : m_bridge->modules()) {
        ViewManager* manager = data->viewManager();
        if (manager != nullptr) {
            ComponentData* cd = new ComponentData(data);
            m_componentData.insert(cd->name(), cd);
        }
    }
}

QString UIManager::moduleName() {
    return "RCTUIManager";
}

QList<ModuleMethod*> UIManager::methodsToExport() {
    return QList<ModuleMethod*>{};
}

QVariantMap UIManager::constantsToExport() {
    QVariantMap rc;
    QVariantMap directEvents;
    QVariantMap bubblingEvents;

    for (const ComponentData* componentData : m_componentData) {
        // qDebug() << "Checking" << componentData->name();

        QVariantMap managerInfo;

        managerInfo.insert("Manager", componentData->manager()->moduleName());

        QVariantMap config = componentData->viewConfig();
        if (!config.isEmpty()) {
            managerInfo.insert("NativeProps", config["propTypes"]);
        }

        for (const QString& eventName : config["directEvents"].toStringList()) {
            if (!directEvents.contains(eventName)) {
                QString tmp = eventName;
                tmp.replace(0, 3, "on");
                directEvents.insert(eventName, QVariantMap{{"registrationName", tmp}});
            }
        }

        for (const QString& eventName : config["bubblingEvents"].toStringList()) {
            if (!bubblingEvents.contains(eventName)) {
                QString tmp = eventName;
                tmp.replace(0, 3, "on");
                bubblingEvents.insert(
                    eventName,
                    QVariantMap{{"phasedRegistrationNames",
                                 QVariantMap{{"bubbled", tmp}, {"captured", tmp.append("Capture")}}}});
            }
        }

        rc.insert(componentData->name(), managerInfo);
    }

    rc.insert("customBubblingEventTypes", bubblingEvents);
    rc.insert("customDirectEventTypes", directEvents);
    rc.insert("Dimensions",
              QVariantMap{{"window",
                           QVariantMap{{"width", m_bridge->visualParent()->width()},
                                       {"height", m_bridge->visualParent()->height()},
                                       {"scale", m_bridge->visualParent()->scale()}}},
                          {"modalFullscreenView",
                           QVariantMap{{"width", m_bridge->visualParent()->width()},
                                       {"height", m_bridge->visualParent()->height()}}}});
    rc.insert(
        "modalFullscreenView",
        QVariantMap{{"width", m_bridge->visualParent()->width()}, {"height", m_bridge->visualParent()->height()}});

    return rc;
}

int UIManager::allocateRootTag() {
    int tag = m_nextRootTag;
    m_nextRootTag += 10;
    return tag;
}

void UIManager::registerRootView(QQuickItem* root) {
    AttachedProperties* properties = AttachedProperties::get(root);
    m_rootTag = properties->tag();
    m_views.insert(m_rootTag, root);
}

QQuickItem* UIManager::viewForTag(int reactTag) {
    return m_views.value(reactTag);
}
