#include "cameralabel.h"
#include <QPainter>
#include <qdebug.h>
#include <QGuiApplication>
#include <QApplication>
#include <QPicture>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>

CameraLabel::CameraLabel(QWidget *parent) :
    QLabel(parent),
    isOnSnip(false),
    scaleRatio(1.0),
    scaleFactor(-1),
    mouseActionType(None)
{

}
CameraLabel::~CameraLabel()
{

}

void CameraLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    /* draw text */
    drawText(&painter);
    if (samplePixmap.isNull()) {
        return;
    }
    QPixmap tmpPixmap = samplePixmap.copy();
    /* scaling */
    int scaledWidth = scaleRatio * samplePixmap.width();
    int scaledHeight = scaleRatio * samplePixmap.height();
    if (mouseActionType == MouseActionType::Amplification) {
        scaleRatio *= 0.9;
        if (scaleRatio < minRatio) {
            scaleRatio = minRatio;
            if (isminfScale) {
                emit minimizeScale();
                isminfScale = false;
            }
        }
    } else if (mouseActionType == MouseActionType::Shrink) {
        isminfScale = true;
        scaleRatio *= 1.1;
        scaleRatio = scaleRatio > maxRatio ? maxRatio : scaleRatio;
    }

    if (mouseActionType == MouseActionType::Amplification ||
            mouseActionType == MouseActionType::Shrink) {
        scaledWidth = scaleRatio * samplePixmap.width();
        scaledHeight = scaleRatio * samplePixmap.height();
        mouseActionType = MouseActionType::None;
    }
    if (mouseActionType == MouseActionType::Move &&
            scaleRatio > minRatio &&
            scaledWidth > width() &&
            scaledHeight > height()) {
        mouseTotalOffset += mouseMoveOffset;
        mouseActionType = MouseActionType::None;
    }
    tmpPixmap = tmpPixmap.scaled(scaledWidth, scaledHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int distance = width() / 2;
    int distance1 = height() / 2;
    if (scaledWidth > width() && scaledHeight > height()) {
        if (abs(mouseTotalOffset.x()) >= (scaledWidth - width()) / 2) {
            if (mouseTotalOffset.x() > 0)
                mouseTotalOffset.setX(width() / 2 + (scaledWidth - width()) / 2 - distance);
            else
                mouseTotalOffset.setX(-width() / 2 - (scaledWidth - width()) / 2 + distance);
        }
        if (abs(mouseTotalOffset.y()) >= (scaledHeight - height()) / 2)
        {
            if (mouseTotalOffset.y() > 0)
                mouseTotalOffset.setY(height() / 2 + (scaledHeight - height()) / 2 - distance1);
            else
                mouseTotalOffset.setY(-height() / 2 -(scaledHeight - height()) / 2 + distance1);
        }
    }

    int x = (width() - scaledWidth ) / 2 + mouseTotalOffset.x();
    x = x < 0 ? 0:x;
    int y = (height() - scaledHeight) / 2 + mouseTotalOffset.y();
    y = y < 0 ? 0:y;

    int  moveOffsetX = (scaledWidth - width()) / 2 - mouseTotalOffset.x();
    moveOffsetX = moveOffsetX < 0 ? 0:moveOffsetX;
    int  moveOffsetY = (scaledHeight - height()) / 2 - mouseTotalOffset.y();
    moveOffsetY = moveOffsetY < 0 ? 0:moveOffsetY;

    int w = (scaledWidth - moveOffsetX)>width() ? width() : (scaledWidth - moveOffsetX);
    if (w >(width() - x)) {
        w = width() - x;
    }
    int h = (scaledHeight - moveOffsetY)>height() ? height() : (scaledHeight - moveOffsetY);
    if (h > (height() - y)) {
        h = height() - y;
    }
    painter.drawPixmap(0, 0, width(), height(), tmpPixmap, moveOffsetX, moveOffsetY, w, h);
    /* snip */
    if (isOnSnip == true) {
        QPen pen;
        pen.setBrush(Qt::red);
        pen.setStyle(Qt::DashLine);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawRect(snipRect.getRect(1));
        SnipRect::State state = snipRect.state();
        if (state == SnipRect::ReadySnip) {
            double ratio = double(samplePixmap.height()) / double(scaledHeight);
            SnipRect tmpRect = snipRect;
            tmpRect.move(QPoint(moveOffsetX, moveOffsetY));
            QPixmap snipPixmap = samplePixmap.copy(tmpRect.getRect(ratio));
            emit snipFinished(snipPixmap);
            snipRect.setState(SnipRect::WaitSnip);
        }
    }
    return;
}

void CameraLabel::drawText(QPainter *painter)
{
    painter->save();
    QString labelText = this->text();
    if(labelText.isEmpty() == false) {
        painter->setPen(QPen(Qt::red));
        QRect rcDraw(0, 0, rect().width(), rect().height());
        painter->drawText(rcDraw, Qt::AlignCenter, labelText);
    }
    painter->restore();
    return;
}

void CameraLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (isOnSnip == true) {
        snipRect.setSecondPoint(event->pos());
        snipRect.setState(SnipRect::Sliding);
    } else if (isMousePressed && isTwoPoint == false) {
        mouseMoveOffset = event->pos() - mousePreviousPos;
        mousePreviousPos = event->pos();
        mouseActionType = MouseActionType::Move;
        this->update();
    }

    return QWidget::mouseMoveEvent(event);
}

void CameraLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton &&
             isMousePressed &&
             isTwoPoint ==false) {
        QApplication::setOverrideCursor(Qt::ArrowCursor);
        isMousePressed = false;
        snipRect.setSecondPoint(event->pos());
    }
    isTwoPoint = false;
    return QWidget::mouseReleaseEvent(event);
}

void CameraLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isOnSnip == true) {
        snipRect.reset();
        snipRect.setFirstPoint(event->pos());
    } else if (event->button() == Qt::LeftButton && isTwoPoint == false) {
        isMousePressed = true;
        QApplication::setOverrideCursor(Qt::OpenHandCursor);
        mousePreviousPos = event->pos();
    }
    return QWidget::mousePressEvent(event);
}

void CameraLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    Qt::MouseButton button = event->button();
    if (button == Qt::LeftButton) {
        emit leftMouseDoubleClicked();
    } else if (button == Qt::RightButton) {
        adjust();
    }
    return QWidget::mouseDoubleClickEvent(event);
}

void CameraLabel::keyPressEvent(QKeyEvent *ev)
{
    switch (ev->key()) {
    case Qt::Key_F1:
        setSnip(!isOnSnip);
         break;
    case Qt::Key_1:
    case Qt::Key_Enter:
        if (snipRect.state() == SnipRect::Sliding) {
            snipRect.setState(SnipRect::ReadySnip);
            update();
        }
        break;
    default:
        break;
    }
    return QWidget::keyPressEvent(ev);
}

void CameraLabel::resizeEvent(QResizeEvent *event)
{
    reset();
    return QWidget::resizeEvent(event);
}

void CameraLabel::zoom(int mode)
{
    if (mode == 0) {
        mouseActionType = Amplification;
    } else {
        mouseActionType = Shrink;
    }
    this->update();
    return;
}

void CameraLabel::wheelEvent(QWheelEvent* event)
{
    if (event->delta() > 0) {
        mouseActionType = Shrink;
    } else {
        mouseActionType = Amplification;
    }
    this->update();
    event->accept();
    return QWidget::wheelEvent(event);
}

/* touch event */
bool CameraLabel::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
        if (touchPoints.count() >= 2) {
            isTwoPoint = true;
            const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();
            const QTouchEvent::TouchPoint &touchPoint1 = touchPoints.last();
            double currentScaleFactor =  QLineF(touchPoint0.pos(), touchPoint1.pos()).length();
            int len = 3;
            if (scaleFactor < 0) {
                scaleFactor = currentScaleFactor;
                return true;
            }

            if (touchEvent->touchPointStates() & Qt::TouchPointReleased) {
                scaleFactor *= currentScaleFactor;
            }
            if (currentScaleFactor < (scaleFactor - len)) {
                mouseActionType = Amplification;
                this->update();
            }  else if (currentScaleFactor > (scaleFactor + len)) {
                mouseActionType = Shrink;
                this->update();
            }
            scaleFactor = currentScaleFactor;
        } else if (touchPoints.count() == 1) {
            if (isTwoPoint == true) {
                scaleFactor = -1;
                isTwoPoint = false;
            }
        }
        return true;
    }
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

void CameraLabel::adapt(const QPixmap &pixmap)
{
    int pHeight = parentWidget()->height();
    int imgWidth = pixmap.width() * double(pHeight) / double(pixmap.height());
    resize(imgWidth, pHeight);
    return;
}

void CameraLabel::updatePixmap(const QPixmap &image)
{
    if (image.isNull()) {
        return;
    }
    /* adapt image and align */
    adapt(image);
    if (samplePixmap.width() != image.width() &&
            samplePixmap.height() != image.height()) {
        scaleRatio = 1.0;
    }
    samplePixmap = image;
    if (scaleRatio == 1.0) {
        scaleRatio = 1.1 * double(height() - 80) / samplePixmap.height();
        minRatio = scaleRatio;
        maxRatio = scaleRatio;
        int num = 0;
        minRatio = scaleRatio;
        while (--num >= 0) {
           minRatio *= 0.9;
        }
        maxRatio = scaleRatio;
        num = 10;
        while (--num >= 0) {
           maxRatio *= 1.1;
        }
        mouseTotalOffset.setX(0);
        mouseTotalOffset.setY(0);
    }
    update();
}

void CameraLabel::snip()
{
    snipRect.setState(SnipRect::ReadySnip);
}

void CameraLabel::reset()
{
    scaleRatio = 1.0;
    samplePixmap = QPixmap();
}

void CameraLabel::setSnip(bool on)
{
    isOnSnip = on;
    snipRect.reset();
    update();
    return;
}

void CameraLabel::adjust()
{
    isTwoPoint = false;
    scaleRatio = 1.1 * double(height()) / double(samplePixmap.height());
    int num = 0;
    minRatio = scaleRatio;
    while (--num >= 0) {
       minRatio *= 0.9;
    }
    maxRatio = scaleRatio;
    num = 10;
    while (--num >= 0) {
       maxRatio *= 1.1;
    }
    mouseTotalOffset.setX(0);
    mouseTotalOffset.setY(0);
}
