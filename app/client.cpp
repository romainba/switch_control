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
#include <QTime>

#include "client.h"
#include "switch.h"
#include "ui_client.h"

#if 1
#define ENDIAN QDataStream::BigEndian
#else
#define ENDIAN QDataStream::LittleEndian
#endif

#define STATUSTIMEOUT 5

Client::Client(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Client)
{
    /*
     * Send broadcast discover message in order to receive back the IP address
     * from all the servers.
     */

    groupAddr = QHostAddress(MULTICAST_ADDR);

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress::AnyIPv4, MULTICAST_PORT + 1, QUdpSocket::ShareAddress);
    udpSocket->joinMulticastGroup(groupAddr);

    serverAddr = NULL;

    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

    sendMulticastMsg("discover");
    qDebug() << "sent discover, waiting answer";

    /*
     * Initializing the user interface
     */

    ui->setupUi(this);

    ui->tempThresSlider->setMinimum(15);
    ui->tempThresSlider->setMaximum(28);
    ui->tempThresSlider->setValue(0);

    ui->tempThresValue = new QLabel(tr("-"));

    ui->switchButton->setCheckable(false);
    ui->switchButton->setEnabled(false);

    connect(ui->tempThresSlider, SIGNAL(valueChanged(int)),
            this, SLOT(tempThresChanged()));
    connect(ui->switchButton, SIGNAL(clicked()), this, SLOT(switchToggled()));

    statusTimer = 0;
}

Client::~Client()
{
    delete ui;
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

void delay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}

void Client::sendCmd(int cmd, int *data)
{
    if (cmd >= CMD_NUM)
        return;

    struct cmd c;
    c.header.id = cmd;
    c.header.len = cmdDataSize[cmd];
    memcpy(&c.u, data, cmdDataSize[cmd]);

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
    qDebug() << "readReady" << tcpSocket->bytesAvailable();

    if (tcpSocket->bytesAvailable() < (int)(sizeof(struct resp_header)))
        return;

    QDataStream in(tcpSocket);
    struct resp resp;

    in.setVersion(QDataStream::Qt_4_0);
    in.setByteOrder(ENDIAN);

    in >> resp.header;

    qDebug() << "resp" << resp.header.id <<
                "status" << resp.header.status <<
                "len" << resp.header.len;

    if (tcpSocket->bytesAvailable() < resp.header.len)
        return;

    in >> resp;

    if (resp.header.len != respDataSize[resp.header.id]) {
        qDebug() << "Cmd" << resp.header.id << "with invalid len " << resp.header.len;
        return;
    }

    switch (resp.header.id) {
    case CMD_GET_STATUS: {
        struct status *s = &resp.u.status;

        memcpy(&status, s, sizeof(struct status));

        QString str = s->sw_pos ? "ON" : "OFF";
        ui->statusLabel->setText( str + " " + QString::number(s->temp / 1000.0, 'f', 1) + "°C");

        ui->tempThresSlider->setValue(s->tempThres/1000.0);
        qDebug() << "  temp" << s->temp / 1000.0 << "thres" << s->tempThres / 1000.0;
        ui->switchButton->setText(s->sw_pos ? "Switch OFF" : "Switch ON");

        if (!ui->switchButton->isEnabled()) {
                qDebug() << "enabled with switch" << s->sw_pos;
                enable();
        }
        break;
    }
    }

}

void Client::requestStatus()
{
    sendCmd(CMD_GET_STATUS, NULL);
}

void Client::tempThresChanged()
{
    int temp = ui->tempThresSlider->value();

    ui->tempThresValue->setText(QString::number(temp) + "°C");
}

void Client::switchToggled()
{
    status.sw_pos = !status.sw_pos;

    sendCmd(CMD_SET_SW_POS, &status.sw_pos);

    ui->switchButton->setText(status.sw_pos ? "Switch OFF" : "Switch ON");
}

void Client::enable()
{
    requestStatus();
    statusTimer = startTimer(STATUSTIMEOUT * 1000);
    ui->switchButton->setEnabled(1);
    qDebug() << "enable";
}

void Client::disable()
{
    qDebug() << "disable";
    if (statusTimer) {
        killTimer(statusTimer);
        statusTimer = 0;
    }
    ui->switchButton->setEnabled(0);
    ui->statusLabel->setText("-");
    ui->tempThresValue->setText("-");

    tcpSocket->close();
}

void Client::socketStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "tcpSocket" << state;
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        tcpSocket->abort();
        tcpSocket->close();

        delay(1000);

        qDebug() << "reconnecting";
        tcpSocket->connectToHost(serverAddr->toStdString().c_str(), serverPort);
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
    qDebug() << "socketerror" << error;
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
    tcpSocket->abort();
    tcpSocket->close();

    delay(1000);
    qDebug() << "reconnecting";
    tcpSocket->connectToHost(serverAddr->toStdString().c_str(), serverPort);
}

void Client::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == statusTimer) {

        int t = status.tempThres / 1000;
        if (t != ui->tempThresSlider->value()) {
            status.tempThres = ui->tempThresSlider->value() * 1000;
            sendCmd(CMD_SET_TEMP, &status.tempThres);
        } else
            requestStatus();

        e->accept();
    }
}

void Client::sendMulticastMsg(QByteArray datagram)
{
    QUdpSocket *s = new QUdpSocket(this);

    s->setSocketOption(QAbstractSocket::MulticastTtlOption, 2);

    s->writeDatagram(datagram.data(), datagram.size(), groupAddr, MULTICAST_PORT);
    qDebug() << "sent multicast" << groupAddr << "port" << MULTICAST_PORT;
    s->close();
}

void Client::processPendingDatagrams()
{
    //qDebug() << "upd recv msg";
    if (serverAddr) {
            qDebug() << "skip udp msg";
            return;
    }

    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
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

        /* connect socket to the server */
        tcpSocket = new QTcpSocket(this);
        connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readResp()));
        connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(socketError(QAbstractSocket::SocketError)));
        connect(tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));

        tcpSocket->connectToHost(serverAddr->toStdString().c_str(), serverPort);

        break;
    }
}
