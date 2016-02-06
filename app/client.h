#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QTcpSocket>
#include "switch.h"
#include "device.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QSlider;
class QPushButton;
class QTcpSocket;
class QNetworkSession;
QT_END_NAMESPACE

class Client : public QDialog
{
    Q_OBJECT

public:
    Client(QWidget *parent = 0, int pos = 0, QString *serverAddr = 0, int serverPort = 0);

private slots:
    void readResp();

    void socketError(QAbstractSocket::SocketError error);
    void socketStateChanged(QAbstractSocket::SocketState tate);

    void enable();
    void disable();

    void requestStatus();
    void switchToggled();
    void tempThresChanged();

    void sendCmd(int cmd, int *data);
    void timerEvent(QTimerEvent *e);

private:
    QLabel *tempThresLabel;
    QLabel *tempThresValueLabel;
    QSlider *tempThresSlider;
    QLabel *tempLabel;
    QPushButton *switchButton;
    QDialogButtonBox *buttonBox;

    qint16 currentTemp;
    qint16 tempThres;
    quint16 blockSize;

    int statusTimer;
    struct status status;

    class QString *serverAddr;
    int serverPort;

    QTcpSocket *tcpSocket;
};

#endif
