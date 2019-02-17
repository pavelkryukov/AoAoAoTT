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

namespace struct_reader {
    // Based on public domain code authored by Alexandr Poltavsky
    // https://github.com/alexpolt/luple
    template<typename... TT> struct type_list {};
    template<typename T, int N, int M = 0> struct tlist_get;
    template<int N, int M, typename T, typename... TT> struct tlist_get< type_list<T, TT...>, N, M > {
        static_assert(N < (int) sizeof...(TT)+1 + M, "type index out of bounds");
        using type = std::conditional_t< N == M, T, typename tlist_get< type_list<TT...>, N, M + 1 >::type >;
    };

    template<int N, int M> struct tlist_get< type_list<>, N, M > { using type = void; };
    template<int N> struct tlist_get< type_list<>, N, 0 > {};
    template<typename T, int N> using tlist_get_t = typename tlist_get<T, N>::type;

    template<typename T, typename U, int N = 0> struct tlist_get_n;
    template<typename U, int N, typename T, typename... TT> struct tlist_get_n< type_list<T, TT...>, U, N > {
        static const int value = std::is_same< T, U >::value ? N : tlist_get_n< type_list<TT...>, U, N + 1 >::value;
    };
    template<typename U, int N> struct tlist_get_n< type_list<>, U, N > {
        static const int value = -1;
    };

    //this is the main type list, add your own types here
    using type_list_t = type_list<
                        void *, bool, char, unsigned char, signed char, short, int, long, long long, 
                        unsigned short, unsigned int, unsigned long, unsigned long long, 
                        float, double, long double
                    >;

    //helper to get type using a templated conversion operator
    template<typename T>
    struct read_type {
        template<typename U>
        constexpr operator U() {
            using noptr = std::remove_pointer_t<U>;
            using nocon = std::remove_const_t<noptr>;
            static_assert( tlist_get_n<T, U>::value != -1 || tlist_get_n<T, nocon>::value != -1, "no such type in type list");
            constexpr int const tid = 0xFFFF, is_ptr = 1 << 16, is_con = 1 << 17;
            data = tlist_get_n<T, U>::value;
            if( data == -1 ) {
                data = tlist_get_n<T, nocon>::value & tid;
                data = data | (std::is_pointer<U>::value ? is_ptr : 0);
                data = data | (std::is_const<noptr>::value ? is_con : 0);
            }
            return {};
        }
        int data;
    };

    using read_type_t = read_type< type_list_t >;

    //here we're using overload resolution to get a data member type
    template<typename T, int... N>
    constexpr auto get_type_id(int n) {
        read_type_t tid[sizeof...(N)]{};
        T d = T{ tid[N]... }; (void)d;
        return tid[n].data;
    }

    //helper to rebuild the type
    template<typename T, int tid, int is_pointer, int is_const>
    constexpr auto get_type() {
        using type = tlist_get_t<T, tid>;
        using ctype = std::conditional_t< (bool)is_const, std::add_const_t<type>, type >;
        using ptype = std::conditional_t< (bool)is_pointer, std::add_pointer_t<ctype>, ctype >;
        return ptype{};
    }

    //read struct data member types and put it into a type list
    template<typename T, int... N>
    constexpr auto get_type_list(std::integer_sequence<int, N...>) {
        constexpr int t[] = { get_type_id<T, N...>(N)... };
        constexpr int const tid = 0xFFFF, is_ptr = 1 << 16, is_con = 1 << 17;
        return type_list< decltype(get_type<type_list_t, t[N]&tid, t[N]&is_ptr, t[N]&is_con>())...>{};
    }

    //get fields number using expression SFINAE
    template<typename T, int... N>
    constexpr int fields_number(...) { return sizeof...(N)-1; }

    template<typename T, int... N>
    constexpr auto fields_number(int) -> decltype(T{ (N,read_type_t{})... }, sizeof(0)) { return fields_number<T, N..., 0>(0); }

    //and here is our hot and fresh out of kitchen type list (alias template)
    template<typename T>
    using as_type_list = decltype(get_type_list< T >(std::make_integer_sequence< int, fields_number<T>(0) >{}));
}

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
    SoA(std::size_t s) : size(s) { resize(s); }
    void resize(std::size_t s) { storage.resize(s * sizeof(T)); }

    Iface operator[](std::size_t index) { return Iface{ this, index, size}; }
    ConstIface operator[](std::size_t index) const { return ConstIface{ this, index, size}; }
};

#endif
