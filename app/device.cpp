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
    udpSocket->bind(QHostAddress::AnyIPv4, MULTICAST_PORT, QUdpSocket::ShareAddress);
    udpSocket->joinMulticastGroup(groupAddr);

    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

    sendMulticastMsg("discover");
    qDebug() << "sent discover, waiting answer";

    mainLayout = new QGridLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    qDebug() << QDesktopWidget().availableGeometry(this).size();
    //resize(QDesktopWidget().availableGeometry(this).size());
}

class QGridLayout *Device::getLayout(void)
{
    return mainLayout;
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
        class QString *serverAddr, *name;
        class Client *client;
        int serverPort, busy = 0;

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
        it++;
        name = new QString(*it);

        for (int i = 0; i < clientList.size(); i++) {
               class Client *c = clientList.at(i);
               if (c->getAddr() == serverAddr && c->getPort() == serverPort)
                    busy = 1;
        }
        if (busy) {
            qDebug() << "busy" << serverAddr->toStdString().c_str() << ":" << serverPort;
            continue;
        }

        qDebug() << "radiator" << serverAddr->toStdString().c_str() << ":" << serverPort << ":" << name;

        client = new Client(mainLayout, name, clientList.size(), serverAddr, serverPort);
        setLayout(mainLayout);

        clientList.append(client);
   }
}
