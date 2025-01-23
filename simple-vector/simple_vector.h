#pragma once

#include <iostream>
#include <cassert>
#include <utility>
#include <initializer_list>

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

    explicit SimpleVector(size_t size) : my_vector(size)
    {
        capacity_ = size;
        size_ = size;
        std::fill(begin(), end(), Type());
    }

    SimpleVector(size_t size, const Type &value) : my_vector(size)
    {
        capacity_ = size;
        size_ = size;
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init) : my_vector(init.size()), size_(init.size()), capacity_(init.size())
    {
        if (capacity_ > 0)
        {
            std::copy(init.begin(), init.end(), my_vector.Get());
        }
    }

    SimpleVector(const SimpleVector &other)
    {
        assert(size_ == 0);
        SimpleVector<Type> cop(other.GetSize());
        std::copy((other.my_vector).Get(), ((other.my_vector).Get() + other.GetSize()), (cop.my_vector).Get());
        cop.capacity_ = other.capacity_;
        swap(cop);
    }

    SimpleVector(SimpleVector &&v) noexcept : size_(std::exchange(v.size_, 0)), capacity_(std::exchange(v.capacity_, 0))
    {
        my_vector.swap(v.my_vector);
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
            auto rhs_copy(rhs);
            swap(rhs_copy);
        }

        return *this;
    }

    SimpleVector &operator=(SimpleVector &&rhs) noexcept
    {
        if (this != &rhs)
        {
            my_vector.swap(rhs.my_vector);
            size_ = std::exchange(rhs.size_, 0);
            capacity_ = std::exchange(rhs.capacity_, 0);
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
        return my_vector[index];
    }

    const Type &operator[](size_t index) const noexcept
    {
        return my_vector[index];
    }

    Type &At(size_t index)
    {
        return (index >= size_) ? throw std::out_of_range("index >= size") : my_vector[index];
    }

    const Type &At(size_t index) const
    {
        return (index >= size_) ? throw std::out_of_range("index >= size") : my_vector[index];
    }

    void Clear() noexcept
    {
        size_ = 0;
    }

    void Resize(size_t new_size)
    {
        if (new_size <= size_)
        {
            size_ = new_size;
        }
        else if (new_size <= capacity_)
        {
            for (auto it = begin() + size_; it != begin() + new_size; ++it)
            {
                *it = std::move(Type());
            }

            size_ = new_size;
        }
        else
        {
            ArrayPtr<Type> temp_v(new_size);
            for (auto it = temp_v.Get(); it != temp_v.Get() + size_; ++it)
            {
                *it = std::move(*(begin() + (it - temp_v.Get())));
            }
            for (auto it = temp_v.Get() + size_; it != temp_v.Get() + new_size; ++it)
            {
                *it = std::move(Type());
            }
            my_vector.swap(temp_v);
            capacity_ = size_ = new_size;
        }
    }

    Iterator begin() noexcept
    {
        return my_vector.Get();
    }

    Iterator end() noexcept
    {
        return my_vector.Get() + size_;
    }

    ConstIterator begin() const noexcept
    {
        return my_vector.Get();
    }

    ConstIterator end() const noexcept
    {
        return my_vector.Get() + size_;
    }

    ConstIterator cbegin() const noexcept
    {
        return my_vector.Get();
    }

    ConstIterator cend() const noexcept
    {
        return my_vector.Get() + size_;
    }

    void PushBack(const Type &item)
    {
        if (size_ < capacity_)
        {
            my_vector[size_++] = item;
        }
        else
        {
            ArrayPtr<Type> temp_v((capacity_ != 0) ? (2 * capacity_) : 1);
            std::copy(begin(), end(), temp_v.Get());
            std::fill(temp_v.Get() + size_, temp_v.Get() + capacity_, Type());
            temp_v[size_++] = item;
            my_vector.swap(temp_v);
            capacity_ = (capacity_ != 0) ? (2 * capacity_) : 1;
        }
    }

    void PushBack(Type &&item)
    {
        if (size_ < capacity_)
        {
            my_vector[size_++] = std::move(item);
        }
        else
        {
            ArrayPtr<Type> temp_v((capacity_ != 0) ? (2 * capacity_) : 1);
            for (size_t i = 0; i < size_; ++i)
            {
                temp_v[i] = std::move(my_vector[i]);
            }
            for (size_t i = size_; i < capacity_; ++i)
            {
                temp_v[i] = std::move(Type());
            }
            temp_v[size_++] = std::move(item);
            my_vector.swap(temp_v);
            capacity_ = (capacity_ != 0) ? (2 * capacity_) : 1;
        }
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
            auto number = pos - my_vector.Get();
            ArrayPtr<Type> temp_v((capacity_ != 0) ? (2 * capacity_) : 1);

            std::copy(begin(), end(), temp_v.Get());
            std::fill(temp_v.Get() + size_, temp_v.Get() + capacity_, Type());
            std::copy_backward(temp_v.Get() + number, temp_v.Get() + capacity_, temp_v.Get() + capacity_ + 1);
            temp_v[number] = value;
            my_vector.swap(temp_v);
            ++size_;
            capacity_ = (capacity_ != 0) ? (2 * capacity_) : 1;
            return Iterator(my_vector.Get() + number);
        }
    }

    Iterator Insert(ConstIterator pos, Type &&value)
    {
        if (size_ < capacity_)
        {
            auto it = Iterator(pos);
            for (int i = (end() - begin() + 1); i > Iterator(pos) - begin(); --i)
            {
                my_vector[i] = std::move(my_vector[i - 1]);
            }
            my_vector[it - begin()] = std::move(value);
            ++size_;
            return Iterator(pos);
        }
        else
        {
            auto number = pos - my_vector.Get();
            ArrayPtr<Type> temp_v((capacity_ != 0) ? (2 * capacity_) : 1);
            for (size_t i = 0; i < size_; ++i)
            {
                temp_v[i] = std::move(my_vector[i]);
            }
            for (size_t i = size_; i < capacity_; ++i)
            {
                temp_v[i] = std::move(Type());
            }
            for (int i = static_cast<int>(capacity_ + 1); i > number; --i)
            {
                temp_v[i] = std::move(temp_v[i - 1]);
            }
            temp_v[number] = std::move(value);
            my_vector.swap(temp_v);
            ++size_;
            capacity_ = (capacity_ != 0) ? (2 * capacity_) : 1;
            return Iterator(my_vector.Get() + number);
        }
    }

    void PopBack() noexcept
    {
        assert(!IsEmpty());
        size_--;
    }

    Iterator Erase(ConstIterator pos)
    {
        auto aim = std::distance(cbegin(), pos);
        if (pos == begin() + size_)
        {
            size_--;
        }
        else
        {
            ArrayPtr<Type> temp(size_);
            temp[size_ - 1] = std::move(Type{});
            //  auto aim = pos - begin();
            for (auto i = 0; i < aim; ++i)
            {
                temp[i] = std::move(my_vector[i]);
            }
            for (auto i = static_cast<int>(size_ - 1); i > aim; --i)
            {
                temp[i - 1] = std::move(my_vector[i]);
            }
            my_vector.swap(temp);
            size_--;
        }
        return Iterator(cbegin() + aim);
    }

    void swap(SimpleVector &other) noexcept
    {
        my_vector.swap(other.my_vector);
        std::swap(other.size_, size_);
        std::swap(other.capacity_, capacity_);
    }

    void Reserve(size_t new_capacity)
    {
        if (capacity_ < new_capacity)
        {
            ArrayPtr<Type> temp_v(new_capacity);
            std::copy(begin(), end(), temp_v.Get());
            std::fill(temp_v.Get() + size_, temp_v.Get() + new_capacity, Type());
            my_vector.swap(temp_v);
            capacity_ = new_capacity;
        }
    }

private:
    ArrayPtr<Type> my_vector;
    size_t size_ = 0;
    size_t capacity_ = 0;
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