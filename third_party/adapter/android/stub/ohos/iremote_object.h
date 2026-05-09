/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * Minimal iremote_object.h stub for Android — provides sptr<T> and IRemoteObject.
 */

#ifndef OHOS_IREMOTE_OBJECT_H
#define OHOS_IREMOTE_OBJECT_H

#include <cstddef>

namespace OHOS {

class IRemoteObject {
public:
    virtual ~IRemoteObject() = default;
};

template<typename T>
class sptr {
public:
    sptr() : ptr_(nullptr) {}
    sptr(std::nullptr_t) : ptr_(nullptr) {}
    explicit sptr(T* ptr) : ptr_(ptr) {}

    T* GetRefPtr() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    operator bool() const { return ptr_ != nullptr; }

private:
    T* ptr_;
};

} // namespace OHOS

#endif // OHOS_IREMOTE_OBJECT_H
