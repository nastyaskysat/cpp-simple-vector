#pragma once

#include <iostream>
#include <cassert>
#include <utility>
#include <initializer_list>
#include <algorithm>

#include "array_ptr.h"

class ReserveProxyObj
{
public:
    ReserveProxyObj(size_t res_capacity) : res_capacity_(res_capacity) {}
    size_t GetCapacity() const
    {
        return res_capacity_;
    }

private:
    size_t res_capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve)
{
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector
{
public:
    using Iterator = Type *;
    using ConstIterator = const Type *;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size)
        : items_(size),
          size_(size),
          capacity_(size)

    {
        std::fill(begin(), end(), Type());
    }

    SimpleVector(size_t size, const Type &value)
        : items_(size),
          size_(size),
          capacity_(size)
    {
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init)
        : items_(init.size()),
          size_(init.size()),
          capacity_(init.size())
    {
        if (capacity_ > 0)
        {
            std::copy(init.begin(), init.end(), items_.Get());
        }
    }

    SimpleVector(const SimpleVector &other)
        : items_(other.GetSize()),
          size_(other.size_),
          capacity_(other.capacity_)

    {
        std::copy(other.items_.Get(), other.items_.Get() + other.GetSize(), items_.Get());
    }

    SimpleVector(SimpleVector &&v) noexcept : size_(std::exchange(v.size_, 0)), capacity_(std::exchange(v.capacity_, 0))
    {
        items_.swap(v.items_);
    }

    SimpleVector(const ReserveProxyObj &obj) : capacity_(obj.GetCapacity())
    {
        if (capacity_ > 0)
        {
            std::fill(begin(), end(), Type());
        }
    }

    SimpleVector &operator=(const SimpleVector &rhs)
    {
        if (this != &rhs)
        {
            if (rhs.size_ == 0)
            {
                Clear();
                size_ = 0;
                capacity_ = 0;
            }
            else
            {
                auto rhs_copy(rhs);
                swap(rhs_copy);
            }
        }
        return *this;
    }

    SimpleVector &operator=(SimpleVector &&rhs) noexcept
    {
        if (this != &rhs)
        {
            if (rhs.size_ == 0)
            {
                Clear();
                size_ = 0;
                capacity_ = 0;
            }
            else
            {
                items_.swap(rhs.items_);
                size_ = std::exchange(rhs.size_, 0);
                capacity_ = std::exchange(rhs.capacity_, 0);
            }
        }
        return *this;
    }
    size_t GetSize() const noexcept
    {
        return size_;
    }

    size_t GetCapacity() const noexcept
    {
        return capacity_;
    }

    bool IsEmpty() const noexcept
    {
        return size_ == 0;
    }

    Type &operator[](size_t index) noexcept
    {
        return items_[index];
    }

    const Type &operator[](size_t index) const noexcept
    {
        return items_[index];
    }

    Type &At(size_t index)
    {
        return (index >= size_) ? throw std::out_of_range("index >= size") : items_[index];
    }

    const Type &At(size_t index) const
    {
        return (index >= size_) ? throw std::out_of_range("index >= size") : items_[index];
    }

    void Clear() noexcept
    {
        size_ = 0;
    }

    void Resize(size_t new_size)
    {
        if (size_ < new_size)
        {
            if (new_size <= capacity_)
                std::fill(items_.Get() + size_, items_.Get() + new_size, Type());
            else
                ReCapacity(new_size * 2);
        }
        size_ = new_size;
    }

    Iterator begin() noexcept
    {
        return items_.Get();
    }

    Iterator end() noexcept
    {
        return items_.Get() + size_;
    }

    ConstIterator begin() const noexcept
    {
        return items_.Get();
    }

    ConstIterator end() const noexcept
    {
        return items_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept
    {
        return items_.Get();
    }

    ConstIterator cend() const noexcept
    {
        return items_.Get() + size_;
    }

    void PushBack(const Type &item)
    {
        if (size_ >= capacity_)
        {
            Reserve((capacity_ != 0) ? (2 * capacity_) : 1);
        }
        items_[size_++] = item;
    }

    void PushBack(Type &&item)
    {
        if (size_ >= capacity_)
        {
            Reserve((capacity_ != 0) ? (2 * capacity_) : 1);
        }
        items_[size_++] = std::move(item);
    }

    Iterator Insert(ConstIterator pos, const Type &value)
    {
        if (size_ < capacity_)
        {
            Iterator it = Iterator(pos);
            std::copy_backward(Iterator(pos), end(), end() + 1);
            *it = value;
            ++size_;
            return Iterator(pos);
        }
        else
        {
            auto index = pos - cbegin();
            Reserve(size_ + 1);
            std::move_backward(items_.Get() + index, items_.Get() + size_, items_.Get() + size_ + 1);
            items_.Get()[index] = value;
            ++size_;
            return Iterator(items_.Get() + index);
        }
    }

    Iterator Insert(ConstIterator pos, Type &&value)
    {
        if (size_ < capacity_)
        {
            Iterator it = begin() + (pos - cbegin());
            std::move_backward(it, end(), end() + 1);
            *it = std::move(value);
            ++size_;
            return it;
        }
        else
        {
            auto index = pos - cbegin();
            Reserve(size_ + 1);
            std::move_backward(items_.Get() + index, items_.Get() + size_, items_.Get() + size_ + 1);
            items_.Get()[index] = std::move(value);
            ++size_;
            return Iterator(items_.Get() + index);
        }
    }

    Iterator Erase(ConstIterator pos)
    {
        auto index = std::distance(cbegin(), pos);
        if (pos == begin() + size_)
        {
            size_--;
        }
        else
        {
            ArrayPtr<Type> temp(size_);
            temp[size_ - 1] = std::move(Type{});
            //  auto index = pos - begin();
            for (auto i = 0; i < index; ++i)
            {
                temp[i] = std::move(items_[i]);
            }
            for (auto i = static_cast<int>(size_ - 1); i > index; --i)
            {
                temp[i - 1] = std::move(items_[i]);
            }
            items_.swap(temp);
            size_--;
        }
        return Iterator(cbegin() + index);
    }

    void swap(SimpleVector &other) noexcept
    {
        items_.swap(other.items_);
        std::swap(other.size_, size_);
        std::swap(other.capacity_, capacity_);
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity > capacity_)
        {
            ReCapacity(new_capacity);
        }
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;

    void ReCapacity(size_t new_capacity)
    {
        ArrayPtr<Type> new_cont_(new_capacity);
        std::move(items_.Get(), items_.Get() + size_, new_cont_.Get());
        items_.swap(new_cont_);
        capacity_ = new_capacity;
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return (lhs.GetSize() == rhs.GetSize()) ? std::equal(lhs.begin(), lhs.end(), rhs.begin()) : false;
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type> &lhs, const SimpleVector<Type> &rhs)
{
    return !(lhs < rhs);
}
