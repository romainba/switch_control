#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QTcpSocket>
#include <QHostAddress>
#include "switch.h"

class Client
{
    Q_OBJECT

public:
    Client(class Device *device = 0, int num = 0, QString *_serverAddr = NULL,
           int _serverPort = 0);

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
    class Device *device;

    class QLabel *tempThresLabel;
    class QLabel *tempThresValueLabel;
    class QSlider *tempThresSlider;
    class QLabel *tempLabel;
    class QPushButton *switchButton;
    class QDialogButtonBox *buttonBox;

    qint16 currentTemp;
    qint16 tempThres;
    quint16 blockSize;

    int statusTimer;
    struct status status;

    class QString *serverAddr;
    int serverPort;

    class QTcpSocket *tcpSocket;
    class QNetworkSession *networkSession;
};

#endif
