#include <QtWidgets>
#include <QTcpSocket>
#include <QDataStream>
#include <QTime>

#include "client.h"
#include "switch.h"

#if 0
#define ENDIAN QDataStream::BigEndian
#else
#define ENDIAN QDataStream::LittleEndian
#endif

#define STATUSTIMEOUT 5

Client::Client(QGridLayout *layout, QString *name, int pos, QString *addr, int port)
    : name(name), pos(pos), addr(addr), port(port)
{
    /* connect socket to the server */
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readResp()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));

    tcpSocket->connectToHost(addr->toStdString().c_str(), port);

    /*
     * Initializing the user interface
     */

    tempThresSlider = new QSlider(Qt::Horizontal, this);
    tempThresSlider->setMinimum(15);
    tempThresSlider->setMaximum(28);
    tempThresSlider->setValue(0);

    tempThresLabel = new QLabel(tr("Max"));
    tempThresLabel->setBuddy(tempThresSlider);

    tempLabel = new QLabel(*name + tr("-"));
    tempThresValueLabel = new QLabel(tr("-"));
    tempThresValueLabel->setBuddy(tempThresSlider);

    switchButton = new QPushButton("Switch ON", this);
    switchButton->setCheckable(false);
    switchButton->setEnabled(false);

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(switchButton, QDialogButtonBox::ActionRole);

    connect(tempThresSlider, SIGNAL(valueChanged(int)),
            this, SLOT(tempThresChanged()));
    connect(switchButton, SIGNAL(clicked()), this, SLOT(switchToggled()));

    layout->addWidget(tempLabel, 2*pos, 0, 1, 1);
    layout->addWidget(buttonBox, 2*pos, 1, 1, 2);

    layout->addWidget(tempThresLabel, 2*pos + 1, 0);
    layout->addWidget(tempThresSlider, 2*pos + 1, 1);
    layout->addWidget(tempThresValueLabel, 2*pos + 1, 2);

    qDebug() << "client" << pos << "created";
    statusTimer = 0;
}

QString *Client::getAddr(void)
{
    return addr;
}

int Client::getPort(void)
{
    return port;
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
        tempLabel->setText( *name + " " + str + " " +
                            QString::number(s->temp / 1000.0, 'f', 1) + "°C");

        tempThresSlider->setValue(s->tempThres/1000.0);
        qDebug() << "  temp" << s->temp / 1000.0 << "thres" << s->tempThres / 1000.0;
        switchButton->setText(s->sw_pos ? "Switch OFF" : "Switch ON");

        if (!switchButton->isEnabled()) {
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
    requestStatus();
    statusTimer = startTimer(STATUSTIMEOUT * 1000);
    switchButton->setEnabled(1);
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
        tcpSocket->connectToHost(addr->toStdString().c_str(), port);
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
    tcpSocket->connectToHost(addr->toStdString().c_str(), port);
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
