#ifndef DEVICE_H
#define DEVICE_H

#include <QDialog>
#include <QUdpSocket>
#include <QHostAddress>

class Device : public QDialog
{
    Q_OBJECT

public:
    Device(QWidget *parent = 0);
    class QGridLayout *getLayout(void);

private slots:
    void processPendingDatagrams();
    void sendMulticastMsg(QByteArray datagram);

private:
    class QUdpSocket *udpSocket;
    class QHostAddress groupAddr;
    int num_clients;
    class QGridLayout *mainLayout;
};

#endif // DEVICE_H
