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

#ifndef LIBORPHANMEM_ALLOCATOR_LINKED_LIST_H
#define LIBORPHANMEM_ALLOCATOR_LINKED_LIST_H

namespace OHOS {

template <class T>
class LinkedList {
public:
    LinkedList() : next_(this), prev_(this), data_() {}
    explicit LinkedList(T data) : LinkedList()
    {
        data_ = data;
    }
    ~LinkedList() {}
    void Insert(LinkedList<T> &node)
    {
        LinkedList<T> *currentNext = this->next_;
        node.next_ = currentNext;
        node.next_->prev_ = &node;
        this->next_ = &node;
        node.prev_ = this;
    }
    bool Empty()
    {
        return (next_ == this) && (prev_ == this);
    }
    void Remove()
    {
        LinkedList<T> *nextNode = this->next_;
        LinkedList<T> *prevNode = this->prev_;
        nextNode->prev_ = prevNode;
        prevNode->next_ = nextNode;
        this->next_ = this;
        this->prev_ = this;
    }

    LinkedList<T> *next_;
    LinkedList<T> *prev_;
    T data_;
};

}  // namespace OHOS

#endif  // LIBORPHANMEM_ALLOCATOR_LINKED_LIST_H
