//
// Created by panshiquan on 2026/5/13.
//
#include "test_manage.hpp"

bool TestManage::connect(){
    if (m_state != Idle) {
        return false;
    }


    m_working = false;
    if (m_worker.joinable()) m_worker.join();

    if (m_connect_protocol) {
        m_connect_protocol->reset();
        delete m_connect_protocol;
        m_connect_protocol = nullptr;
    }

    auto cfg = Config::instance();
    m_connect_protocol = new CFactoryTestProtocol(
        new SerialChannel(cfg->connectSerial().com,static_cast<QSerialPort::BaudRate>(cfg->connectSerial().baud_rate)));
    if (!m_connect_protocol->start()) {
        delete m_connect_protocol;
        m_connect_protocol = nullptr;
        return false;
    }

    m_working = true;
    m_worker = std::thread([this]() {
        while (m_working) {
            // 接收消息
            auto msg = m_connect_protocol->pull();
            if (msg.data_len > 3) {
                handleMsg(msg);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });

    m_state = Idle;
    emit stateChanged(m_state);
    return true;
}

void TestManage::disconnect() {
    if (m_state == Idle) return;
    if (m_state == Busy) stop();

    m_working = false;
    if (m_worker.joinable()) m_worker.join();

    if (m_connect_protocol) {
        m_connect_protocol->reset();
        delete m_connect_protocol;
        m_connect_protocol = nullptr;
    }

    m_state = Idle;
    emit stateChanged(m_state);
}

void TestManage::start(){
    if (m_state != Idle) {
        return;
    }

    QVector<TestRunItem> plan = Config::instance()->enabledTestPlan();
    m_flowController->loadTestPlan(plan);
    m_flowController->setAllStandby();   // 表格显示"待测试"

    m_state = Standby;
    emit stateChanged(m_state);
}

void TestManage::restart() {
    if (m_state != Busy) return;
    m_flowController->restartAll();

    // 状态仍为 Busy
    emit stateChanged(m_state);
}

void TestManage::stop() {
    if (m_state != Busy) return;
    m_flowController->stopAll();
    m_state = Standby;
    emit stateChanged(m_state);
}

void TestManage::reset() {
    if (m_state == Busy) stop();
    m_flowController->resetAll();
    m_state = Standby;
    emit stateChanged(m_state);
}

TestManage::TestManage() {
    m_rt = RtModel::instance();
    m_flowController = new TestFlowController(m_rt, this);

    // 跨线程：工作线程 → 主线程
    QObject::connect(this, &TestManage::startAllRequested, m_flowController, &TestFlowController::startAll, Qt::QueuedConnection);
    QObject::connect(this, &TestManage::itemResultReceived, m_flowController, &TestFlowController::onItemResultReceived, Qt::QueuedConnection);

    // 所有测试项完成，通知 QML 结果后切回 Standby
    QObject::connect(m_flowController, &TestFlowController::allItemsFinished, this, [this]() {
        if (m_state == Busy) {
            int fail = m_rt->fail_num().toInt();
            emit testFinished(fail);
            qDebug() << "所有测试项已完成" << (fail == 0 ? "PASS" : "FAIL");
            m_state = Standby;
            emit stateChanged(m_state);
        }
    });

    m_state = Idle;
    this->connect();
    m_flowController->loadTestPlan(Config::instance()->enabledTestPlan());
}

TestManage::~TestManage() {
    disconnect();
}

void TestManage::handleMsg(const MessageEntity &msg) {
    if ( msg.data_len < 1 || msg.data == nullptr || m_state.load() == Idle) {
        return;
    }
    std::string buf(msg.data, msg.data_len);
    CodeEntity* request_code = CFactoryTestProtocol::parseCodeEntity(buf);
    if (request_code == nullptr) {
        qDebug() << "handleMsg: parseCodeEntity error!";
        return;
    }

    switch (m_state.load()) {
        case Standby:
            if (request_code->code != MsgFuncCode::HANDSHAKE) {
                qDebug() << "收到错误消息, 请复位重新测试";
                return;
            }
            handleHandShake();
            break;
        case Busy:
            handleBusyMsg(request_code);
            break;
        default: break;
    }
}

void TestManage::handleHandShake() {
    qDebug() << "Standby 状态收到设备握手";
    emit startAllRequested();             // 跨线程通知主线程创建定时器
    auto envs =  Config::instance()->envItems();
    QJsonObject obj;
    obj["code"] = 101;

    // 构建 para 对象
    QJsonObject paraObj;
    for (auto& env : envs) {
        paraObj[env.name] = QString(env.value);
    }

    // 将 para 对象序列化为 JSON 字符串
    QJsonDocument paraDoc(paraObj);
    QByteArray paraBytes = paraDoc.toJson(QJsonDocument::Compact);

    // 构建数字数组
    QJsonArray paraArray;
    for (int i = 0; i < paraBytes.size(); ++i) {
        paraArray.append(static_cast<int>(static_cast<unsigned char>(paraBytes.at(i))));
    }

    obj["para"] = paraArray;

    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity response_msg;
    response_msg.index = 0;
    response_msg.type = 0;
    response_msg.data = strdup(bytes.constData());
    response_msg.data_len = bytes.size();
    m_connect_protocol->push(response_msg);
    sleep(2);

    // ==================== 第二条消息：code = 102 ====================
    QList<int> item_codes;
    auto items = Config::instance()->enabledTestPlan();
    for (auto& item : items) {
        item_codes.append(item.code);
    }

    // 将 item_codes 转成 JSON 数组字符串，如 "[200,202,203,...]"
    QJsonArray codeArray;
    for (int code : item_codes) {
        codeArray.append(code);
    }
    QJsonDocument codeDoc(codeArray);
    QByteArray codeBytes = codeDoc.toJson(QJsonDocument::Compact);
    QString paraString = QString::fromUtf8(codeBytes);

    // 将字符串的 UTF-8 字节转成数字数组
    QByteArray paraUtf8 = paraString.toUtf8();
    QJsonArray paraArray102;
    for (int i = 0; i < paraUtf8.size(); ++i) {
        paraArray102.append(static_cast<int>(static_cast<signed char>(paraUtf8.at(i))));
    }

    QJsonObject obj102;
    obj102["code"] = 102;
    obj102["para"] = paraArray102;

    QJsonDocument doc102(obj102);
    QByteArray bytes102 = doc102.toJson(QJsonDocument::Compact);

    MessageEntity msg102;
    msg102.index = 0;
    msg102.type = 0;
    msg102.data = strdup(bytes102.constData());
    msg102.data_len = bytes102.size();
    m_connect_protocol->push(msg102);
    m_state.store(Busy);
    emit stateChanged(m_state);
    emit handshakeDone();
}
void TestManage::handleBusyMsg(CodeEntity *request) {
    if (!request->common) return;

    int state = request->common->state;
    QString msg = (request->common->msg && request->common->msg[0] != '\0')
                  ? QString::fromUtf8(request->common->msg) : QString();

    qDebug() << "测项结果上报 code:" << request->code
             << (state == 1 ? "PASS" : "FAIL")
             << (msg.isEmpty() ? "" : "msg:" + msg);

    emit itemResultReceived(request->code, state, msg);
}

