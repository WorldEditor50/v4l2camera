#ifndef REALLOCATOR_HPP
#define REALLOCATOR_HPP
#include <QMap>
#include <QVector>

template <typename T>
class Reallocator
{
public:
    using Pointer = T*;
private:
    static QMap<unsigned long, QVector<T*> > arrayPool;
    static QVector<T*> elementPool;
private:
    Reallocator() = default;
    ~Reallocator()
    {
        clear();
    }
public:

    static T* get(unsigned long size_)
    {
        if (size_ == 0) {
            return nullptr;
        }
        T* ptr = nullptr;
        if (arrayPool.find(size_) == arrayPool.end()) {
            ptr = new T[size_];
        } else {
            ptr = arrayPool[size_].back();
            arrayPool[size_].pop_back();
        }
        return ptr;
    }
    static void recycle(unsigned long size_, Pointer &ptr)
    {
        if (size_ == 0 || ptr == nullptr) {
            return;
        }
        arrayPool[size_].push_back(ptr);
        ptr = nullptr;
        return;
    }
    static T* get()
    {
        T* ptr = nullptr;
        if (elementPool.empty()) {
            ptr = new T;
        } else {
            ptr = elementPool.back();
            elementPool.pop_back();
        }
        return ptr;
    }
    static void recycle(Pointer &ptr)
    {
        if (ptr == nullptr) {
            return;
        }
        elementPool.push_back(ptr);
        ptr = nullptr;
        return;
    }
    static void clear()
    {
        for (auto& block : arrayPool) {
            for (int i = 0; i < block.size(); i++) {
                T* ptr = block.at(i);
                delete [] ptr;
            }
            block.clear();
        }
        arrayPool.clear();
        for (int i = 0; i < elementPool.size(); i++) {
            T* ptr = elementPool.at(i);
            delete ptr;
        }
        elementPool.clear();
        return;
    }
};

template <typename T>
QMap<unsigned long, QVector<T*> > Reallocator<T>::arrayPool;
template <typename T>
QVector<T*> Reallocator<T>::elementPool;
#endif // REALLOCATOR_HPP
