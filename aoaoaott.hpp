/*
 * Copyright (c) 2019 Pavel I. Kryukov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef AO_AO_AO_TT
#define AO_AO_AO_TT

#include <memory>
#include <vector>

template<typename T, typename = std::enable_if_t<std::is_trivial<T>::value>>
class AoS {
    struct Iface : private T
    {
        T& operator=(const T& rhs) {
            T::operator=(rhs);
            return *this;
        }
        T& operator=(T&& rhs) {
            T::operator=(std::move(rhs));
            return *this;
        }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        const auto& get() const
        {
            return this->*ptr;
        }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        auto& get()
        {
            return this->*ptr;
        }

        template<typename R>
        R& operator->*(R T::* field)
        {
            return this->*field;
        }

        template<typename R>
        const R& operator->*(R T::* field) const
        {
            return this->*field;
        }
    };
    std::vector<Iface> storage;
public:
    AoS() { }
    AoS(std::size_t size) : storage(size) { }
    void resize(std::size_t size) { storage.resize( size); }

    Iface& operator[](std::size_t index) { return storage[index]; }
    const Iface& operator[](std::size_t index) const { return storage[index]; }
};

template<typename T, typename = std::enable_if_t<std::is_trivial<T>::value>>
class SoA {
    std::vector<char> storage;
    std::size_t size;
    class BaseIface {
    private:
        size_t index;
        size_t size;
        template<typename R>
        static constexpr auto member_offset(R T::* member)
        {
            return reinterpret_cast<std::ptrdiff_t>(
                  &(reinterpret_cast<T const volatile*>(NULL)->*member)
            );
        }
    protected:
        BaseIface(std::size_t index, std::size_t size) : index(index), size(size) { }
        template<typename Class, typename Type> static Type get_pointer_type(Type Class::*);

        template<typename R>
        std::size_t get_offset(R T::* member) const
        {
            return member_offset(member) * size + index * sizeof(R);
        }
        
        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        std::size_t get_offset_template() const
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return member_offset(ptr) * size + index * sizeof(Type);
        }
    };

    class Iface : private BaseIface
    {
        SoA<T>* base;
    public:
        Iface( SoA<T>* b, std::size_t index, std::size_t size) : BaseIface(index, size), base(b) { }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        auto& get()
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return *reinterpret_cast<Type*>(base->storage.data() + this->template get_offset_template<ptr>());
        }

        template<typename R>
        R& operator->*(R T::* field)
        {
            return *reinterpret_cast<R*>(base->storage.data() + this->get_offset(field));
        }
    };

    class ConstIface : private BaseIface
    {
        const SoA<T>* base;
    public:
        ConstIface( const SoA<T>* b, std::size_t index, std::size_t size) : BaseIface(index, size), base(b) { }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        const auto& get() const
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return *reinterpret_cast<const Type*>(base->storage.data() + this->template get_offset_template<ptr>());
        }

        template<typename R>
        const R& operator->*(R T::* field) const
        {
            return *reinterpret_cast<const R*>(base->storage.data() + this->get_offset(field));
        }
    };

    friend class Iface;
    friend class ConstIface;
public:
    SoA() = default;
    SoA(std::size_t s) : storage(s * sizeof(T)), size(s) { }

    Iface operator[](std::size_t index) { return Iface{ this, index, size}; }
    ConstIface operator[](std::size_t index) const { return ConstIface{ this, index, size}; }
};

#endif
