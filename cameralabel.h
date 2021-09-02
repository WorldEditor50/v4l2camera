#ifndef DLG_CAMERA_H
#define DLG_CAMERA_H

#include <QWidget>
#include <QDesktopWidget>
#include <QScreen>
#include <QTimer>
#include <QString>
#include <QLabel>
#include <QDateTime>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QLibrary>

class SnipRect
{
public:
    enum State {
        NotSetPoint = 0,
        StartSnip,
        Sliding,
        ReadySnip,
        WaitSnip
    };
private:
    QPoint firstPoint;
    QPoint secondPoint;
    QRect rect;
    State state_;
public:
    SnipRect()
    {
        reset();
    }
    SnipRect(const SnipRect &r)
    {
        firstPoint = r.firstPoint;
        secondPoint = r.secondPoint;
        rect = r.rect;
        state_ = r.state_;
    }

    SnipRect& operator = (const SnipRect &r)
    {
        if (this == &r) {
            return *this;
        }
        firstPoint = r.firstPoint;
        secondPoint = r.secondPoint;
        rect = r.rect;
        state_ = r.state_;
        return *this;
    }

    ~SnipRect(){}

    State state()
    {
        return state_;
    }
    void setState(State s)
    {
        state_ = s;
    }

    void move(const QPoint &offset)
    {
        firstPoint += offset;
        secondPoint += offset;
        return;
    }

    QRect getRect(double r)
    {
        rect = QRect(QPoint(0, 0), QSize(0, 0));
        firstPoint *= r;
        secondPoint *= r;
        int w = firstPoint.x() - secondPoint.x();
        int h = firstPoint.y() - secondPoint.y();
        if (w < 0 &&  h < 0) {
            /* top-left to buttom-right */
            rect = QRect(firstPoint, secondPoint);
        } else if (w > 0 && h > 0) {
            /* buttom-right to top-left */
            rect = QRect(secondPoint, firstPoint);
        } else if (w > 0 && h < 0) {
            /* top-right to buttom-left */
            rect = QRect(secondPoint.x(), firstPoint.y(), w, -h);
        } else if (w < 0 && h > 0) {
            /* buttom-left to top-right */
            rect = QRect(firstPoint.x(), secondPoint.y(), -w, h);
        } else {

        }
        return rect;
    }

    void setFirstPoint(const QPoint &s)
    {
        state_ = StartSnip;
        firstPoint = s;
        secondPoint = s;
        return;
    }

    void setSecondPoint(const QPoint &e)
    {
        secondPoint = e;
        return;
    }

    void reset()
    {
        QPoint point(0, 0);
        firstPoint = point;
        secondPoint = point;
        rect = QRect(point, QSize(0, 0));
        state_ = NotSetPoint;
        return;
    }
};

class CameraLabel : public QLabel
{
    Q_OBJECT
public:
    enum MouseActionType {
        None = 0,
        Amplification,
        Shrink,
        Lift,
        Right,
        Up,
        Down,
        Move
    };
public:
    explicit CameraLabel(QWidget *parent = nullptr);
    ~CameraLabel();
signals:
    void leftMouseDoubleClicked();
    void minimizeScale();
    void snipFinished(const QPixmap &snipPixmap);
public slots:
    void reset();
    void setSnip(bool on);
    void updatePixmap(const QPixmap &pixmap);
    void snip();
    void adjust();
    void zoom(int mode);
protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void resizeEvent(QResizeEvent *event) override;
    /* draw and scale */
    void paintEvent(QPaintEvent *event) override;
    void drawText(QPainter *painter);
    bool event(QEvent *event) override;
private:
    void adapt(const QPixmap &pixmap);
private:
    QPixmap samplePixmap;
    /* snip */
    bool isOnSnip;
    SnipRect snipRect;
    /* scale */
    bool isminfScale;
    double scaleRatio;
    double scaleFactor;
    double maxRatio;
    double minRatio;
    /* mouse action */
    MouseActionType mouseActionType;
    QPoint mouseMoveOffset;
    QPoint mousePreviousPos;
    QPoint mouseTotalOffset;
    bool isMousePressed;
    bool isTwoPoint;
};

#endif // DLG_CAMERA_H
