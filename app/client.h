#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QTcpSocket>
#include "switch.h"
#include "device.h"

class Client : public QDialog
{
    Q_OBJECT

public:
    Client(class QGridLayout *layout = 0, QString *name = 0, int devType = 0,
           int pos = 0, QString *addr = 0, int port = 0);

    QString *getAddr();
    int getPort();

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
    union status status;

    QString *name;
    int devType;
    int pos;
    QString *addr;
    int port;

    QTcpSocket *tcpSocket;
};

#endif
