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

Client::Client(QWidget *parent)
:   QDialog(parent), networkSession(0)
{
    hostLabel = new QLabel(tr("&Server name:"));
    portLabel = new QLabel(tr("S&erver port:"));

    hostCombo = new QComboBox;
    hostCombo->setEditable(true);
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

    portLineEdit = new QLineEdit;
    portLineEdit->setValidator(new QIntValidator(1, 65535, this));
    portLineEdit->setText(QString::number(PORT));

    hostLabel->setBuddy(hostCombo);
    portLabel->setBuddy(portLineEdit);

    tempLabel = new QLabel(tr("None"));

    getTempButton = new QPushButton(tr("Get temp"));
    getTempButton->setDefault(true);
    getTempButton->setEnabled(false);

    switchButton = new QPushButton("Switch ON", this);
    switchButton->setCheckable(true);
    switchButton->setEnabled(false);

    quitButton = new QPushButton(tr("Quit"));

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(getTempButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(switchButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    tcpSocket = new QTcpSocket(this);

    connect(hostCombo, SIGNAL(editTextChanged(QString)),
            this, SLOT(enableButtons()));
    connect(portLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(enableButtons()));
    connect(getTempButton, SIGNAL(clicked()),
            this, SLOT(requestTemp()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(switchButton, SIGNAL(clicked()), this, SLOT(switchToggled()));

    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readResp()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostCombo, 0, 1);
    mainLayout->addWidget(portLabel, 1, 0);
    mainLayout->addWidget(portLineEdit, 1, 1);
    mainLayout->addWidget(tempLabel, 2, 0, 1, 1);
    mainLayout->addWidget(buttonBox, 2, 1, 1, 3);
    setLayout(mainLayout);

    setWindowTitle(tr("Switch client"));
    portLineEdit->setFocus();

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

        getTempButton->setEnabled(false);
        switchButton->setEnabled(false);

        qDebug() << "Opening network session";

        networkSession->open();
    }
}

QDataStream &operator<<(QDataStream &out, const struct cmd_header &p)
{
    out << (qint32)p.id << (qint32)p.len;
    return out;
}

QDataStream &operator>>(QDataStream &in, struct resp_header &p)
{
    quint32 id, status, len;

    in >> id >> status >> len;
    p.id = (int)id;
    p.status = (int)status;
    p.len = (int)len;
    return in;
}

void Client::sendHeader(quint16 id, quint16 len)
{
    struct cmd_header header;

    blockSize = 0;
    tcpSocket->abort();

    tcpSocket->connectToHost(hostCombo->currentText(),
                             portLineEdit->text().toInt());

    qDebug() << "connected to server";

    QDataStream out(tcpSocket);
    out.setByteOrder(QDataStream::BigEndian);

    header.id = id;
    header.len = len;

    out << header;

    qDebug() << "send header";
}

void Client::requestTemp()
{
    sendHeader(CMD_READ_TEMP, 0);
}

void Client::switchToggled()
{
    sendHeader(CMD_SET_SW, sizeof(int));

    QDataStream out(tcpSocket);
    out.setByteOrder(QDataStream::BigEndian);

    out << (quint32)switchButton->isChecked();

    switchButton->setText(switchButton->isChecked() ? "Switch OFF" : "Switch ON");
}

static int resp_size[CMD_NUM] = {
    0, /* CMD_SET_SW */
    sizeof(int) /* CMD_READ_TEMP */
};

void Client::readResp()
{
    QDataStream in(tcpSocket);
    struct resp resp;
    in.setVersion(QDataStream::Qt_4_0);
    in.setByteOrder(QDataStream::BigEndian);

    if (tcpSocket->bytesAvailable() < (int)(sizeof(struct resp_header)))
        return;
    in >> resp.header;

    qDebug() << "resp status " << resp.header.id <<
                "status" << resp.header.status <<
                "len" << resp.header.len;

    if (resp.header.len != resp_size[resp.header.id]) {
        qDebug() << "Cmd" << resp.header.id << "with invalid len";
        return;
    }
    if (resp.header.len) {
        if (tcpSocket->bytesAvailable() < resp.header.len)
            return;
    }

    switch (resp.header.id) {
    case CMD_READ_TEMP: {
        qint32 temp;
        in >> temp;
        tempLabel->setText("Temp " + QString::number(temp / 1000.0));
        break;
    }
    default:
        qDebug() << "invalid resp id" << resp.header.id;
    }

    getTempButton->setEnabled(true);
    switchButton->setEnabled(true);
}

void Client::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }

    getTempButton->setEnabled(true);
    switchButton->setEnabled(true);
}


void Client::enableButtons()
{
    int flag = ((!networkSession || networkSession->isOpen()) &&
            !hostCombo->currentText().isEmpty() &&
            !portLineEdit->text().isEmpty());

    getTempButton->setEnabled(flag);
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

