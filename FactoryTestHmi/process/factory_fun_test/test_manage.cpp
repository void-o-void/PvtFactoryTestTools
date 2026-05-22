//
// Created by panshiquan on 2026/5/13.
//
#include "test_manage.hpp"
#include "aging_test_manage.hpp"

// ==================== openSerial ====================
void TestManage::openSerial() {
    if (m_connect_protocol) {
        m_connect_protocol->reset();
        delete m_connect_protocol;
        m_connect_protocol = nullptr;
    }

    auto cfg = Config::instance();
    m_connect_protocol = new CFactoryTestProtocol(
        new SerialChannel(cfg->connectSerial().com,
                          static_cast<QSerialPort::BaudRate>(cfg->connectSerial().baud_rate)));
    m_connect_protocol->start();

    m_working = true;
    m_worker = std::thread([this]() {
        while (m_working) {
            // 老化模式：休眠让出串口，由老化管理器独占通道
            if (m_mode == TestMode::AGING) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            auto msg = m_connect_protocol->pull();
            if (msg.data_len > 3) {
                handleMsg(msg);
            } else {
                if(m_mode == TestMode::AGING) {
                    qDebug() << "唤醒移交控制权";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });
}

// ==================== switchToAging ====================
void TestManage::switchToAging() {
    reset();                                          // 复位功能测试（m_mode→FUNC，停旧老化）
    AmingTestManage::instance()->reset();             // 确保旧老化 worker 已退出
    m_mode = TestMode::AGING;                         // 设置老化模式（在 reset 之后）
    m_connect_protocol->wakeUpQueue();                // 唤醒功能 worker，下次循环睡觉
    AmingTestManage::instance()->takeOver(m_connect_protocol);
}

// ==================== switchToFunctional ====================
void TestManage::switchToFunctional() {
    AmingTestManage::instance()->reset();             // 停止老化 worker
    m_connect_protocol->clearQueue();                 // 先清队列（此时 worker 还在 AGING 睡眠）
    m_mode = TestMode::FUNC;                          // 再切模式，worker 下次循环安全 pull
    reset();                                          // 功能测试 UI 回 Idle
}

// ==================== reset ====================
void TestManage::reset() {
    if (m_flowController) {
        m_flowController->cancelAll();
    }

    QVector<TestRunItem> plan = Config::instance()->enabledTestPlan();
    m_flowController->loadTestPlan(plan);

    m_state = Idle;
    emit stateChanged(m_state);
}

// ==================== start ====================
void TestManage::start() {
    if (m_state != Idle) return;

    QVector<TestRunItem> plan = Config::instance()->enabledTestPlan();
    m_flowController->loadTestPlan(plan);
    m_flowController->setAllStandby();

    m_state = Standby;
    emit stateChanged(m_state);
}

// ==================== 构造 / 析构 ====================
TestManage::TestManage() {
    m_rt = RtModel::instance();
    m_flowController = new TestFlowController(m_rt, this);

    QObject::connect(this, &TestManage::startAllRequested,
                     m_flowController, &TestFlowController::startAll,
                     Qt::QueuedConnection);
    QObject::connect(this, &TestManage::itemResultReceived,
                     m_flowController, &TestFlowController::onItemResultReceived,
                     Qt::QueuedConnection);

    QObject::connect(m_flowController, &TestFlowController::allItemsFinished,
                     this, [this]() {
        if (m_state == Busy) {
            int fail = m_rt->fail_num().toInt();
            emit testFinished(fail);
            qDebug() << "所有测试项已完成" << (fail == 0 ? "PASS" : "FAIL");
            m_state = Standby;
            emit stateChanged(m_state);
        }
    });

    openSerial();
    reset();
}

TestManage::~TestManage() {
    m_working = false;
    if (m_worker.joinable()) m_worker.join();

    if (m_connect_protocol) {
        m_connect_protocol->reset();
        delete m_connect_protocol;
        m_connect_protocol = nullptr;
    }
}

// ==================== handleMsg ====================
void TestManage::handleMsg(const MessageEntity &msg) {
    if (msg.data_len < 1 || msg.data == nullptr || m_state.load() == Idle) {
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

// ==================== handleHandShake ====================
void TestManage::handleHandShake() {
    qDebug() << "Standby 状态收到设备握手";
    emit startAllRequested();

    auto envs = Config::instance()->envItems();
    QJsonObject obj;
    obj["code"] = 101;

    QJsonObject paraObj;
    for (auto& env : envs) {
        paraObj[env.name] = QString(env.value);
    }

    QJsonDocument paraDoc(paraObj);
    QByteArray paraBytes = paraDoc.toJson(QJsonDocument::Compact);

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

    // code = 102
    QList<int> item_codes;
    auto items = Config::instance()->enabledTestPlan();
    for (auto& item : items) {
        item_codes.append(item.code);
    }

    QJsonArray codeArray;
    for (int code : item_codes) {
        codeArray.append(code);
    }
    QJsonDocument codeDoc(codeArray);
    QByteArray codeBytes = codeDoc.toJson(QJsonDocument::Compact);
    QString paraString = QString::fromUtf8(codeBytes);

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

// ==================== handleBusyMsg ====================
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

// ==================== sendCodeMessage ====================
void TestManage::sendCodeMessage(int code, const QJsonObject &paraObj) {
    QJsonDocument paraDoc(paraObj);
    QByteArray paraBytes = paraDoc.toJson(QJsonDocument::Compact);

    QJsonArray paraArray;
    for (int i = 0; i < paraBytes.size(); ++i) {
        paraArray.append(static_cast<int>(static_cast<signed char>(paraBytes.at(i))));
    }

    QJsonObject obj;
    obj["code"] = code;
    obj["para"] = paraArray;

    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);

    MessageEntity response_msg;
    response_msg.index = 0;
    response_msg.type = 0;
    response_msg.data = strdup(bytes.constData());
    response_msg.data_len = bytes.size();
    m_connect_protocol->push(response_msg);
}
