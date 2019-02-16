#ifndef AO_AO_AO_TT
#define AO_AO_AO_TT

#include <memory>
#include <vector>

template<typename T, typename = std::enable_if_t<std::is_trivial<T>::value>>
class AoS {
    struct Iface : T {};
    std::vector<Iface> storage;
public:
    AoS() { }
    AoS(std::size_t size) : storage(size) { }
    void resize(std::size_t size) { storage.resize( size); }

    Iface& operator[](std::size_t index) { return storage[index]; }
    const Iface& operator[](std::size_t index) const { return storage[index]; }

    template<typename R>
    friend R& operator->*(Iface& iface, R T::* field)
    {
        return iface.*field;
    }

    template<typename R>
    friend const R& operator->*(const Iface& iface, R T::* field)
    {
        return iface.*field;
    }
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
        template<typename R>
        std::size_t get_offset(R T::* member) const
        {
            return member_offset(member) * size + index * sizeof(R);
        }
    };

    class Iface : private BaseIface
    {
        SoA<T>* base;
    public:
        Iface( SoA<T>* b, std::size_t index, std::size_t size) : BaseIface(index, size), base(b) { }

        template<typename R>
        R& get_ref(R T::* field) const
        {
            return *reinterpret_cast<R*>(base->storage.data() + this->get_offset(field));
        }
    };

    class ConstIface : private BaseIface
    {
        const SoA<T>* base;
    public:
        ConstIface( const SoA<T>* b, std::size_t index, std::size_t size) : BaseIface(index, size), base(b) { }
        template<typename R>
        const R& get_ref(R T::* field) const
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

    template<typename R>
    friend R& operator->*(const Iface& iface, R T::* field)
    {
        return iface.get_ref( field);
    }

    template<typename R>
    friend const R& operator->*(const ConstIface& iface, R T::* field)
    {
        return iface.get_ref( field);
    }
};

#endif
