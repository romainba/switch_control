/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QtNetwork>
#include <QDataStream>

#include "client.h"
#include "switch.h"

#define APP_NAME "Radiateur"

#if 0
#define ENDIAN QDataStream::BigEndian
#else
#define ENDIAN QDataStream::LittleEndian
#endif

#define STATUSTIMEOUT 5

Client::Client(QWidget *parent)
:   QDialog(parent), networkSession(0)
{
    tempThresLabel = new QLabel(tr("Max"));

#if 0
    // find out name of this machine
    QString name = QHostInfo::localHostName();
    if (!name.isEmpty()) {
        hostCombo->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            hostCombo->addItem(name + QChar('.') + domain);
    }
    if (name != QString("localhost"))
        hostCombo->addItem(QString("localhost"));
    // find out IP addresses of this machine
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // add non-localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (!ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }
    // add localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }
#endif

    tempThresSlider = new QSlider(Qt::Horizontal, this);
    tempThresSlider->setMinimum(15);
    tempThresSlider->setMaximum(28);
    tempThresSlider->setValue(0);

    tempThresLabel->setBuddy(tempThresSlider);

    tempLabel = new QLabel(tr("-"));
    tempThresValueLabel = new QLabel(tr("-"));
    tempThresValueLabel->setBuddy(tempThresSlider);

    switchButton = new QPushButton("Switch ON", this);
    switchButton->setCheckable(true);
    switchButton->setEnabled(false);

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(switchButton, QDialogButtonBox::ActionRole);

    tcpSocket = new QTcpSocket(this);

    connect(tempThresSlider, SIGNAL(valueChanged(int)),
            this, SLOT(tempThresChanged()));
    connect(switchButton, SIGNAL(clicked()), this, SLOT(switchToggled()));

    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readResp()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(tempThresLabel, 0, 0);
    mainLayout->addWidget(tempThresSlider, 0, 1);
    mainLayout->addWidget(tempThresValueLabel, 0, 2);
    mainLayout->addWidget(tempLabel, 1, 0, 1, 1);
    mainLayout->addWidget(buttonBox, 1, 1, 1, 3);
    setLayout(mainLayout);

    setWindowTitle(tr(APP_NAME));

    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
            QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, SIGNAL(opened()), this, SLOT(sessionOpened()));

        switchButton->setEnabled(false);

        qDebug() << "Opening network session";

        networkSession->open();
    }

    groupAddr = QHostAddress(MULTICAST_ADDR);
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::AnyIPv4, MULTICAST_PORT, QUdpSocket::ShareAddress);
    udpSocket->joinMulticastGroup(groupAddr);
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

    enableButtons();

    socketBusy = 0;
    statusTimer = 0;
    //requestStatus();
}


QDataStream &operator<<(QDataStream &out, const struct cmd &p)
{
    int *ptr = (int *)&p.u;
    out << p.header.id << p.header.len;
    for (int i = 0; i < p.header.len / 4; i++)
         out << ptr[i];
    return out;
}

QDataStream &operator>>(QDataStream &in, struct resp_header &p)
{
    in >> p.id >> p.status >> p.len;
    return in;
}

QDataStream &operator>>(QDataStream &in, struct resp &p)
{
    int i, *ptr = (int *)&p.u;

    for (i = 0; i < p.header.len / 4; i++)
        in >> ptr[i];
    return in;
}

static int cmdDataSize[CMD_NUM] = {
    [CMD_SET_SW_POS] = sizeof(int),
    [CMD_TOGGLE_MODE] = 0,
    [CMD_SET_TEMP] = sizeof(int),
    [CMD_GET_STATUS] = 0
};

static int respDataSize[CMD_NUM] = {
    [CMD_SET_SW_POS] = 0,
    [CMD_TOGGLE_MODE] = 0,
    [CMD_SET_TEMP] = 0,
    [CMD_GET_STATUS] = sizeof(struct status)
};

void Client::sendCmd(int cmd, int *data)
{
    int cnt = 0;
    QThread *thread = QThread::currentThread();

    if (cmd >= CMD_NUM)
        return;

    while (socketBusy) {
        if (cnt++ % 5)
            qDebug() << "busy";
        thread->msleep(100);
    }
    socketBusy = 1;

    struct cmd c;
    c.header.id = cmd;
    c.header.len = cmdDataSize[cmd];
    memcpy(&c.u, data, cmdDataSize[cmd]);

    if (!tcpSocket->isValid()) {
        tcpSocket->connectToHost(serverAddr->toStdString().c_str(), serverPort);
        if (!tcpSocket->isValid()) {
                tcpSocket->error(QAbstractSocket::HostNotFoundError);
                qDebug() << "connectToHost failed";
                return;
        }
    }

    QDataStream out(tcpSocket);
    out.setByteOrder(ENDIAN);

    out << c;

    if (cmdDataSize[cmd])
        qDebug() << "sendCmd" << cmd << "len" << cmdDataSize[cmd] << "data" << data[0];
    else
        qDebug() << "sendCmd" << cmd;
}

void Client::readResp()
{
    QDataStream in(tcpSocket);
    struct resp resp;

    in.setVersion(QDataStream::Qt_4_0);
    in.setByteOrder(ENDIAN);

    if (tcpSocket->bytesAvailable() < (int)(sizeof(struct resp_header)))
        return;

    in >> resp.header;

    qDebug() << "resp" << resp.header.id <<
                "status" << resp.header.status <<
                "len" << resp.header.len;

    if (tcpSocket->bytesAvailable() < resp.header.len)
        return;

    in >> resp;
    socketBusy = 0;

    if (resp.header.len != respDataSize[resp.header.id]) {
        qDebug() << "Cmd" << resp.header.id << "with invalid len " << resp.header.len;
        return;
    }

    switch (resp.header.id) {
    case CMD_GET_STATUS: {
        struct status *s = &resp.u.status;

        memcpy(&status, s, sizeof(struct status));

        QString str = s->sw_pos ? "ON" : "OFF";
        tempLabel->setText( str + " " + QString::number(s->temp / 1000.0) + "°C");

        tempThresSlider->setValue(s->tempThres/1000.0);
        qDebug() << "  temp" << s->temp / 1000.0 << "thres" << s->tempThres / 1000.0;
        switchButton->setText(s->sw_pos ? "Switch OFF" : "Switch ON");
        break;
    }
    }

    switchButton->setEnabled(true);
}

void Client::requestStatus()
{
    sendCmd(CMD_GET_STATUS, NULL);
}

void Client::tempThresChanged()
{
    int temp = tempThresSlider->value();

    tempThresValueLabel->setText(QString::number(temp) + "°C");
}

void Client::switchToggled()
{
    status.sw_pos = !status.sw_pos;

    sendCmd(CMD_SET_SW_POS, &status.sw_pos);

    switchButton->setText(status.sw_pos ? "Switch OFF" : "Switch ON");
}

void Client::enable()
{
    statusTimer = startTimer(STATUSTIMEOUT * 1000);
    qDebug() << "enable";
}

void Client::disable()
{
    qDebug() << "disable";
    if (statusTimer) {
        killTimer(statusTimer);
        statusTimer = 0;
    }
    switchButton->setEnabled(0);
    tempLabel->setText("-");
    tempThresValueLabel->setText("-");
}

void Client::socketStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        tcpSocket->abort();
        disable();
        break;
    case QAbstractSocket::ConnectedState:
        enable();
        break;
    default:
        break;
    }
}

void Client::socketError(QAbstractSocket::SocketError error)
{
    switch (error) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr(APP_NAME),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr(APP_NAME),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr(APP_NAME),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }
}


void Client::enableButtons()
{
    int flag = ((!networkSession || networkSession->isOpen()));
     switchButton->setEnabled(flag);
}


void Client::sessionOpened()
{
    // Save the used configuration
    QNetworkConfiguration config = networkSession->configuration();
    QString id;
    if (config.type() == QNetworkConfiguration::UserChoice)
        id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
    else
        id = config.identifier();

    QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
    settings.beginGroup(QLatin1String("QtNetwork"));
    settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
    settings.endGroup();

    qDebug() << "switch server should run";

    enableButtons();
}

void Client::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == statusTimer) {

        int t = status.tempThres / 1000;
        if (t != tempThresSlider->value()) {
            status.tempThres = tempThresSlider->value() * 1000;
            sendCmd(CMD_SET_TEMP, &status.tempThres);
        } else
            requestStatus();

        e->accept();
    }
}

void Client::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());

        qDebug() << "multicast:" << datagram.data();
        QString data = datagram.data();
        QStringList list = data.split(":");

        QStringList::iterator it = list.begin();

        if (*it != "radiator")
            continue;

        it++;
        qDebug() << *it;
        serverAddr = new QString(*it++);
        serverPort = it->toInt();

        qDebug() << "radiator" << serverAddr << ":" << serverPort;

        enable();
    }
}
