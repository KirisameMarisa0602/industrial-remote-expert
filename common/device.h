#pragma once
// ===============================================
// common/device.h
// Device data interfaces and simulator for MVP foundation
// Provides: IDeviceSource, SerialPortSource, SocketCanSource, DeviceDataSimulator
// ===============================================

#include <QtCore>
#include <QTimer>
#include <QJsonObject>
#include <QSerialPort>
#include <QMutex>

// Device data sample structure
struct DeviceSample {
    QString deviceId;
    QString metricName;
    QVariant value;
    QString unit;
    QDateTime timestamp;
    QJsonObject metadata; // Additional context data
    
    DeviceSample() = default;
    
    DeviceSample(const QString& id, const QString& metric, const QVariant& val, 
                const QString& u = QString(), const QJsonObject& meta = QJsonObject())
        : deviceId(id), metricName(metric), value(val), unit(u), metadata(meta)
        , timestamp(QDateTime::currentDateTime())
    {}
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["deviceId"] = deviceId;
        obj["metric"] = metricName;
        obj["value"] = value.toJsonValue();
        obj["unit"] = unit;
        obj["timestamp"] = timestamp.toMSecsSinceEpoch();
        if (!metadata.isEmpty()) {
            obj["metadata"] = metadata;
        }
        return obj;
    }
    
    static DeviceSample fromJson(const QJsonObject& obj) {
        DeviceSample sample;
        sample.deviceId = obj["deviceId"].toString();
        sample.metricName = obj["metric"].toString();
        sample.value = obj["value"].toVariant();
        sample.unit = obj["unit"].toString();
        sample.timestamp = QDateTime::fromMSecsSinceEpoch(obj["timestamp"].toVariant().toLongLong());
        sample.metadata = obj["metadata"].toObject();
        return sample;
    }
};

// Base interface for device data sources
class IDeviceSource : public QObject
{
    Q_OBJECT
    
public:
    explicit IDeviceSource(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IDeviceSource() = default;
    
    // Configuration
    virtual bool configure(const QJsonObject& config) = 0;
    virtual QJsonObject getConfiguration() const = 0;
    
    // Lifecycle
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    
    // Device info
    virtual QString getDeviceId() const = 0;
    virtual QString getDeviceType() const = 0;
    virtual QStringList getAvailableMetrics() const = 0;
    
    // Statistics
    virtual int getSampleCount() const = 0;
    virtual QDateTime getLastSampleTime() const = 0;
    virtual QString getLastError() const = 0;

signals:
    void sampleReady(const DeviceSample& sample);
    void error(const QString& message);
    void started();
    void stopped();

protected:
    void emitSample(const DeviceSample& sample) {
        emit sampleReady(sample);
    }
    
    void emitError(const QString& message) {
        emit error(message);
    }
};

// Device data simulator for development and testing
class DeviceDataSimulator : public IDeviceSource
{
    Q_OBJECT
    
public:
    explicit DeviceDataSimulator(QObject* parent = nullptr);
    ~DeviceDataSimulator() override;
    
    // IDeviceSource interface
    bool configure(const QJsonObject& config) override;
    QJsonObject getConfiguration() const override;
    bool start() override;
    void stop() override;
    bool isRunning() const override;
    QString getDeviceId() const override { return deviceId_; }
    QString getDeviceType() const override { return "Simulator"; }
    QStringList getAvailableMetrics() const override;
    int getSampleCount() const override { return sampleCount_; }
    QDateTime getLastSampleTime() const override { return lastSampleTime_; }
    QString getLastError() const override { return lastError_; }
    
    // Simulator-specific configuration
    void setUpdateInterval(int ms) { updateIntervalMs_ = ms; }
    void addMetric(const QString& name, const QString& unit, double minVal, double maxVal);
    void setDeviceId(const QString& id) { deviceId_ = id; }

private slots:
    void generateSample();

private:
    struct MetricConfig {
        QString name;
        QString unit;
        double minValue;
        double maxValue;
        double currentValue;
        double trend; // Current trend direction
    };
    
    QString deviceId_;
    QTimer* timer_;
    int updateIntervalMs_;
    QList<MetricConfig> metrics_;
    int sampleCount_;
    QDateTime lastSampleTime_;
    QString lastError_;
    bool isRunning_;
    
    void initializeDefaultMetrics();
    double generateRealisticValue(MetricConfig& metric);
};

// Serial port device source
class SerialPortSource : public IDeviceSource
{
    Q_OBJECT
    
public:
    explicit SerialPortSource(QObject* parent = nullptr);
    ~SerialPortSource() override;
    
    // IDeviceSource interface
    bool configure(const QJsonObject& config) override;
    QJsonObject getConfiguration() const override;
    bool start() override;
    void stop() override;
    bool isRunning() const override;
    QString getDeviceId() const override { return deviceId_; }
    QString getDeviceType() const override { return "SerialPort"; }
    QStringList getAvailableMetrics() const override;
    int getSampleCount() const override { return sampleCount_; }
    QDateTime getLastSampleTime() const override { return lastSampleTime_; }
    QString getLastError() const override { return lastError_; }

private slots:
    void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort* serialPort_;
    QString deviceId_;
    QString portName_;
    qint32 baudRate_;
    QByteArray readBuffer_;
    int sampleCount_;
    QDateTime lastSampleTime_;
    QString lastError_;
    
    void parseMessage(const QByteArray& message);
    DeviceSample parseJsonMessage(const QByteArray& data);
    DeviceSample parseCSVMessage(const QByteArray& data);
};

// CAN bus device source (Linux only)
#ifdef Q_OS_LINUX
class SocketCanSource : public IDeviceSource
{
    Q_OBJECT
    
public:
    explicit SocketCanSource(QObject* parent = nullptr);
    ~SocketCanSource() override;
    
    // IDeviceSource interface
    bool configure(const QJsonObject& config) override;
    QJsonObject getConfiguration() const override;
    bool start() override;
    void stop() override;
    bool isRunning() const override;
    QString getDeviceId() const override { return deviceId_; }
    QString getDeviceType() const override { return "SocketCAN"; }
    QStringList getAvailableMetrics() const override;
    int getSampleCount() const override { return sampleCount_; }
    QDateTime getLastSampleTime() const override { return lastSampleTime_; }
    QString getLastError() const override { return lastError_; }

private slots:
    void readCanFrames();

private:
    QString deviceId_;
    QString interfaceName_;
    int socketFd_;
    QSocketNotifier* socketNotifier_;
    int sampleCount_;
    QDateTime lastSampleTime_;
    QString lastError_;
    
    DeviceSample parseCanFrame(const QByteArray& frame);
    bool setupCanSocket();
    void closeCanSocket();
};
#endif

// Device manager for handling multiple sources
class DeviceManager : public QObject
{
    Q_OBJECT
    
public:
    explicit DeviceManager(QObject* parent = nullptr);
    ~DeviceManager();
    
    // Source management
    void addSource(IDeviceSource* source);
    void removeSource(IDeviceSource* source);
    QList<IDeviceSource*> getSources() const { return sources_; }
    IDeviceSource* getSource(const QString& deviceId) const;
    
    // Lifecycle
    void startAll();
    void stopAll();
    int getRunningCount() const;
    
    // Sample aggregation
    void setAggregationInterval(int ms) { aggregationIntervalMs_ = ms; }
    QList<DeviceSample> getRecentSamples(int maxAge = 5000) const; // Last 5 seconds by default

signals:
    void sampleReceived(const DeviceSample& sample);
    void sourceError(const QString& deviceId, const QString& error);

private slots:
    void onSampleReady(const DeviceSample& sample);
    void onSourceError(const QString& message);

private:
    QList<IDeviceSource*> sources_;
    QList<DeviceSample> recentSamples_;
    int aggregationIntervalMs_;
    QMutex mutable samplesMutex_;
};