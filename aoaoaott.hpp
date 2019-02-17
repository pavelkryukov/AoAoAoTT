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

namespace ao_ao_ao_tt {

namespace loophole_ns {
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

    /*
        tag<T,N> generates friend declarations and helps with overload resolution.
        There are two types: one with the auto return type, which is the way we read types later.
        The second one is used in the detection of instantiations without which we'd get multiple
        definitions.
    */
    template<typename T, int N>
    struct tag {
        friend auto loophole(tag<T,N>);
        constexpr friend int cloophole(tag<T,N>);
    };


    /*
        The definitions of friend functions.
    */
    template<typename T, typename U, int N, bool B>
    struct fn_def {
        friend auto loophole(tag<T,N>) { return U{}; }
        constexpr friend int cloophole(tag<T,N>) { return 0; }
    };

    /*
        This specialization is to avoid multiple definition errors.
    */
    template<typename T, typename U, int N>
    struct fn_def<T, U, N, true> {};

    /*
        This has a templated conversion operator which in turn triggers instantiations.
        Important point, using sizeof seems to be more reliable. Also default template
        arguments are "cached" (I think). To fix that I provide a U template parameter to
        the ins functions which do the detection using constexpr friend functions and SFINAE.
    */
    template<typename T, int N>
    struct c_op {
        template<typename U, int M> static auto ins(...) -> int;
        template<typename U, int M, int = cloophole(tag<T,M>{}) > static auto ins(int) -> char;

        template<typename U, int = sizeof(fn_def<T, U, N, sizeof(ins<U, N>(0)) == sizeof(char)>)>
        operator U();
    };

    /*
        Here we detect the data type field number. The byproduct is instantiations.
        Uses list initialization. Won't work for types with user provided constructors.
        In C++17 there is std::is_aggregate which can be added later.
    */

    template<typename T, int... NN>
    constexpr int fields_number(...) { return sizeof...(NN)-1; }

    template<typename T, int... NN>
    constexpr auto fields_number(int) -> decltype(T{ c_op<T,NN>{}... }, 0) {
        return fields_number<T, NN..., sizeof...(NN)>(0);
    }

    /*
        This is a helper to turn a data structure into a type list.
        Usage is: loophole_ns::as_type_list< data_t >
        I keep dependency on luple (a lightweight tuple of my design) because it's handy
        to turn a structure into luple (tuple). luple has the advantage of more stable layout
        across compilers and we can reinterpret_cast between the data structure and luple.
        More details are in the luple.h header.
    */

    template<typename T, typename U>
    struct loophole_type_list;

    template<typename T, int... NN>
    struct loophole_type_list< T, std::integer_sequence<int, NN...> > {
        using type = type_list< decltype(loophole(tag<T, NN>{}))... >;
    };

    template<typename T>
    using as_type_list =
        typename loophole_type_list<T, std::make_integer_sequence<int, fields_number<T>(0)>>::type;

} // namespace loophole_ns

namespace tlist_helpers
{
    using namespace loophole_ns;

    template<typename TL, size_t N>
    constexpr std::size_t nth_member_offset = sizeof(tlist_get_t<TL, N - 1>) + nth_member_offset<TL, N - 1>;

    template<typename TL>
    constexpr std::size_t nth_member_offset<TL, 0> = 0;

    template<typename T, size_t N>
    class copy_n_members_to_storage
    {
        using TL = as_type_list<T>;
        using R = tlist_get_t<TL, N - 1>;
        static constexpr const size_t offset = nth_member_offset<TL, N - 1>;
        static constexpr const size_t sizeofR = sizeof(R);

        void copy_member(const T& src, char* dst, size_t index, size_t size)
            const noexcept(noexcept(std::is_nothrow_copy_assignable_v<R>))
        {
            *reinterpret_cast<R*>(dst + offset * size + index * sizeofR) =
                *reinterpret_cast<const R*>(reinterpret_cast<const char*>(&src) + offset);
        }
    public:
        void operator()(const T& src, char* dst, size_t index, size_t size)
            const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
        {
            copy_member(src, dst, index, size);
            copy_n_members_to_storage<T, N - 1>()(src, dst, index, size);
        }
    };

    template<typename T>
    struct copy_n_members_to_storage<T, 0>
    {
        void operator()(const T&, char*, size_t, size_t) const
            noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
        { }
    };

    template<typename T>
    void copy_all_members_to_storage(const T& src, char* dst, size_t index, size_t size)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        using TL = as_type_list<T>;
        copy_n_members_to_storage<T, TL::size>()(src, dst, index, size);
    }

    template<typename T, size_t N>
    class copy_n_members_from_storage
    {
        using TL = as_type_list<T>;
        using R = tlist_get_t<TL, N - 1>;
        static constexpr const size_t offset = nth_member_offset<TL, N - 1>;
        static constexpr const size_t sizeofR = sizeof(R);

        void copy_member(const char* src, T* dst, size_t index, size_t size)
            const noexcept(noexcept(std::is_nothrow_copy_assignable_v<R>))
        {
            *reinterpret_cast<R*>(reinterpret_cast<char*>(dst) + offset) =
                *reinterpret_cast<const R*>(src + offset * size + index * sizeofR);
        }
    public:
        void operator()(const char* src, T* dst, size_t index, size_t size)
            const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
        {
            copy_member(src, dst, index, size);
            copy_n_members_from_storage<T, N - 1>()(src, dst, index, size);
        }
    };

    template<typename T>
    struct copy_n_members_from_storage<T, 0>
    {
        void operator()(const char*, T*, size_t, size_t) const
            noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
        { }
    };

    template<typename T>
    void copy_all_members_from_storage(const char* src, T* dst, size_t index, size_t size)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        using TL = as_type_list<T>;
        copy_n_members_from_storage<T, TL::size>()(src, dst, index, size);
    }
} // namespace tlist_helpers

template<typename T>
class AoS {
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
public:
    class Iface;

    AoS() : AoS(0) { }
    explicit AoS(std::size_t size) : storage(size) { }
    AoS(std::size_t size, const T& value) : storage(size, Iface(value)) { }

    void resize(std::size_t size) { storage.resize( size); }
    void resize(std::size_t size, const T& value) { storage.resize( size, Iface(value)); }

    Iface& operator[](std::size_t index) noexcept { return storage[index]; }
    const Iface& operator[](std::size_t index) const noexcept { return storage[index]; }

    using iterator = typename std::vector<Iface>::iterator;
    using const_iterator = typename std::vector<Iface>::const_iterator;

    const_iterator begin() const noexcept { return storage.cbegin(); }
    const_iterator end() const noexcept { return storage.cend(); }
    const_iterator cbegin() const noexcept { return storage.cbegin(); }
    const_iterator cend() const noexcept { return storage.cend(); }
    iterator begin() noexcept { return storage.begin(); }
    iterator end() noexcept { return storage.end(); }

    class Iface : private T
    {
    public:
        Iface() : T() { }
        explicit Iface(const T& value) : T(value) { }
        explicit Iface(T&& value) : T(std::move(value)) { }

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

        T aggregate_object() const noexcept { return *this; }
    };
    std::vector<Iface> storage;
};

template<typename T>
class SoA {
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
public:
    class Iface;
    class ConstIface;

    SoA() : SoA(0) { }
    explicit SoA(std::size_t s) : size(0) { resize(s); }
    SoA(std::size_t s, const T& value) : size(0) { resize(s, value); }

    void resize(std::size_t s)
    {
        if constexpr(std::is_trivially_constructible_v<T>)
            resize_memory(s);
        else
            resize(s, T());
    }

    void resize(std::size_t s, const T& value)
    {
        size_t old_size = size;
        resize_memory(s);
        for (size_t i = old_size; i < size; ++i)
            Iface( this, i).copy_object(value);
    }

    Iface operator[](std::size_t index) noexcept { return Iface{ this, index}; }
    ConstIface operator[](std::size_t index) const noexcept { return ConstIface{ this, index}; }

    class BaseIface
    {
    public:
        T aggregate_object() const noexcept
        {
            T result{};
            tlist_helpers::copy_all_members_from_storage(base->storage.data(), &result, this->get_index(), this->get_size());
            return result;
        }
    private:
        const SoA<T>* base;
        size_t index;
        template<typename R>
        static constexpr auto member_offset(R T::* member) noexcept
        {
            return reinterpret_cast<std::ptrdiff_t>(
                  &(reinterpret_cast<T const volatile*>(NULL)->*member)
            );
        }
    protected:
        constexpr BaseIface(const SoA<T>* base, std::size_t index) : base(base), index(index) { }
        constexpr auto get_size()  const noexcept { return base->size; }
        constexpr auto get_index() const noexcept { return index; }
        constexpr const auto* get_base() const noexcept { return base; }
        void inc_index() noexcept { ++index; }

        template<typename Class, typename Type> static Type get_pointer_type(Type Class::*);

        template<typename R>
        std::size_t get_offset(R T::* member) const noexcept
        {
            return member_offset(member) * get_size() + index * sizeof(R);
        }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        std::size_t get_offset_template() const noexcept
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return member_offset(ptr) * get_size() + index * sizeof(Type);
        }
    };

    class ConstIface : public BaseIface
    {
    public:
        ConstIface( const SoA<T>* b, std::size_t index) : BaseIface(b, index) { }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        const auto& get() const noexcept
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return *reinterpret_cast<const Type*>(this->get_base()->storage.data() + this->template get_offset_template<ptr>());
        }

        template<typename R>
        const R& operator->*(R T::* field) const noexcept
        {
            return *reinterpret_cast<const R*>(this->get_base()->storage.data() + this->get_offset(field));
        }
    };

    class Iface : public BaseIface
    {
    public:
        Iface( SoA<T>* b, std::size_t index) : BaseIface(b, index), mutable_base(b) { }

        template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
        auto& get() const noexcept
        {
            using Type = decltype(this->get_pointer_type(ptr));
            return *reinterpret_cast<Type*>(mutable_base->storage.data() + this->template get_offset_template<ptr>());
        }

        template<typename R>
        R& operator->*(R T::* field) const noexcept
        {
            return *reinterpret_cast<R*>(mutable_base->storage.data() + this->get_offset(field));
        }

        void operator=(const T& rhs) const noexcept
        {
            static_assert(std::is_copy_assignable_v<T>, "Object cannot be assigned because its copy assignment operator is implicitly deleted");
            copy_object(rhs);
        }

        void operator=(T&& rhs) const noexcept
        {
            static_assert(std::is_move_assignable_v<T>, "Object cannot be assigned because its move assignment operator is implicitly deleted");
            move_object(std::move(rhs));
        }
    private:
        friend class SoA<T>;
        SoA<T>* mutable_base;

        void copy_object(const T& rhs) const noexcept
        {
            tlist_helpers::copy_all_members_to_storage(rhs, mutable_base->storage.data(), this->get_index(), this->get_size());
        }

        void move_object(T&& rhs) const noexcept
        {
            // Do not care about move semantics since we support only trivial structures so far
            tlist_helpers::copy_all_members_to_storage(rhs, mutable_base->storage.data(), this->get_index(), this->get_size());
        }
    };

    class iterator;
    class const_iterator : private ConstIface
    {
    public:
        const_iterator(const SoA<T>* base, std::size_t index) : ConstIface(base, index) { }

        bool operator!=(const const_iterator& rhs) { return this->get_index() != rhs.get_index(); }
        bool operator!=(const iterator& rhs) { return this->get_index() != rhs.get_index(); }

        const_iterator& operator++() { this->inc_index(); return *this; }

        ConstIface& operator*() { return *this; }
    };
    
    class iterator : private Iface
    {
    public:
        iterator(SoA<T>* base, std::size_t index) : Iface(base, index) { }

        bool operator!=(const iterator& rhs) { return this->get_index() != rhs.get_index(); }
        bool operator!=(const const_iterator& rhs) { return this->get_index() != rhs.get_index(); }

        iterator& operator++() { this->inc_index(); return *this; }

        Iface& operator*() { return *this; }
    };

    const_iterator cbegin() const noexcept { return const_iterator{ this, 0}; }
    const_iterator cend() const noexcept { return const_iterator{ this, size}; }
    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator end() const noexcept { return cend(); }
    iterator begin() noexcept { return iterator{ this, size}; }
    iterator end() noexcept { return iterator{ this, size}; }
private:
    std::vector<char> storage;
    std::size_t size;

    friend class Iface;
    friend class ConstIface;

    void resize_memory(std::size_t s)
    {
        storage.resize(s * sizeof(T));
        size = s;
    }
};

} // namespace ao_ao_ao_tt

#endif
