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
    template<typename... TT> struct type_list { static constexpr size_t size = sizeof...(TT); };
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

    static constexpr int const tid = 0xFFFF;
    static constexpr int const is_ptr = 1 << 16;
    static constexpr int const is_con = 1 << 17;

    //read struct data member types and put it into a type list
    template<typename T, int... N>
    constexpr auto get_type_list(std::integer_sequence<int, N...>) {
        constexpr int t[] = { get_type_id<T, N...>(N)... };
        (void)t; // maybe unused if N == 0
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

namespace tlist_helpers
{
    using namespace struct_reader;

    template<typename TL, size_t N>
    constexpr std::size_t nth_member_offset = sizeof(tlist_get_t<TL, N - 1>) + nth_member_offset<TL, N - 1>;

    template<typename TL>
    constexpr std::size_t nth_member_offset<TL, 0> = 0;

    template<typename T, size_t N>
    void copy_nth_member(const T& src, char* dst, size_t index, size_t size)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<tlist_get_t<as_type_list<T>, N>>))
    {
        using TL = as_type_list<T>;
        using R = tlist_get_t<TL, N>;
        *reinterpret_cast<R*>(dst + nth_member_offset<TL, N> * size + index * sizeof(R)) =
            *reinterpret_cast<const R*>(reinterpret_cast<const char*>(&src) + nth_member_offset<TL, N>);
    }

    template<typename T, size_t N>
    struct copy_n_members
    {
        void operator()(const T& src, char* dst, size_t index, size_t size) const
            noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
        {
            copy_nth_member<T, N - 1>(src, dst, index, size);
            copy_n_members<T, N - 1>()(src, dst, index, size);
        }
    };

    template<typename T>
    struct copy_n_members<T, 0>
    {
        void operator()(const T&, char*, size_t, size_t) const
            noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
        { }
    };

    template<typename T>
    void copy_all_members(const T& src, char* dst, size_t index, size_t size)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        using TL = as_type_list<T>;
        copy_n_members<T, TL::size>()(src, dst, index, size);
    }
}

template<typename T>
class AoS {
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
    struct Iface : private T
    {
        T& operator=(const T& rhs) noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
        {
            T::operator=(rhs);
            return *this;
        }

        T& operator=(T&& rhs) noexcept(noexcept(std::is_nothrow_move_assignable_v<T>))
        {
            T::operator=(std::move(rhs));
            return *this;
        }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        const auto& get() const noexcept
        {
            return this->*ptr;
        }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        auto& get() noexcept
        {
            return this->*ptr;
        }

        template<typename R>
        R& operator->*(R T::* field) noexcept
        {
            return this->*field;
        }

        template<typename R>
        const R& operator->*(R T::* field) const noexcept
        {
            return this->*field;
        }
    };
    std::vector<Iface> storage;
public:
    AoS() { }
    AoS(std::size_t size) : storage(size) { }
    void resize(std::size_t size) { storage.resize( size); }

    Iface& operator[](std::size_t index) noexcept { return storage[index]; }
    const Iface& operator[](std::size_t index) const noexcept { return storage[index]; }
};

template<typename T>
class SoA {
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");

    std::vector<char> storage;
    std::size_t size;
    class BaseIface {
    private:
        size_t index;
        size_t size;
        template<typename R>
        static constexpr auto member_offset(R T::* member) noexcept
        {
            return reinterpret_cast<std::ptrdiff_t>(
                  &(reinterpret_cast<T const volatile*>(NULL)->*member)
            );
        }
    protected:
        BaseIface(std::size_t index, std::size_t size) : index(index), size(size) { }
        constexpr auto get_size()  const noexcept { return size; }
        constexpr auto get_index() const noexcept { return index; }
        
        template<typename Class, typename Type> static Type get_pointer_type(Type Class::*);

        template<typename R>
        std::size_t get_offset(R T::* member) const noexcept
        {
            return member_offset(member) * size + index * sizeof(R);
        }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        std::size_t get_offset_template() const noexcept
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
        auto& get() const noexcept
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return *reinterpret_cast<Type*>(base->storage.data() + this->template get_offset_template<ptr>());
        }

        template<typename R>
        R& operator->*(R T::* field) const noexcept
        {
            return *reinterpret_cast<R*>(base->storage.data() + this->get_offset(field));
        }

        void operator=(const T& rhs) const noexcept
        {
            tlist_helpers::copy_all_members(rhs, base->storage.data(), this->get_index(), this->get_size());
        }

        void operator=(T&& rhs) const noexcept
        {
            // Do not care about move semantics since we support only trivial structures so far
            tlist_helpers::copy_all_members(rhs, base->storage.data(), this->get_index(), this->get_size());
        }
    };

    class ConstIface : private BaseIface
    {
        const SoA<T>* base;
    public:
        ConstIface( const SoA<T>* b, std::size_t index, std::size_t size) : BaseIface(index, size), base(b) { }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        const auto& get() const noexcept
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return *reinterpret_cast<const Type*>(base->storage.data() + this->template get_offset_template<ptr>());
        }

        template<typename R>
        const R& operator->*(R T::* field) const noexcept
        {
            return *reinterpret_cast<const R*>(base->storage.data() + this->get_offset(field));
        }
    };

    friend class Iface;
    friend class ConstIface;
public:
    SoA() = default;
    SoA(std::size_t s) : size(0) { resize(s); }
    void resize(std::size_t s)
    {
        size_t old_size = size;
        storage.resize(s * sizeof(T));
        size = s;
        if constexpr (!std::is_trivially_constructible_v<T>)
            for (size_t i = old_size; i < size; ++i)
                Iface( this, i, size) = T();
        else
            (void)old_size;
    }

    Iface operator[](std::size_t index) noexcept { return Iface{ this, index, size}; }
    ConstIface operator[](std::size_t index) const noexcept { return ConstIface{ this, index, size}; }
};

#endif
