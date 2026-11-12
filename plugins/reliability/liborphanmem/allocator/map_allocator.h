/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBMEMUNREACHABLE_ALLOCATOR_H
#define LIBMEMUNREACHABLE_ALLOCATOR_H

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OHOS {

class HeapImpl;

template <typename T>
class Allocator;

class Heap {
public:
    Heap();
    ~Heap();

    Heap(const Heap &other) : impl_(other.impl_), ownsImpl_(false) {}

    Heap &operator=(const Heap &) = delete;

    void *Allocate(size_t size) const;

    void Deallocate(void *ptr) const;

    bool Empty();

    static void Deallocate(HeapImpl *impl, void *ptr);

    template <class T>
    T *Allocate() const
    {
        return static_cast<T *>(Allocate(sizeof(T)));
    }

    bool operator==(const Heap &other) const
    {
        return impl_ == other.impl_;
    }

    bool operator!=(const Heap &other) const
    {
        return !(*this == other);
    }

    template <class T>
    using unique_ptr = std::unique_ptr<T, std::function<void(void *)>>;

    template <class T, class... Args>
    unique_ptr<T> MakeUnique(Args &&...args) const
    {
        HeapImpl *impl = impl_;
        auto deleter = [impl](void *ptr) {
            static_cast<T *>(ptr)->~T();
            Deallocate(impl, ptr);
        };
        // placement new on the custom heap, std::make_unique is not applicable here
        T *obj = new (Allocate<T>()) T(std::forward<Args>(args)...);
        return unique_ptr<T>(obj, deleter);
    }

    template <class T>
    using shared_ptr = std::shared_ptr<T>;

protected:
    HeapImpl *impl_;
    bool ownsImpl_;
};

template <typename T>
class STLAllocator {
public:
    using value_type = T;  // NOLINT

    ~STLAllocator() {}

    STLAllocator() : heap_() {}

    explicit STLAllocator(const Heap &heap) : heap_(heap) {}

    template <typename U>
    explicit STLAllocator(const STLAllocator<U> &other) : heap_(other.heap_)
    {
    }

    STLAllocator(const STLAllocator &) = default;
    STLAllocator<T> &operator=(const STLAllocator<T> &) = default;

    T *allocate(std::size_t n)  // NOLINT
    {
        size_t totalBytes = n * sizeof(T);
        void *ptr = heap_.Allocate(totalBytes);
        return static_cast<T *>(ptr);
    }

    template <typename U>
    bool operator==(const STLAllocator<U> &other) const
    {
        return heap_ == other.heap_;
    }

    void deallocate(T *ptr, std::size_t)  // NOLINT
    {
        heap_.Deallocate(ptr);
    }

    template <typename U>
    inline bool operator!=(const STLAllocator<U> &other) const
    {
        return !(heap_ == other.heap_);
    }

    template <typename U>
    friend class STLAllocator;

protected:
    Heap heap_;
};

template <class T>
class Allocator : public STLAllocator<T> {
public:
    ~Allocator() {}

    Allocator() = default;

    explicit Allocator(const Heap &other) : STLAllocator<T>(other) {}

    template <typename U>
    explicit Allocator(const STLAllocator<U> &other) : STLAllocator<T>(other)
    {
    }

    Allocator(const Allocator &) = default;
    Allocator<T> &operator=(const Allocator<T> &) = default;

    using STLAllocator<T>::allocate;
    using STLAllocator<T>::deallocate;
    using STLAllocator<T>::heap_;

    T *Allocate() const
    {
        return STLAllocator<T>::allocate(1);
    }

    void Deallocate(void *ptr) const
    {
        heap_.Deallocate(ptr);
    }

    using UniquePtr = Heap::unique_ptr<T>;

    template <class... Args>
    UniquePtr MakeUnique(Args &&...args) const
    {
        return heap_.template MakeUnique<T>(std::forward<Args>(args)...);
    }
};

namespace MAP_ALLOCATOR {

template <class T>
using vector = std::vector<T, Allocator<T>>;

using String = std::basic_string<char, std::char_traits<char>, Allocator<char>>;

template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator<std::pair<const Key, T>>>;

template <class Key, class T, class Compare = std::less<Key>>
using map = std::map<Key, T, Compare, Allocator<std::pair<const Key, T>>>;

template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_set = std::unordered_set<Key, Hash, KeyEqual, Allocator<Key>>;

template <class T>
using queue = std::queue<T, std::deque<T, Allocator<T>>>;
}  // namespace MAP_ALLOCATOR

}  // namespace OHOS

#endif
