
/**
 * Copyright (c) 2017-present, Status Research and Development GmbH.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "flexbox.h"
#include <QDebug>
#include <QMap>
#include <QQuickItem>

static QMap<QString, YGFlexDirection> flexDirectionByString{
    {"row", YGFlexDirectionRow},
    {"row-reverse", YGFlexDirectionRowReverse},
    {"column", YGFlexDirectionColumn},
    {"column-reverse", YGFlexDirectionColumnReverse},
};

static QMap<QString, YGJustify> justificationByString{
    {"flex-start", YGJustifyFlexStart},
    {"flex-end", YGJustifyFlexEnd},
    {"center", YGJustifyCenter},
    {"space-between", YGJustifySpaceBetween},
    {"space-around", YGJustifySpaceAround},
};

static QMap<QString, YGAlign> alignByString{
    {"flex-start", YGAlignFlexStart},
    {"flex-end", YGAlignFlexEnd},
    {"center", YGAlignCenter},
    {"stretch", YGAlignStretch},
    {"baseline", YGAlignBaseline},
    {"space-between", YGAlignSpaceBetween},
    {"space-around", YGAlignSpaceAround},
};

static QMap<QString, YGWrap> wrapByString{
    {"wrap", YGWrapWrap}, {"nowrap", YGWrapNoWrap},
};

static QMap<QString, YGDisplay> displayByString{
    {"none", YGDisplayNone}, {"flex", YGDisplayFlex},
};

static QMap<QString, YGOverflow> overflowByString{
    {"visible", YGOverflowVisible}, {"hidde", YGOverflowHidden}, {"scroll", YGOverflowScroll}};

static QMap<QString, YGPositionType> positionByString{
    {"relative", YGPositionTypeRelative}, {"absolute", YGPositionTypeAbsolute},
};

static QMap<QString, YGDirection> directionByString{
    {"inherit", YGDirectionInherit}, {"ltr", YGDirectionLTR}, {"rtl", YGDirectionRTL}};

class FlexboxPrivate {
public:
    FlexboxPrivate() {
        m_node = YGNodeNew();
        YGNodeSetContext(m_node, this);
    }
    ~FlexboxPrivate() {
        YGNodeRef parent = YGNodeGetParent(m_node);
        if (parent) {
            YGNodeRemoveChild(parent, m_node);
        }
        YGNodeFree(m_node);
    }

    static void updatePropertiesForControlsTree(YGNodeRef node);
    static void updatePropertiesForControl(YGNodeRef node);

    YGNodeRef m_node = nullptr;
    QString m_flexDirection;
    QString m_justifyContent;
    QQuickItem* m_control = nullptr;
    ygnode_measure_function m_measureFunction = nullptr;
    QString m_alignItems;
    QString m_alignContent;
    QString m_alignSelf;
    QString m_flexWrap;
    static QMap<QQuickItem*, Flexbox*> m_FlexboxesForControls;
    QString m_display;
    QString m_overflow;
    QString m_position;
    QString m_direction;
};

QMap<QQuickItem*, Flexbox*> FlexboxPrivate::m_FlexboxesForControls;

YGSize measure(YGNodeRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
    FlexboxPrivate* d = static_cast<FlexboxPrivate*>(YGNodeGetContext(node));
    auto nodeMeasureFunction = d->m_measureFunction;
    return nodeMeasureFunction(node, width, widthMode, height, heightMode);
}

Flexbox::Flexbox(QObject* parent) : QObject(parent), d_ptr(new FlexboxPrivate()) {}

Flexbox::~Flexbox() {}

void Flexbox::recalculateLayout(float width, float height) {
    Q_D(Flexbox);

    YGNodeCalculateLayout(d->m_node, width, height, YGDirectionLTR);
    d->updatePropertiesForControlsTree(d->m_node);
}

void Flexbox::setMeasureFunction(ygnode_measure_function measureFunction) {
    Q_D(Flexbox);

    d->m_measureFunction = measureFunction;
    YGNodeSetMeasureFunc(d->m_node, measure);
}

void Flexbox::addChild(int index, Flexbox* child) {
    Q_ASSERT(child);

    YGNodeInsertChild(d_ptr->m_node, child->d_ptr->m_node, index);
}

void Flexbox::removeChilds(const QList<int>& indicesToRemove) {
    Q_D(Flexbox);

    QList<YGNodeRef> toRemove;
    for (int index : indicesToRemove) {
        toRemove.push_back(YGNodeGetChild(d->m_node, index));
    }

    for (YGNodeRef nodeToRemove : toRemove) {
        YGNodeRemoveChild(d->m_node, nodeToRemove);
    }
}

Flexbox* Flexbox::findFlexbox(QQuickItem* control) {
    return FlexboxPrivate::m_FlexboxesForControls[control];
}

void FlexboxPrivate::updatePropertiesForControlsTree(YGNodeRef node) {
    static int level = 1;

    updatePropertiesForControl(node);

    int childCount = YGNodeGetChildCount(node);
    for (int i = 0; i < childCount; ++i) {
        updatePropertiesForControlsTree(YGNodeGetChild(node, i));
    }
}

void FlexboxPrivate::updatePropertiesForControl(YGNodeRef node) {
    auto qmlControl = static_cast<FlexboxPrivate*>(YGNodeGetContext(node))->m_control;

    qmlControl->setX(YGNodeLayoutGetLeft(node));
    qmlControl->setY(YGNodeLayoutGetTop(node));
    qmlControl->setWidth(YGNodeLayoutGetWidth(node));
    qmlControl->setHeight(YGNodeLayoutGetHeight(node));
}

void Flexbox::setControl(QQuickItem* value) {
    if (value != d_ptr->m_control) {
        d_ptr->m_control = value;
        d_ptr->m_FlexboxesForControls[d_ptr->m_control] = this;
        controlChanged();
    }
}

QQuickItem* Flexbox::control() {
    return d_ptr->m_control;
}

float Flexbox::width() {
    return YGNodeStyleGetWidth(d_ptr->m_node).value;
}

void Flexbox::setWidth(float value) {
    if (value != width()) {
        YGNodeStyleSetWidth(d_ptr->m_node, value);
        widthChanged();
    }
}

float Flexbox::height() {
    return YGNodeStyleGetHeight(d_ptr->m_node).value;
}

void Flexbox::setHeight(float value) {
    if (value != height()) {
        YGNodeStyleSetHeight(d_ptr->m_node, value);
        heightChanged();
    }
}

QString Flexbox::flexDirection() {
    return d_ptr->m_flexDirection;
}

void Flexbox::setFlexDirection(const QString& value) {
    if (value != d_ptr->m_flexDirection) {
        YGNodeStyleSetFlexDirection(d_ptr->m_node, flexDirectionByString[value]);
        flexDirectionChanged();
    }
}

QString Flexbox::justifyContent() {
    return d_ptr->m_justifyContent;
}

void Flexbox::setJustifyContent(const QString& value) {
    if (value != d_ptr->m_justifyContent) {
        YGNodeStyleSetJustifyContent(d_ptr->m_node, justificationByString[value]);
        justifyContentChanged();
    }
}

float Flexbox::margin() {
    return YGNodeStyleGetMargin(d_ptr->m_node, YGEdgeAll).value;
}

void Flexbox::setMargin(float value) {
    if (value != margin()) {
        YGNodeStyleSetMargin(d_ptr->m_node, YGEdgeAll, value);
        marginChanged();
    }
}

float Flexbox::marginTop() {
    return YGNodeStyleGetMargin(d_ptr->m_node, YGEdgeTop).value;
}

void Flexbox::setMarginTop(float value) {
    if (value != marginTop()) {
        YGNodeStyleSetMargin(d_ptr->m_node, YGEdgeTop, value);
        marginTopChanged();
    }
}

float Flexbox::marginBottom() {
    return YGNodeStyleGetMargin(d_ptr->m_node, YGEdgeBottom).value;
}

void Flexbox::setMarginBottom(float value) {
    if (value != marginBottom()) {
        YGNodeStyleSetMargin(d_ptr->m_node, YGEdgeBottom, value);
        marginBottomChanged();
    }
}

float Flexbox::marginLeft() {
    return YGNodeStyleGetMargin(d_ptr->m_node, YGEdgeLeft).value;
}

void Flexbox::setMarginLeft(float value) {
    if (value != marginLeft()) {
        YGNodeStyleSetMargin(d_ptr->m_node, YGEdgeLeft, value);
        marginLeftChanged();
    }
}

float Flexbox::marginRight() {
    return YGNodeStyleGetMargin(d_ptr->m_node, YGEdgeRight).value;
}

void Flexbox::setMarginRight(float value) {
    if (value != marginRight()) {
        YGNodeStyleSetMargin(d_ptr->m_node, YGEdgeRight, value);
        marginRightChanged();
    }
}

float Flexbox::marginHorizontal() {
    return YGNodeStyleGetMargin(d_ptr->m_node, YGEdgeHorizontal).value;
}

void Flexbox::setMarginHorizontal(float value) {
    if (value != marginHorizontal()) {
        YGNodeStyleSetMargin(d_ptr->m_node, YGEdgeHorizontal, value);
        marginHorizontalChanged();
    }
}

float Flexbox::marginVertical() {
    return YGNodeStyleGetMargin(d_ptr->m_node, YGEdgeVertical).value;
}

void Flexbox::setMarginVertical(float value) {
    if (value != marginVertical()) {
        YGNodeStyleSetMargin(d_ptr->m_node, YGEdgeVertical, value);
        marginVerticalChanged();
    }
}

float Flexbox::borderWidth() {
    return YGNodeStyleGetBorder(d_ptr->m_node, YGEdgeAll);
}

void Flexbox::setBorderWidth(float value) {
    if (value != borderWidth()) {
        YGNodeStyleSetBorder(d_ptr->m_node, YGEdgeAll, value);
        borderWidthChanged();
    }
}

float Flexbox::borderTopWidth() {
    return YGNodeStyleGetBorder(d_ptr->m_node, YGEdgeTop);
}

void Flexbox::setBorderTopWidth(float value) {
    if (value != borderTopWidth()) {
        YGNodeStyleSetBorder(d_ptr->m_node, YGEdgeTop, value);
        borderTopWidthChanged();
    }
}

float Flexbox::borderBottomWidth() {
    return YGNodeStyleGetBorder(d_ptr->m_node, YGEdgeBottom);
}

void Flexbox::setBorderBottomWidth(float value) {
    if (value != borderBottomWidth()) {
        YGNodeStyleSetBorder(d_ptr->m_node, YGEdgeBottom, value);
        borderBottomWidthChanged();
    }
}

float Flexbox::borderLeftWidth() {
    return YGNodeStyleGetBorder(d_ptr->m_node, YGEdgeLeft);
}

void Flexbox::setBorderLeftWidth(float value) {
    if (value != borderLeftWidth()) {
        YGNodeStyleSetBorder(d_ptr->m_node, YGEdgeLeft, value);
        borderLeftWidthChanged();
    }
}

float Flexbox::borderRightWidth() {
    return YGNodeStyleGetBorder(d_ptr->m_node, YGEdgeRight);
}

void Flexbox::setBorderRightWidth(float value) {
    if (value != borderRightWidth()) {
        YGNodeStyleSetBorder(d_ptr->m_node, YGEdgeRight, value);
        borderRightWidthChanged();
    }
}

QString Flexbox::alignItems() {
    return d_ptr->m_alignItems;
}

void Flexbox::setAlignItems(const QString& value) {
    if (value != d_ptr->m_alignItems) {
        YGNodeStyleSetAlignItems(d_ptr->m_node, alignByString[value]);
        alignItemsChanged();
    }
}

QString Flexbox::alignContent() {
    return d_ptr->m_alignContent;
}

void Flexbox::setAlignContent(const QString& value) {
    if (value != d_ptr->m_alignContent) {
        YGNodeStyleSetAlignContent(d_ptr->m_node, alignByString[value]);
        alignContentChanged();
    }
}

QString Flexbox::alignSelf() {
    return d_ptr->m_alignSelf;
}

void Flexbox::setAlignSelf(const QString& value) {
    if (value != d_ptr->m_alignSelf) {
        YGNodeStyleSetAlignSelf(d_ptr->m_node, alignByString[value]);
        alignSelfChanged();
    }
}

float Flexbox::aspectRatio() {
    return YGNodeStyleGetAspectRatio(d_ptr->m_node);
}

void Flexbox::setAspectRatio(float value) {
    if (value != aspectRatio()) {
        YGNodeStyleSetAspectRatio(d_ptr->m_node, value);
        aspectRatioChanged();
    }
}

float Flexbox::top() {
    return YGNodeLayoutGetTop(d_ptr->m_node);
}

float Flexbox::bottom() {
    return YGNodeLayoutGetBottom(d_ptr->m_node);
}

float Flexbox::left() {
    return YGNodeLayoutGetLeft(d_ptr->m_node);
}

float Flexbox::right() {
    return YGNodeLayoutGetRight(d_ptr->m_node);
}

float Flexbox::minWidth() {
    return YGNodeStyleGetMinWidth(d_ptr->m_node).value;
}

void Flexbox::setMinWidth(float value) {
    if (value != minWidth()) {
        YGNodeStyleSetMinWidth(d_ptr->m_node, value);
        minWidthChanged();
    }
}

float Flexbox::minHeight() {
    return YGNodeStyleGetMinHeight(d_ptr->m_node).value;
}

void Flexbox::setMinHeight(float value) {
    if (value != minHeight()) {
        YGNodeStyleSetMinHeight(d_ptr->m_node, value);
        minHeightChanged();
    }
}

float Flexbox::maxWidth() {
    return YGNodeStyleGetMaxWidth(d_ptr->m_node).value;
}

void Flexbox::setMaxWidth(float value) {
    if (value != maxWidth()) {
        YGNodeStyleSetMaxWidth(d_ptr->m_node, value);
        maxWidthChanged();
    }
}

float Flexbox::maxHeight() {
    return YGNodeStyleGetMaxHeight(d_ptr->m_node).value;
}

void Flexbox::setMaxHeight(float value) {
    if (value != maxHeight()) {
        YGNodeStyleSetMaxHeight(d_ptr->m_node, value);
        maxHeightChanged();
    }
}

float Flexbox::flex() {
    return YGNodeStyleGetFlex(d_ptr->m_node);
}

void Flexbox::setFlex(float value) {
    if (value != flex()) {
        YGNodeStyleSetFlex(d_ptr->m_node, value);
        flexChanged();
    }
}

float Flexbox::flexBasis() {
    return YGNodeStyleGetFlexBasis(d_ptr->m_node).value;
}

void Flexbox::setFlexBasis(float value) {
    if (value != flexBasis()) {
        YGNodeStyleSetFlexBasis(d_ptr->m_node, value);
        flexBasisChanged();
    }
}

float Flexbox::flexGrow() {
    return YGNodeStyleGetFlexGrow(d_ptr->m_node);
}

void Flexbox::setFlexGrow(float value) {
    if (value != flexGrow()) {
        YGNodeStyleSetFlexGrow(d_ptr->m_node, value);
        flexGrowChanged();
    }
}

float Flexbox::flexShrink() {
    return YGNodeStyleGetFlexShrink(d_ptr->m_node);
}

void Flexbox::setFlexShrink(float value) {
    if (value != flexShrink()) {
        YGNodeStyleSetFlexShrink(d_ptr->m_node, value);
        flexShrinkChanged();
    }
}

QString Flexbox::flexWrap() {
    return d_ptr->m_flexWrap;
}

void Flexbox::setFlexWrap(const QString& value) {
    if (value != d_ptr->m_flexWrap) {
        YGNodeStyleSetFlexWrap(d_ptr->m_node, wrapByString[value]);
        flexWrapChanged();
    }
}

float Flexbox::padding() {
    return YGNodeStyleGetPadding(d_ptr->m_node, YGEdgeAll).value;
}

void Flexbox::setPadding(float value) {
    if (value != padding()) {
        YGNodeStyleSetPadding(d_ptr->m_node, YGEdgeAll, value);
        paddingChanged();
    }
}

float Flexbox::paddingTop() {
    return YGNodeStyleGetPadding(d_ptr->m_node, YGEdgeTop).value;
}

void Flexbox::setPaddingTop(float value) {
    if (value != paddingTop()) {
        YGNodeStyleSetPadding(d_ptr->m_node, YGEdgeTop, value);
        paddingTopChanged();
    }
}

float Flexbox::paddingBottom() {
    return YGNodeStyleGetPadding(d_ptr->m_node, YGEdgeBottom).value;
}

void Flexbox::setPaddingBottom(float value) {
    if (value != paddingBottom()) {
        YGNodeStyleSetPadding(d_ptr->m_node, YGEdgeBottom, value);
        paddingBottomChanged();
    }
}

float Flexbox::paddingLeft() {
    return YGNodeStyleGetPadding(d_ptr->m_node, YGEdgeLeft).value;
}

void Flexbox::setPaddingLeft(float value) {
    if (value != paddingLeft()) {
        YGNodeStyleSetPadding(d_ptr->m_node, YGEdgeLeft, value);
        paddingLeftChanged();
    }
}

float Flexbox::paddingRight() {
    return YGNodeStyleGetPadding(d_ptr->m_node, YGEdgeRight).value;
}

void Flexbox::setPaddingRight(float value) {
    if (value != paddingRight()) {
        YGNodeStyleSetPadding(d_ptr->m_node, YGEdgeRight, value);
        paddingRightChanged();
    }
}

float Flexbox::paddingHorizontal() {
    return YGNodeStyleGetPadding(d_ptr->m_node, YGEdgeHorizontal).value;
}

void Flexbox::setPaddingHorizontal(float value) {
    if (value != paddingHorizontal()) {
        YGNodeStyleSetPadding(d_ptr->m_node, YGEdgeHorizontal, value);
        paddingHorizontalChanged();
    }
}

float Flexbox::paddingVertical() {
    return YGNodeStyleGetPadding(d_ptr->m_node, YGEdgeVertical).value;
}

void Flexbox::setPaddingVertical(float value) {
    if (value != paddingVertical()) {
        YGNodeStyleSetPadding(d_ptr->m_node, YGEdgeVertical, value);
        paddingVerticalChanged();
    }
}

QString Flexbox::display() {
    return d_ptr->m_display;
}

void Flexbox::setDisplay(const QString& value) {
    if (value != d_ptr->m_display) {
        YGNodeStyleSetDisplay(d_ptr->m_node, displayByString[value]);
        displayChanged();
    }
}

QString Flexbox::overflow() {
    return d_ptr->m_overflow;
}

void Flexbox::setOverflow(const QString& value) {
    if (value != d_ptr->m_overflow) {
        YGNodeStyleSetOverflow(d_ptr->m_node, overflowByString[value]);
        overflowChanged();
    }
}

QString Flexbox::position() {
    return d_ptr->m_position;
}

void Flexbox::setPosition(const QString& value) {
    if (value != d_ptr->m_position) {
        YGNodeStyleSetPositionType(d_ptr->m_node, positionByString[value]);
        positionChanged();
    }
}

QString Flexbox::direction() {
    d_ptr->m_direction;
}

void Flexbox::setDirection(const QString& value) {
    if (value != d_ptr->m_direction) {
        YGNodeStyleSetDirection(d_ptr->m_node, directionByString[value]);
        directionChanged();
    }
}

void printYGNode(YGNodeRef node, const QString& nodeName) {
    qDebug() << nodeName << ": width: " << YGNodeLayoutGetWidth(node) << " height: " << YGNodeLayoutGetHeight(node)
             << " x: " << YGNodeLayoutGetLeft(node) << " y: " << YGNodeLayoutGetTop(node);
}

#include "flexbox.moc"
