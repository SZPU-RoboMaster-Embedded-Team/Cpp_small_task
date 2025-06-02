/**
 * @file device_online_detector.hpp
 * @brief 设备在线检测HAL层库 (C++版本)
 * @details 用于检测设备是否在线，当特定数据在设定的时间内未更新时，判定设备断联
 */

#ifndef DEVICE_ONLINE_DETECTOR_HPP
#define DEVICE_ONLINE_DETECTOR_HPP

#include "main.h"
#include <cstdint>

namespace BSP::WATCH_STATE
{
/**
 * @brief 设备状态枚举
 */
enum class Status
{
    OFFLINE = 0, // 设备离线
    ONLINE = 1   // 设备在线
};
/**
 * @brief 设备在线检测类
 */
class StateWatch
{
  public:
    /**
     * @brief 构造函数
     * @param name 设备名称
     * @param timeout_ms 超时时间(毫秒)
     */
    StateWatch(const char *name, uint32_t timeout_ms);

    /**
     * @brief 默认析构函数
     */
    ~StateWatch() = default;

    /**
     * @brief 更新设备数据时间戳
     * @details 在收到设备数据时调用此函数更新时间戳
     */
    void updateTimestamp();

    /**
     * @brief 检查设备是否在线
     * @details 检查从上次更新到当前是否超时
     * @return 设备状态
     */
    Status check();

    /**
     * @brief 获取设备状态
     * @return 当前设备状态
     */
    Status getStatus() const;

    /**
     * @brief 获取设备断联时间(毫秒)
     * @details 如果设备在线，返回0
     * @return 设备断联时长(毫秒)
     */
    uint32_t getOfflineTime() const;

    /**
     * @brief 手动设置设备状态
     * @param status 设备状态
     */
    void setStatus(Status status);

    /**
     * @brief 获取设备名称
     * @return 设备名称
     */
    const char *getName() const
    {
        return name_;
    }

    /**
     * @brief 设置断联回调函数
     * @param callback 当设备状态变为离线时调用的回调函数
     * @param user_data 回调函数的用户数据指针
     */
    typedef void (*OfflineCallbackFunc)(const StateWatch *, void *);
    void setOfflineCallback(OfflineCallbackFunc callback, void *user_data = nullptr)
    {
        offline_callback_ = callback;
        offline_callback_data_ = user_data;
    }

    /**
     * @brief 设置上线回调函数
     * @param callback 当设备状态变为在线时调用的回调函数
     * @param user_data 回调函数的用户数据指针
     */
    typedef void (*OnlineCallbackFunc)(const StateWatch *, void *);
    void setOnlineCallback(OnlineCallbackFunc callback, void *user_data = nullptr)
    {
        online_callback_ = callback;
        online_callback_data_ = user_data;
    }

  private:
    /**
     * @brief 获取系统当前时间(毫秒)
     * @return 当前系统时间
     */
    static uint32_t getSystemTimeMs();

    static constexpr uint8_t MAX_NAME_LENGTH = 32; /// 设备名称最大长度
    char name_[MAX_NAME_LENGTH];                   /// 设备名称
    uint32_t timeout_ms_;                          /// 超时时间(毫秒)
    uint32_t last_update_time_;                    /// 上次数据更新时间
    Status status_ = Status::OFFLINE;              /// 当前设备状态

    OfflineCallbackFunc offline_callback_ = nullptr; // 断联回调函数
    void *offline_callback_data_ = nullptr;          // 断联回调函数用户数据
    OnlineCallbackFunc online_callback_ = nullptr;   // 上线回调函数
    void *online_callback_data_ = nullptr;           // 上线回调函数用户数据
};

} // namespace BSP::WATCH_STATE

#endif /* DEVICE_ONLINE_DETECTOR_HPP */