#include "device.h"
#include "protocol.h"
#include <QRandomGenerator>
#include <QMutexLocker>
#include <QtMath>
#include <QSerialPortInfo>

#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <unistd.h>
#include <QSocketNotifier>
#endif

// DeviceDataSimulator implementation
DeviceDataSimulator::DeviceDataSimulator(QObject* parent)
    : IDeviceSource(parent)
    , deviceId_("SIM001")
    , timer_(new QTimer(this))
    , updateIntervalMs_(1000)
    , sampleCount_(0)
    , isRunning_(false)
{
    connect(timer_, &QTimer::timeout, this, &DeviceDataSimulator::generateSample);
    initializeDefaultMetrics();
}

DeviceDataSimulator::~DeviceDataSimulator()
{
    stop();
}

bool DeviceDataSimulator::configure(const QJsonObject& config)
{
    if (config.contains("deviceId")) {
        deviceId_ = config["deviceId"].toString();
    }
    if (config.contains("updateInterval")) {
        updateIntervalMs_ = config["updateInterval"].toInt();
    }
    
    // Configure custom metrics if provided
    if (config.contains("metrics")) {
        QJsonArray metricsArray = config["metrics"].toArray();
        metrics_.clear();
        
        for (const QJsonValue& value : metricsArray) {
            QJsonObject metricConfig = value.toObject();
            QString name = metricConfig["name"].toString();
            QString unit = metricConfig["unit"].toString();
            double minVal = metricConfig["min"].toDouble();
            double maxVal = metricConfig["max"].toDouble();
            addMetric(name, unit, minVal, maxVal);
        }
    }
    
    return true;
}

QJsonObject DeviceDataSimulator::getConfiguration() const
{
    QJsonObject config;
    config["deviceId"] = deviceId_;
    config["updateInterval"] = updateIntervalMs_;
    
    QJsonArray metricsArray;
    for (const MetricConfig& metric : metrics_) {
        QJsonObject metricObj;
        metricObj["name"] = metric.name;
        metricObj["unit"] = metric.unit;
        metricObj["min"] = metric.minValue;
        metricObj["max"] = metric.maxValue;
        metricsArray.append(metricObj);
    }
    config["metrics"] = metricsArray;
    
    return config;
}

bool DeviceDataSimulator::start()
{
    if (isRunning_) return true;
    
    timer_->setInterval(updateIntervalMs_);
    timer_->start();
    isRunning_ = true;
    
    qCInfo(logDevice) << "Device simulator started:" << deviceId_ << "interval:" << updateIntervalMs_ << "ms";
    emit started();
    return true;
}

void DeviceDataSimulator::stop()
{
    if (!isRunning_) return;
    
    timer_->stop();
    isRunning_ = false;
    
    qCInfo(logDevice) << "Device simulator stopped:" << deviceId_;
    emit stopped();
}

bool DeviceDataSimulator::isRunning() const
{
    return isRunning_;
}

QStringList DeviceDataSimulator::getAvailableMetrics() const
{
    QStringList metrics;
    for (const MetricConfig& metric : metrics_) {
        metrics.append(metric.name);
    }
    return metrics;
}

void DeviceDataSimulator::addMetric(const QString& name, const QString& unit, double minVal, double maxVal)
{
    MetricConfig metric;
    metric.name = name;
    metric.unit = unit;
    metric.minValue = minVal;
    metric.maxValue = maxVal;
    metric.currentValue = (minVal + maxVal) / 2.0; // Start at middle
    metric.trend = 0.0;
    
    metrics_.append(metric);
}

void DeviceDataSimulator::generateSample()
{
    for (MetricConfig& metric : metrics_) {
        double value = generateRealisticValue(metric);
        
        DeviceSample sample(deviceId_, metric.name, value, metric.unit);
        
        // Add some realistic metadata
        QJsonObject metadata;
        metadata["quality"] = "good";
        metadata["source"] = "simulator";
        sample.metadata = metadata;
        
        emitSample(sample);
        sampleCount_++;
        lastSampleTime_ = QDateTime::currentDateTime();
    }
}

void DeviceDataSimulator::initializeDefaultMetrics()
{
    addMetric("temperature", "°C", 15.0, 35.0);
    addMetric("pressure", "bar", 0.8, 1.2);
    addMetric("vibration", "mm/s", 0.0, 10.0);
    addMetric("current", "A", 0.0, 50.0);
    addMetric("voltage", "V", 220.0, 240.0);
    addMetric("rpm", "rpm", 1400.0, 1600.0);
}

double DeviceDataSimulator::generateRealisticValue(MetricConfig& metric)
{
    // Generate realistic trending values with some noise
    QRandomGenerator* rng = QRandomGenerator::global();
    
    // Random walk with trend
    double noise = (rng->generateDouble() - 0.5) * 0.1; // ±5% noise
    double trendChange = (rng->generateDouble() - 0.5) * 0.05; // Small trend changes
    
    metric.trend = qBound(-0.1, metric.trend + trendChange, 0.1);
    metric.currentValue += metric.trend * (metric.maxValue - metric.minValue) + noise;
    
    // Keep within bounds
    metric.currentValue = qBound(metric.minValue, metric.currentValue, metric.maxValue);
    
    return metric.currentValue;
}

// SerialPortSource implementation
SerialPortSource::SerialPortSource(QObject* parent)
    : IDeviceSource(parent)
    , serialPort_(new QSerialPort(this))
    , deviceId_("SERIAL001")
    , baudRate_(9600)
    , sampleCount_(0)
{
    connect(serialPort_, &QSerialPort::readyRead, this, &SerialPortSource::readData);
    connect(serialPort_, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error),
            this, &SerialPortSource::handleError);
}

SerialPortSource::~SerialPortSource()
{
    stop();
}

bool SerialPortSource::configure(const QJsonObject& config)
{
    if (config.contains("deviceId")) {
        deviceId_ = config["deviceId"].toString();
    }
    if (config.contains("portName")) {
        portName_ = config["portName"].toString();
    }
    if (config.contains("baudRate")) {
        baudRate_ = config["baudRate"].toInt();
    }
    
    if (portName_.isEmpty()) {
        // Auto-detect available ports
        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        if (!ports.isEmpty()) {
            portName_ = ports.first().portName();
            qCInfo(logDevice) << "Auto-selected serial port:" << portName_;
        }
    }
    
    return !portName_.isEmpty();
}

QJsonObject SerialPortSource::getConfiguration() const
{
    QJsonObject config;
    config["deviceId"] = deviceId_;
    config["portName"] = portName_;
    config["baudRate"] = baudRate_;
    return config;
}

bool SerialPortSource::start()
{
    if (isRunning()) return true;
    
    if (portName_.isEmpty()) {
        lastError_ = "No serial port configured";
        emitError(lastError_);
        return false;
    }
    
    serialPort_->setPortName(portName_);
    serialPort_->setBaudRate(baudRate_);
    serialPort_->setDataBits(QSerialPort::Data8);
    serialPort_->setParity(QSerialPort::NoParity);
    serialPort_->setStopBits(QSerialPort::OneStop);
    
    if (!serialPort_->open(QIODevice::ReadWrite)) {
        lastError_ = QString("Failed to open serial port %1: %2").arg(portName_, serialPort_->errorString());
        emitError(lastError_);
        return false;
    }
    
    qCInfo(logDevice) << "Serial port opened:" << portName_ << "at" << baudRate_ << "baud";
    emit started();
    return true;
}

void SerialPortSource::stop()
{
    if (serialPort_->isOpen()) {
        serialPort_->close();
        qCInfo(logDevice) << "Serial port closed:" << portName_;
        emit stopped();
    }
}

bool SerialPortSource::isRunning() const
{
    return serialPort_->isOpen();
}

QStringList SerialPortSource::getAvailableMetrics() const
{
    return QStringList() << "raw_data"; // Basic implementation
}

void SerialPortSource::readData()
{
    QByteArray data = serialPort_->readAll();
    readBuffer_.append(data);
    
    // Look for line breaks to parse complete messages
    while (readBuffer_.contains('\n')) {
        int index = readBuffer_.indexOf('\n');
        QByteArray message = readBuffer_.left(index);
        readBuffer_.remove(0, index + 1);
        
        if (!message.isEmpty()) {
            parseMessage(message.trimmed());
        }
    }
}

void SerialPortSource::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        lastError_ = serialPort_->errorString();
        emitError(lastError_);
        qCWarning(logDevice) << "Serial port error:" << error << lastError_;
    }
}

void SerialPortSource::parseMessage(const QByteArray& message)
{
    DeviceSample sample;
    
    // Try JSON format first
    if (message.startsWith('{')) {
        sample = parseJsonMessage(message);
    } else {
        // Fall back to CSV format
        sample = parseCSVMessage(message);
    }
    
    if (!sample.deviceId.isEmpty()) {
        sample.deviceId = deviceId_; // Override with our device ID
        emitSample(sample);
        sampleCount_++;
        lastSampleTime_ = QDateTime::currentDateTime();
    }
}

DeviceSample SerialPortSource::parseJsonMessage(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return DeviceSample(); // Empty sample
    }
    
    return DeviceSample::fromJson(doc.object());
}

DeviceSample SerialPortSource::parseCSVMessage(const QByteArray& data)
{
    // Simple CSV parser: metric,value,unit
    QStringList parts = QString::fromUtf8(data).split(',');
    if (parts.size() >= 2) {
        QString metric = parts[0].trimmed();
        QVariant value = parts[1].trimmed();
        QString unit = parts.size() > 2 ? parts[2].trimmed() : QString();
        
        return DeviceSample(deviceId_, metric, value, unit);
    }
    
    return DeviceSample(); // Empty sample
}

#ifdef Q_OS_LINUX
// SocketCanSource implementation (Linux only)
SocketCanSource::SocketCanSource(QObject* parent)
    : IDeviceSource(parent)
    , deviceId_("CAN001")
    , interfaceName_("can0")
    , socketFd_(-1)
    , socketNotifier_(nullptr)
    , sampleCount_(0)
{
}

SocketCanSource::~SocketCanSource()
{
    stop();
}

bool SocketCanSource::configure(const QJsonObject& config)
{
    if (config.contains("deviceId")) {
        deviceId_ = config["deviceId"].toString();
    }
    if (config.contains("interface")) {
        interfaceName_ = config["interface"].toString();
    }
    
    return true;
}

QJsonObject SocketCanSource::getConfiguration() const
{
    QJsonObject config;
    config["deviceId"] = deviceId_;
    config["interface"] = interfaceName_;
    return config;
}

bool SocketCanSource::start()
{
    if (isRunning()) return true;
    
    if (!setupCanSocket()) {
        return false;
    }
    
    socketNotifier_ = new QSocketNotifier(socketFd_, QSocketNotifier::Read, this);
    connect(socketNotifier_, &QSocketNotifier::activated, this, &SocketCanSource::readCanFrames);
    
    qCInfo(logDevice) << "CAN socket opened on interface:" << interfaceName_;
    emit started();
    return true;
}

void SocketCanSource::stop()
{
    if (socketNotifier_) {
        socketNotifier_->deleteLater();
        socketNotifier_ = nullptr;
    }
    
    closeCanSocket();
    
    if (socketFd_ != -1) {
        qCInfo(logDevice) << "CAN socket closed on interface:" << interfaceName_;
        emit stopped();
    }
}

bool SocketCanSource::isRunning() const
{
    return socketFd_ != -1;
}

QStringList SocketCanSource::getAvailableMetrics() const
{
    return QStringList() << "can_frame"; // Basic implementation
}

bool SocketCanSource::setupCanSocket()
{
    socketFd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd_ < 0) {
        lastError_ = "Failed to create CAN socket";
        emitError(lastError_);
        return false;
    }
    
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interfaceName_.toUtf8().constData());
    ioctl(socketFd_, SIOCGIFINDEX, &ifr);
    
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(socketFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        lastError_ = QString("Failed to bind CAN socket to %1").arg(interfaceName_);
        emitError(lastError_);
        closeCanSocket();
        return false;
    }
    
    return true;
}

void SocketCanSource::closeCanSocket()
{
    if (socketFd_ != -1) {
        close(socketFd_);
        socketFd_ = -1;
    }
}

void SocketCanSource::readCanFrames()
{
    struct can_frame frame;
    ssize_t nbytes = read(socketFd_, &frame, sizeof(frame));
    
    if (nbytes == sizeof(frame)) {
        QByteArray frameData(reinterpret_cast<const char*>(&frame), sizeof(frame));
        DeviceSample sample = parseCanFrame(frameData);
        
        if (!sample.metricName.isEmpty()) {
            emitSample(sample);
            sampleCount_++;
            lastSampleTime_ = QDateTime::currentDateTime();
        }
    }
}

DeviceSample SocketCanSource::parseCanFrame(const QByteArray& frame)
{
    if (frame.size() != sizeof(struct can_frame)) {
        return DeviceSample();
    }
    
    const struct can_frame* canFrame = reinterpret_cast<const struct can_frame*>(frame.constData());
    
    // Simple CAN frame interpretation (customize based on your protocol)
    QString metric = QString("can_id_0x%1").arg(canFrame->can_id, 0, 16);
    QVariant value = QByteArray(reinterpret_cast<const char*>(canFrame->data), canFrame->can_dlc);
    
    QJsonObject metadata;
    metadata["can_id"] = static_cast<int>(canFrame->can_id);
    metadata["dlc"] = canFrame->can_dlc;
    
    DeviceSample sample(deviceId_, metric, value, "bytes");
    sample.metadata = metadata;
    
    return sample;
}
#endif

// DeviceManager implementation
DeviceManager::DeviceManager(QObject* parent)
    : QObject(parent)
    , aggregationIntervalMs_(100)
{
}

DeviceManager::~DeviceManager()
{
    stopAll();
    qDeleteAll(sources_);
}

void DeviceManager::addSource(IDeviceSource* source)
{
    if (!sources_.contains(source)) {
        sources_.append(source);
        connect(source, &IDeviceSource::sampleReady, this, &DeviceManager::onSampleReady);
        connect(source, &IDeviceSource::error, this, &DeviceManager::onSourceError);
        
        qCInfo(logDevice) << "Device source added:" << source->getDeviceId() << "type:" << source->getDeviceType();
    }
}

void DeviceManager::removeSource(IDeviceSource* source)
{
    sources_.removeAll(source);
    disconnect(source, nullptr, this, nullptr);
    
    qCInfo(logDevice) << "Device source removed:" << source->getDeviceId();
}

IDeviceSource* DeviceManager::getSource(const QString& deviceId) const
{
    for (IDeviceSource* source : sources_) {
        if (source->getDeviceId() == deviceId) {
            return source;
        }
    }
    return nullptr;
}

void DeviceManager::startAll()
{
    for (IDeviceSource* source : sources_) {
        source->start();
    }
    
    qCInfo(logDevice) << "Started" << sources_.size() << "device sources";
}

void DeviceManager::stopAll()
{
    for (IDeviceSource* source : sources_) {
        source->stop();
    }
    
    qCInfo(logDevice) << "Stopped all device sources";
}

int DeviceManager::getRunningCount() const
{
    int count = 0;
    for (IDeviceSource* source : sources_) {
        if (source->isRunning()) {
            count++;
        }
    }
    return count;
}

QList<DeviceSample> DeviceManager::getRecentSamples(int maxAge) const
{
    QMutexLocker locker(&samplesMutex_);
    
    QDateTime cutoff = QDateTime::currentDateTime().addMSecs(-maxAge);
    QList<DeviceSample> recent;
    
    for (const DeviceSample& sample : recentSamples_) {
        if (sample.timestamp >= cutoff) {
            recent.append(sample);
        }
    }
    
    return recent;
}

void DeviceManager::onSampleReady(const DeviceSample& sample)
{
    {
        QMutexLocker locker(&samplesMutex_);
        recentSamples_.append(sample);
        
        // Clean up old samples (keep last 1000 or within time window)
        while (recentSamples_.size() > 1000) {
            recentSamples_.removeFirst();
        }
    }
    
    emit sampleReceived(sample);
}

void DeviceManager::onSourceError(const QString& message)
{
    IDeviceSource* source = qobject_cast<IDeviceSource*>(sender());
    if (source) {
        emit sourceError(source->getDeviceId(), message);
    }
}