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

#include <boost/iterator/iterator_facade.hpp>
#include <array>
#include <vector>

namespace ao_ao_ao_tt {

template<typename T>
class AoSFacade : private T
{
public:
    AoSFacade() : T() { }
    explicit AoSFacade(const T& value) : T(value) { }
    explicit AoSFacade(T&& value) : T(std::move(value)) { }

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

    T aggregate() const noexcept { return *this; }

    template<auto ptr, typename ... Args>
    auto method(Args&& ... args) // noexcept?
    {
        static_assert(std::is_member_function_pointer_v<decltype(ptr)>, "'method' should use member function pointers");
        return (this->*ptr)(std::forward<Args>(args)...);
    }

    template<auto ptr, typename ... Args>
    auto method(Args&& ... args) const // noexcept?
    {
        static_assert(std::is_member_function_pointer_v<decltype(ptr)>, "'method' should use member function pointers");
        return (this->*ptr)(std::forward<Args>(args)...);
    }

    template<typename R, typename ... Args>
    auto operator->*(R (T::* fun)(Args ...)) noexcept
    {
        return [this, fun](Args&& ... args){
            return (this->*fun)(std::forward<Args>(args)...);
        };
    }

    template<typename R, typename ... Args>
    auto operator->*(R (T::* fun)(Args ...) const) const noexcept
    {
        return [this, fun](Args&& ... args){
            return (this->*fun)(std::forward<Args>(args)...);
        };
    }
};

template<typename ContainerT>
class AoSRandomAccessContainer
{
public:
    template<typename ... Args>
    explicit AoSRandomAccessContainer(Args&& ... args) : storage(std::forward<Args>(args)...) { }

    auto& operator[](std::size_t index) noexcept { return storage[index]; }
    const auto& operator[](std::size_t index) const noexcept { return storage[index]; }

    using iterator = typename ContainerT::iterator;
    using const_iterator = typename ContainerT::const_iterator;
    using reverse_iterator = typename ContainerT::reverse_iterator;
    using const_reverse_iterator = typename ContainerT::const_reverse_iterator;

    auto begin() const noexcept { return storage.cbegin(); }
    auto end() const noexcept { return storage.cend(); }
    auto cbegin() const noexcept { return storage.cbegin(); }
    auto cend() const noexcept { return storage.cend(); }
    auto begin() noexcept { return storage.begin(); }
    auto end() noexcept { return storage.end(); }

    auto rbegin() const noexcept { return storage.crbegin(); }
    auto rend() const noexcept { return storage.crend(); }
    auto crbegin() const noexcept { return storage.crbegin(); }
    auto crend() const noexcept { return storage.crend(); }
    auto rbegin() noexcept { return storage.rbegin(); }
    auto rend() noexcept { return storage.rend(); }
protected:
    ContainerT storage; 
};

template<typename T, typename Allocator = std::allocator<T>>
class AoSVector : public AoSRandomAccessContainer<std::vector<AoSFacade<T>, Allocator>>
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
    using Base = AoSRandomAccessContainer<std::vector<AoSFacade<T>, Allocator>>;
public:
    AoSVector() : AoSVector(0) { }
    explicit AoSVector(std::size_t size) : Base(size) { }
    AoSVector(std::size_t size, const T& value) : Base(size, AoSFacade(value)) { }

    void resize(std::size_t size)
    {
        if (!this->storage.empty())
            throw std::runtime_error("SoARandomAccessContainer cannot be sized twice.");

        this->storage.resize(size);
    }

    void resize(std::size_t size, const T& value)
    {
        if (!this->storage.empty())
            throw std::runtime_error("SoARandomAccessContainer cannot be sized twice.");

        this->storage.resize(size, AoSFacade(value));
    }
};

template<typename T, size_t N>
class AoSArray : public AoSRandomAccessContainer<std::array<AoSFacade<T>, N>>
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
    using Base = AoSRandomAccessContainer<std::array<AoSFacade<T>, N>>;
public:
    AoSArray() = default;
    explicit AoSArray(const T& value) : Base(AoSFacade(value)) { }
};

namespace member_offset_helpers
{
    template<typename R, typename T>
    static constexpr auto member_offset(R T::* member) noexcept
    {
        return reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<T const volatile*>(NULL)->*member));
    }

    static inline constexpr std::size_t get_offset(size_t member_offset, size_t member_size, size_t size, size_t index) noexcept
    {
        return member_offset * size + index * member_size;
    }

    template<typename R, typename T>
    constexpr std::size_t get_offset(R T::* member, size_t size, size_t index) noexcept
    {
        return get_offset(member_offset(member), sizeof(R), size, index);
    }
} // namespace member_offset_helpers

namespace copy_helpers
{
    template<typename T>
    void copy_all_members_to_storage(const T& src, char* dst, size_t index, size_t size)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>));

    template<typename T>
    void copy_all_members_from_storage(const char* src, T* dst, size_t index, size_t size)
        noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>));
} // namespace member_offset_helpers
   
struct BaseFixedSize
{
    size_t get_runtime_size() const noexcept
    {
        assert(0);
        return 0;
    }
};

template<size_t N>
struct FixedSize : BaseFixedSize
{
    static constexpr const size_t size = N;
    static constexpr size_t get_constexpr_size() noexcept { return N; }
};

struct VariableSize
{
    size_t size = 0;
    static constexpr size_t get_constexpr_size() noexcept { assert(0); return N; }
    sise_t get_runtime_size() const noexcept { return size; }
};
    
template<typename T, typename Container>
class BaseSoAFacade
{
public:
    T aggregate() const noexcept
    {
        return base->aggregate(this->get_index());
    }

    template<auto ptr, typename ... Args>
    auto method(Args&& ... args) const // noexcept?
    {
        static_assert(std::is_member_function_pointer_v<decltype(ptr)>, "'method' should use member function pointers");
        object_mover om(this);
        return (om.object.*ptr)(std::forward<Args>(args)...);
    }

    template<typename R, typename ... Args>
    auto operator->*(R (T::* fun)(Args ...)) const noexcept
    {
        return [this, fun](Args&& ... args) {
            object_mover om(this);
            return (om.object.*fun)(std::forward<Args>(args)...);
        };
    }

    template<typename R, typename ... Args>
    auto operator->*(R (T::* fun)(Args ...) const) const noexcept
    {
        return [this, fun](Args&& ... args) {
            object_mover om(this);
            return (om.object.*fun)(std::forward<Args>(args)...);
        };
    }

private:
    const Container* base;
    size_t index;
    struct object_mover
    {
        const BaseSoAFacade* const SoAFacade;
        T object;
        explicit object_mover(const BaseSoAFacade* SoAFacade) : SoAFacade(SoAFacade), object(SoAFacade->aggregate()) { }
        ~object_mover() { SoAFacade->get_base()->move_object_mutable(std::move(object), SoAFacade->get_index()); }
    };

protected:
    constexpr BaseSoAFacade(const Container* base, std::size_t index) : base(base), index(index) { }
    constexpr auto get_index() const noexcept { return index; }
    constexpr auto get_size() const noexcept
    {
        if constexpr (std::is_base_of_v<Container, BaseFixedSize>)
            return Conatiner::get_constexpr_size();
        else
            return base->get_runtime_size();
    }
    constexpr const auto* get_base() const noexcept { return base; }
    void inc_index() noexcept { ++index; }
    void dec_index() noexcept { --index; }
    void advance_index(size_t n) noexcept { index += n; }

    template<typename Class, typename Type> static Type get_pointer_type(Type Class::*);

    template<typename R>
    constexpr std::size_t get_offset(R T::* member) const noexcept
    {
        return member_offset_helpers::get_offset(member, get_size(), index);
    }
};

template<typename T, typename Container>
class ConstSoAFacade : public BaseSoAFacade<T, Container>
{
public:
    ConstSoAFacade( const Container* b, std::size_t index) : BaseSoAFacade<T, Container>(b, index) { }

    using BaseSoAFacade<T, Container>::operator->*;

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    const auto& get() const noexcept
    {
        using Type = decltype(this->get_pointer_type(ptr));
        return *reinterpret_cast<const Type*>(this->get_base()->storage.data() + this->get_offset(ptr));
    }

    template<typename R>
    const R& operator->*(R T::* field) const noexcept
    {
        return *reinterpret_cast<const R*>(this->get_base()->storage.data() + this->get_offset(field));
    }
};

template<typename T, typename Container>
class SoAFacade : public BaseSoAFacade<T, Container>
{
public:
    SoAFacade( Container* b, std::size_t index) : BaseSoAFacade<T, Container>(b, index), mutable_base(b) { }

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    auto& get() const noexcept
    {
        using Type = decltype(this->get_pointer_type(ptr));
        return *reinterpret_cast<Type*>(mutable_base->storage.data() + this->get_offset(ptr));
    }

    using BaseSoAFacade<T, Container>::operator->*;

    template<typename R>
    R& operator->*(R T::* field) const noexcept
    {
        return *reinterpret_cast<R*>(mutable_base->storage.data() + this->get_offset(field));
    }

    void operator=(const T& rhs) const noexcept
    {
        static_assert(std::is_copy_assignable_v<T>, "Object cannot be assigned because its copy assignment operator is implicitly deleted");
        mutable_base->copy_object(rhs, this->get_index());
    }

    void operator=(T&& rhs) const noexcept
    {
        static_assert(std::is_move_assignable_v<T>, "Object cannot be assigned because its move assignment operator is implicitly deleted");
        mutable_base->move_object(std::move(rhs), this->get_index());
    }

private:
    Container* mutable_base;
};

template<typename T, typename ContainerT, typename Size>
class SoARandomAccessContainer : public Size
{
public:
    auto operator[](std::size_t index) noexcept { return SoAFacade<T, SoARandomAccessContainer>{ this, index}; }
    auto operator[](std::size_t index) const noexcept { return ConstSoAFacade<T, SoARandomAccessContainer>{ this, index}; }

    class iterator;

    class const_iterator : public boost::iterator_facade<const_iterator,
                                                         ConstSoAFacade<T, SoARandomAccessContainer> const,
                                                         std::random_access_iterator_tag,
                                                         const ConstSoAFacade<T, SoARandomAccessContainer>&>,
                           private ConstSoAFacade<T, SoARandomAccessContainer>
    {
        friend class SoARandomAccessContainer;
        friend class boost::iterator_core_access;

        const_iterator(const SoARandomAccessContainer* base, std::size_t index) : ConstSoAFacade<T, SoARandomAccessContainer>(base, index) { }

        const auto& dereference() const noexcept { return *this; }
        bool equal(const const_iterator& rhs) const noexcept { return this->get_index() == rhs.get_index(); }
        void increment() noexcept { this->inc_index(); }
        void decrement() noexcept { this->dec_index(); }
        void advance(size_t n) noexcept { this->advance_index( n); }
        ptrdiff_t distance_to(const const_iterator& rhs) const noexcept { return rhs.get_index() - this->get_index(); }
    };

    class iterator : public boost::iterator_facade<iterator,
                                                   SoAFacade<T, SoARandomAccessContainer>,
                                                   std::random_access_iterator_tag,
                                                   const SoAFacade<T, SoARandomAccessContainer>&>,
                     private SoAFacade<T, SoARandomAccessContainer>
    {
        friend class SoARandomAccessContainer;
        friend class boost::iterator_core_access;

        iterator(SoARandomAccessContainer* base, std::size_t index) : SoAFacade<T, SoARandomAccessContainer>(base, index) { }

        const auto& dereference() const noexcept { return *this; }
        bool equal(const iterator& rhs) const noexcept { return this->get_index() == rhs.get_index(); }
        void increment() noexcept { this->inc_index(); }
        void decrement() noexcept { this->dec_index(); }
        void advance(size_t n) noexcept { this->advance_index( n); }
        ptrdiff_t distance_to(const iterator& rhs) const noexcept { return rhs.get_index() - this->get_index(); }
    };

    auto cbegin() const noexcept { return const_iterator{ this, 0}; }
    auto cend() const noexcept { return const_iterator{ this, this->size}; }
    auto begin() const noexcept { return cbegin(); }
    auto end() const noexcept { return cend(); }
    auto begin() noexcept { return iterator{ this, 0}; }
    auto end() noexcept { return iterator{ this, this->size}; }

protected:
    // Storage has to be mutable to keep mutable elements
    mutable ContainerT storage;

    friend class BaseSoAFacade<T, SoARandomAccessContainer>;
    friend class SoAFacade<T, SoARandomAccessContainer>; 
    friend class ConstSoAFacade<T, SoARandomAccessContainer>;

    void allocate(const T& value)
    {
        for (size_t i = 0; i < this->size; ++i)
            this->copy_object(value, i);
    }

    void copy_object(const T& rhs, size_t index) noexcept
    {
        copy_helpers::copy_all_members_to_storage(rhs, storage.data(), index, this->size);
    }

    void move_object(T&& rhs, size_t index) noexcept
    {
        // Do not care about move semantics since we support only trivial structures so far
        copy_helpers::copy_all_members_to_storage(rhs, storage.data(), index, this->size);
    }

    void move_object_mutable(T&& rhs, size_t index) const noexcept
    {
        // Do not care about move semantics since we support only trivial structures so far
        copy_helpers::copy_all_members_to_storage(rhs, storage.data(), index, this->size);
    }

    T aggregate(size_t index) const noexcept
    {
        T result{};
        copy_helpers::copy_all_members_from_storage(storage.data(), &result, index, this->size);
        return result;
    }
};

template<typename T, typename Allocator = std::allocator<T>>
class SoAVector : public SoARandomAccessContainer<T, std::vector<char, Allocator>, VariableSize>
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
public:
    SoAVector() : SoAVector(0) { }
    explicit SoAVector(std::size_t s) { resize(s); }
    SoAVector(std::size_t s, const T& value) { resize(s, value); }

    void resize(std::size_t s)
    {
        if constexpr(std::is_trivially_constructible_v<T>)
            resize_memory(s);
        else
            resize(s, T());
    }

    void resize(std::size_t s, const T& value)
    {
        resize_memory(s);
        this->allocate(value);
    }

private:
    void resize_memory(std::size_t s)
    {
        if (!this->storage.empty())
            throw std::runtime_error("SoAVector cannot be sized twice.");

        this->storage.resize(s * sizeof(T));
        this->size = s;
    }
};

template<typename T, size_t N>
class SoAArray : public SoARandomAccessContainer<T, std::array<char, sizeof(T) * N>, FixedSize<N>>
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
public:
    SoAArray()
    {
        if constexpr(!std::is_trivially_constructible_v<T>)
            this->allocate(T());
    }

    SoAArray(const T& value)
    {
        allocate(value);
    }
};

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


namespace copy_helpers
{
    using namespace loophole_ns;
    using namespace member_offset_helpers;

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
            *reinterpret_cast<R*>(dst + get_offset(offset, sizeofR, size, index)) =
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
                *reinterpret_cast<const R*>(src + get_offset(offset, sizeofR, size, index));
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
} // namespace copy_helpers

} // namespace ao_ao_ao_tt

#endif
