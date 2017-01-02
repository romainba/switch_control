#include <QtWidgets>

#include "device.h"
#include "client.h"

#define DEVICETIMEOUT 10 /* sec */

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

    mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    qDebug() << QDesktopWidget().availableGeometry(this).size();
    //resize(QDesktopWidget().availableGeometry(this).size());

    sendMulticastMsg("discover");
    statusTimer = startTimer(DEVICETIMEOUT * 1000);
}

//class QGridLayout *Device::getLayout(void)
//{
//    return mainLayout;
//}

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

void Device::deleteClient(int pos)
{
    class Client *client = clientList.at(pos);
    QGroupBox *box = client->box;

    qDebug() << "dev: delete Client" << pos << *client->getAddr() << ":" << client->getPort();

    mainLayout->removeWidget(box);

    setLayout(mainLayout);

    clientList.removeAt(pos);
    client->deleteLater();
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
        int serverPort, already = 0, devType;

        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());

        qDebug() << "recv:" << datagram.data();
        QString data = datagram.data();
        QStringList list = data.split(":");

        QStringList::iterator it = list.begin();

        for (devType = 0; devType < NUM_DEV_TYPE; devType++) {
            if (*it == QString(dev_type_name[devType]))
                break;
        }
        if (devType == NUM_DEV_TYPE) {
            qDebug() << "device not supported";
            continue;
        }

        it++;
        serverAddr = new QString(*it++);
        serverPort = it->toInt();
        it++;

        foreach(class Client *c, clientList) {
            if (*c->getAddr() == *serverAddr && c->getPort() == serverPort) {
                 already = 1;
                 break;
            }
        }
        if (already) {
            qDebug() << "already" << *serverAddr + ":" + QString::number(serverPort);
            delete serverAddr;
            continue;
        }

        name = new QString(*it);
        qDebug() << "radiator" << *serverAddr + ":" + QString::number(serverPort) << *name;

        QGroupBox *box = new QGroupBox(*name + " - " + *serverAddr + ":" + QString::number(serverPort));
        client = new Client(box, devType, clientList.size(), serverAddr, serverPort);
        mainLayout->addWidget(box);

        setLayout(mainLayout);

        clientList.append(client);
        connect(client, SIGNAL(deleteRequest(int)), this, SLOT(deleteClient(int)));
   }
}

void Device::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == statusTimer) {
        sendMulticastMsg("discover");
        qDebug() << "sent discover, waiting answer";
    }
}
