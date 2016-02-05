#include <QtWidgets>

#include "device.h"
#include "client.h"

Device::Device(QWidget *parent)
	:   QDialog(parent)
{
    num_clients = 0;
    setWindowTitle(tr(APP_NAME));

    /*
     * Send broadcast discover message in order to receive back the IP address
     * from all the servers.
     */

    groupAddr = QHostAddress(MULTICAST_ADDR);

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::AnyIPv4, MULTICAST_PORT + 1, QUdpSocket::ShareAddress);
    udpSocket->joinMulticastGroup(groupAddr);

    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

    sendMulticastMsg("discover");
    qDebug() << "sent discover, waiting answer";
}

/*
 * Send one multicast message
 */
void Device::sendMulticastMsg(QByteArray datagram)
{
    QUdpSocket *s = new QUdpSocket(this);

    s->setSocketOption(QAbstractSocket::MulticastTtlOption, 2);

    s->writeDatagram(datagram.data(), datagram.size(), groupAddr, MULTICAST_PORT);
    qDebug() << "sent multicast" << groupAddr << "port" << MULTICAST_PORT;
    s->close();
}

/*
 * Multicast answer reception
 */
void Device::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {

        class QByteArray datagram;
        class QString *serverAddr;
        class Client *client;
        int serverPort;

        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());

        qDebug() << "recv:" << datagram.data();
        QString data = datagram.data();
        QStringList list = data.split(":");

        QStringList::iterator it = list.begin();

        if (*it != "radiator") {
            qDebug() << "skipped msg";
            continue;
        }

        it++;
        serverAddr = new QString(*it++);
        serverPort = it->toInt();

        qDebug() << "radiator" << serverAddr->toStdString().c_str() << ":" << serverPort;

        udpSocket->close();

        client = new Client(this, num_clients, serverAddr, serverPort);

        num_clients++;
    }
}
