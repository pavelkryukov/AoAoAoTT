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

#include "magic.hpp"

#include <boost/iterator/iterator_facade.hpp>

namespace aoaoaott {

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

    template<auto ptr, typename = std::enable_if_t<std::is_member_function_pointer_v<decltype(ptr)>>, typename ... Args>
    auto method(Args&& ... args) // noexcept?
    {
        return (this->*ptr)(std::forward<Args>(args)...);
    }

    template<auto ptr, typename = std::enable_if_t<std::is_member_function_pointer_v<decltype(ptr)>>, typename ... Args>
    auto method(Args&& ... args) const // noexcept?
    {
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

    auto& operator[](size_t index) noexcept { return storage[index]; }
    const auto& operator[](size_t index) const noexcept { return storage[index]; }

    auto& at(size_t index) { return storage.at(index); }
    const auto& at(size_t index) const { return storage.at(index); }

    using iterator = typename ContainerT::iterator;
    using const_iterator = typename ContainerT::const_iterator;
    using reverse_iterator = typename ContainerT::reverse_iterator;
    using const_reverse_iterator = typename ContainerT::const_reverse_iterator;

    auto empty() const noexcept { return storage.empty(); }
    auto size() const noexcept { return storage.size(); }

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

    const auto& front() const { return *begin(); }
    auto& front() { return *begin(); }

    const auto& back() const { auto tmp = end(); --tmp; return *tmp; }
    auto& back() { auto tmp = end(); --tmp; return *tmp; }
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
    explicit AoSVector(size_t size) : Base(size) { }
    AoSVector(size_t size, const T& value) : Base(size, AoSFacade(value)) { }

    void resize(size_t size) { this->storage.resize(size); }
    void resize(size_t size, const T& value) { this->storage.resize(size, AoSFacade(value)); }

    void reserve(size_t size) { this->storage.reserve(size); }
    auto capacity() const noexcept { return this->storage.capacity(); }
    void shrink_to_fit() { this->storage.shrink_to_fit(); }

    void push_back(const T& value) { this->storage.push_back(AoSFacade(value)); }
    void push_back(T&& value) { this->storage.push_back(AoSFacade(std::move(value))); }

    void assign(size_t count, const T& value) { this->storage.assign(count, AoSFacade(value)); }
};

template<typename T, size_t N>
class AoSArray : public AoSRandomAccessContainer<std::array<AoSFacade<T>, N>>
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
    using Base = AoSRandomAccessContainer<std::array<AoSFacade<T>, N>>;
public:
    AoSArray() = default;
    explicit AoSArray(const T& value) { fill(value); }

    void fill(const T& value) { this->storage.fill(AoSFacade(value)); }
};

template<typename T, typename Container>
class BaseSoAFacade
{
public:
    T aggregate() const noexcept
    {
        return base->aggregate(this->get_index());
    }

    template<auto ptr, typename = std::enable_if_t<std::is_member_function_pointer_v<decltype(ptr)>>, typename ... Args>
    auto method(Args&& ... args) const // noexcept?
    {
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
    constexpr BaseSoAFacade(const Container* base, size_t index) : base(base), index(index) { }
    constexpr auto get_index() const noexcept { return index; }

    constexpr const auto* get_base() const noexcept { return base; }
    void inc_index() noexcept { ++index; }
    void dec_index() noexcept { --index; }
    void advance_index(size_t n) noexcept { index += n; }

    template<typename Class, typename Type> static Type get_pointer_type(Type Class::*);
};

template<typename T, typename Container>
class ConstSoAFacade : public BaseSoAFacade<T, Container>
{
public:
    ConstSoAFacade( const Container* b, size_t index) : BaseSoAFacade<T, Container>(b, index) { }

    using BaseSoAFacade<T, Container>::operator->*;

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    const auto& get() const noexcept
    {
        return this->get_base()->get_start_pointer(ptr)[this->get_index()];
    }

    template<typename R>
    const R& operator->*(R T::* field) const noexcept
    {
        return this->get_base()->get_start_pointer(field)[this->get_index()];
    }
};

template<typename T, typename Container>
class SoAFacade : public BaseSoAFacade<T, Container>
{
public:
    SoAFacade( Container* b, size_t index) : BaseSoAFacade<T, Container>(b, index), mutable_base(b) { }

    template<auto ptr, typename = std::enable_if_t<std::is_member_pointer_v<decltype(ptr)>>>
    auto& get() const noexcept
    {
        return mutable_base->get_start_pointer(ptr)[this->get_index()];
    }

    using BaseSoAFacade<T, Container>::operator->*;

    template<typename R>
    R& operator->*(R T::* field) const noexcept
    {
        return mutable_base->get_start_pointer(field)[this->get_index()];
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

template<typename T, typename ContainerT>
class SoARandomAccessContainer
{
public:
    auto operator[](size_t index) noexcept { return SoAFacade<T, SoARandomAccessContainer>{ this, index}; }
    auto operator[](size_t index) const noexcept { return ConstSoAFacade<T, SoARandomAccessContainer>{ this, index}; }

    auto at(size_t index) { check_index(index); return operator[](index); }
    auto at(size_t index) const { check_index(index); return operator[](index); }

    class iterator;

    class const_iterator : public boost::iterator_facade<const_iterator,
                                                         ConstSoAFacade<T, SoARandomAccessContainer> const,
                                                         std::random_access_iterator_tag,
                                                         const ConstSoAFacade<T, SoARandomAccessContainer>&>,
                           private ConstSoAFacade<T, SoARandomAccessContainer>
    {
        friend class SoARandomAccessContainer;
        friend class boost::iterator_core_access;

        const_iterator(const SoARandomAccessContainer* base, size_t index) : ConstSoAFacade<T, SoARandomAccessContainer>(base, index) { }

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

        iterator(SoARandomAccessContainer* base, size_t index) : SoAFacade<T, SoARandomAccessContainer>(base, index) { }

        const auto& dereference() const noexcept { return *this; }
        bool equal(const iterator& rhs) const noexcept { return this->get_index() == rhs.get_index(); }
        void increment() noexcept { this->inc_index(); }
        void decrement() noexcept { this->dec_index(); }
        void advance(size_t n) noexcept { this->advance_index( n); }
        ptrdiff_t distance_to(const iterator& rhs) const noexcept { return rhs.get_index() - this->get_index(); }
    };

    auto empty() const noexcept { return std::get<0>(this->storage).empty(); }
    auto size() const noexcept { return std::get<0>(this->storage).size(); }

    auto cbegin() const noexcept { return const_iterator{ this, 0}; }
    auto cend() const noexcept { return const_iterator{ this, size()}; }
    auto begin() const noexcept { return cbegin(); }
    auto end() const noexcept { return cend(); }
    auto begin() noexcept { return iterator{ this, 0}; }
    auto end() noexcept { return iterator{ this, size()}; }

    const auto front() const { return *begin(); }
    auto front() { return *begin(); }

    const auto back() const { auto tmp = end(); --tmp; return *tmp; }
    auto back() { auto tmp = end(); --tmp; return *tmp; }
protected:
    // Storage has to be mutable to keep mutable elements
    mutable ContainerT storage;

    friend class BaseSoAFacade<T, SoARandomAccessContainer>;
    friend class SoAFacade<T, SoARandomAccessContainer>;
    friend class ConstSoAFacade<T, SoARandomAccessContainer>;

    using Indices = std::make_index_sequence<loophole_ns::as_type_list<T>::size>;
    
    template<typename R>
    constexpr auto get_start_pointer(R T::* member) const noexcept
    {
        return containers::get_data_ptr<std::remove_cv_t<R>>(this->storage, member_offset_helpers::get_member_id(member));
    }

    template<size_t N>
    constexpr auto get_start_pointer() const noexcept
    {
        return std::get<N>(this->storage).data();
    }

    void copy_object(const T& rhs, size_t index) noexcept
    {
        dissipate(rhs, index, Indices{});
    }

    void move_object(T&& rhs, size_t index) noexcept
    {
        // Do not care about move semantics since we support only trivial structures so far
        dissipate(rhs, index, Indices{});
    }

    void move_object_mutable(T&& rhs, size_t index) const noexcept
    {
        // Do not care about move semantics since we support only trivial structures so far
        dissipate(rhs, index, Indices{});
    }

    T aggregate(size_t index) const noexcept
    {
        return aggregate(index, Indices{});
    }

    template<size_t ... N>
    void dissipate(const T& src, size_t index, std::index_sequence<N...>)
        const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        std::tie(get_start_pointer<N>()[index]...) = std::tie(member_offset_helpers::get_nth_member<T, N>(src)...);
    }

    template<size_t ... N>
    T aggregate(size_t index, std::index_sequence<N...>)
        const noexcept(noexcept(std::is_nothrow_copy_assignable_v<T>))
    {
        T result{};
        std::tie(member_offset_helpers::get_nth_member<T, N>(result)...) = std::tie(get_start_pointer<N>()[index]...);
        return result;
    }

    void check_index(size_t index) const
    {
        if (index >= size())
            throw std::out_of_range("SoA container is out of range");
    }

    void replicate(const T& value, size_t start, size_t end)
    {
        for (size_t i = start; i < end; ++i)
            this->copy_object(value, i);
    }
};

template<typename T>
class SoAVector : public SoARandomAccessContainer<T, containers::loophole_tuple_of_vectors<T>>
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
    static_assert(sizeof(T) == loophole_ns::sizeof_type_elements<T>, "AoAoAoTT does not support types with padding bytes");
public:
    SoAVector() : SoAVector(0) { }
    explicit SoAVector(size_t s) { resize(s); }
    SoAVector(size_t s, const T& value) { resize(s, value); }

    void resize(size_t s)
    {
        if constexpr(std::is_trivially_constructible_v<T>)
            resize_memory(s);
        else
            resize(s, T());
    }

    void resize(size_t s, const T& value)
    {
        size_t old_size = std::get<0>(this->storage).size();
        resize_memory(s);
        this->replicate( value, old_size, s);
    }

    void reserve(size_t s)
    {
        visitor::visit_all(this->storage, [s](auto& v){ v.reserve(s); });
    }

    void shrink_to_fit()
    {
        visitor::visit_all(this->storage, [](auto& v){ v.shrink_to_fit(); });
    }

    void assign(size_t s, const T& value)
    {
        if (s > this->size())
            resize(s, value);
        else
            this->replicate( value, 0, s);
    }

    auto capacity() const noexcept { return std::get<0>(this->storage).capacity(); }

    void push_back(const T& value)
    {
        auto s = this->size();
        resize_memory(s + 1);
        this->copy_object(value, s);
    }

    void push_back(T&& value)
    {
        auto s = this->size();
        resize_memory(s + 1);
        this->move_object(std::move(value), s);
    }
private:
    void resize_memory(size_t s)
    {
        visitor::visit_all(this->storage, [s](auto& v){ v.resize(s); });
    }
};

template<typename T, size_t N>
class SoAArray : public SoARandomAccessContainer<T, containers::loophole_tuple_of_arrays<N, T>>
{
    static_assert(std::is_trivially_copyable<T>::value, "AoAoAoTT supports only trivially copyable types");
    static_assert(sizeof(T) == loophole_ns::sizeof_type_elements<T>, "AoAoAoTT does not support types with padding bytes");
public:
    SoAArray()
    {
        if constexpr(!std::is_trivially_constructible_v<T>)
            fill(T());
    }

    SoAArray(const T& value)
    {
        fill(value);
    }
    
    void fill(const T& value)
    {
        this->replicate( value, 0, N);
    }
};

} // namespace aoaoaott

#endif
