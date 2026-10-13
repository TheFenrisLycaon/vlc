#ifndef HEIMDALL_HOTKEY_CONVERTER_HPP
#define HEIMDALL_HOTKEY_CONVERTER_HPP

#include <QObject>
#include <QPoint>
#include <QtQml/qqmlregistration.h>

/* Heimdall Key/Wheel hotkeys interactions */

Q_MOC_INCLUDE("QKeyEvent")
Q_MOC_INCLUDE("QWheelEvent")

class QKeyEvent;
class QWheelEvent;
class QInputEvent;

int qtKeyModifiersToHeimdall( const QInputEvent& e );
int qtEventToHeimdallKey( QKeyEvent *e );
int qtWheelEventToHeimdallKey( const QWheelEvent& e );
QString HeimdallKeyToString( unsigned val, bool );

/**
 * @brief The WheelToHeimdallConverter class aggregates wheel events and
 * emit a signal once it gathers a full scroll step, as Heimdall doesn't handle
 * fractionnal scroll events
 */
class WheelToHeimdallConverter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    using QObject::QObject;

public:
    Q_INVOKABLE Qt::Orientations getWheelOrientation(int x, int y);

signals:
    /**
     * @param heimdallKey the Heimdall hotkey representation
     */
    void heimdallWheelKey(int heimdallKey);
    /**
     * @param steps: positive value indicated UP wheel events, negative DOWN wheel event
     * @param modifiers are keyboard pressed modifiers
     */
    void wheelUpDown(int steps, Qt::KeyboardModifiers modifiers);
    /**
     * @param steps: positive value indicated UP wheel events, negative DOWN wheel event
     * @param modifiers are keyboard pressed modifiers
     */
    void wheelLeftRight(int steps, Qt::KeyboardModifiers modifiers);

public slots:
    /**
     * @brief qmlWheelEvent handles wheel events as emitted by QWidget
     * @param e the wheel event
     */
    void wheelEvent(const QWheelEvent* e);

    /**
     * @brief qmlWheelEvent handles wheel events as emitted by QML
     * @param wheelEvent qml wheel event
     */
    void qmlWheelEvent(const QObject* wheelEvent);

    void customWheelEvent(const QPoint& pixelDelta,
                          const QPoint& angleDelta,
                          const Qt::MouseButtons buttons = {},
                          const Qt::KeyboardModifiers modifiers = {},
                          const bool inverted = false);

private:
    QPoint m_scrollAmount = {};
    Qt::KeyboardModifiers m_modifiers = {};
    Qt::MouseButtons m_buttons = {};
};

#endif // HEIMDALL_HOTKEY_CONVERTER_HPP
