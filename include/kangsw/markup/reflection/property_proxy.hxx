#pragma once
#include "object.hxx"

namespace kangsw::refl {

/**
 * Performs strict compile-time and runtime type check.
 *
 * In compile time, it checks whether Ty_ is one of object member types.
 *
 * In runtime, it checks whether given property's underlying type is same with Ty_.
 */
class property_type_mismatch_exception : public std::logic_error {
public:
    using std::logic_error::logic_error;

    template <typename Ty_>
    static void verify(property::memory_t const& m) {
        if (m.type != etype::from_type_exact<Ty_>()) {
            auto exception     = property_type_mismatch_exception{"Type mismatch"};
            exception.expected = etype::from_type_exact<Ty_>();
            exception.actual   = m.type;
            throw exception;
        }
    }

public:
    etype expected;
    etype actual;
};

/**
 * Generic property proxy type to manipulate trivial types.
 *
 * Single class can handle both of const and non-const instance of original reference.
 */
template <typename Ty_, bool Constant_>
class property_proxy {
public:
    using void_pointer = std::conditional_t<Constant_, void const*, void*>;
    using src_type     = std::conditional_t<Constant_, Ty_ const, Ty_>;
    using pointer      = src_type*;
    using reference    = src_type&;

    property_proxy(property const& m, void_pointer ptr) : _p(static_cast<pointer>(ptr)) {
        property_type_mismatch_exception::verify<Ty_>(m.memory());
    }
    property_proxy(property::attribute const& m, void_pointer ptr) : _p(static_cast<pointer>(ptr)) {
        property_type_mismatch_exception::verify<Ty_>(m._memory);
    }

    template <bool OtherConstant_>
    property_proxy(property_proxy<Ty_, OtherConstant_> other) : _p(other._p) {}

    constexpr auto type() { return etype::from_type<Ty_>(); }

public:
    reference operator*() const { return *_p; }
    pointer operator->() const { return _p; }

private:
    pointer _p;
};

/**
 * Vector specialization of property_proxy.
 *
 * Which wraps several vector operations into few common APIs which can be shared with object_vector_interface.
 */
template <typename ValueTy_, bool Constant_>
class property_proxy<std::vector<ValueTy_>, Constant_> {
public:
    using Ty_ = std::vector<ValueTy_>;

    using void_pointer = std::conditional_t<Constant_, void const*, void*>;
    using src_type     = std::conditional_t<Constant_, Ty_ const, Ty_>;
    using pointer      = src_type*;
    using reference    = src_type&;

    using value_type = typename src_type::value_type;

public:
    property_proxy(property const& m, void_pointer ptr) : _p(static_cast<pointer>(ptr)) {
        property_type_mismatch_exception::verify<Ty_>(m.memory());
    }

    template <bool OtherConstant_>
    property_proxy(property_proxy<Ty_, OtherConstant_> other) : _p(other._p) {}

    auto size() const { return _p->size(); }
    auto empty() const { return _p->empty(); }
    auto& emplace_back() { return _p->emplace_back(); }

    auto& operator[](size_t i) { return _p->operator[](i); }
    auto& operator[](size_t i) const { return static_cast<Ty_ const*>(_p)->operator[](i); }

    void reserve(size_t c) { _p->reserve(c); }

    void erase(size_t from, size_t to) { _p->erase(_p->begin() + from, _p->begin() + to); }
    void erase(size_t at) { erase(at, at + 1); }

    constexpr auto type() { return etype::from_type<Ty_>(); }

private:
    pointer _p;
};

/**
 * Object vector specialized version of property_proxy.
 * Wraps object_vector_interface
 */
template <bool Constant_>
class property_proxy<std::vector<object>, Constant_> {
public:
    using Vty_         = object;
    using Ty_          = std::vector<Vty_>;
    using void_pointer = std::conditional_t<Constant_, void const*, void*>;

    using value_type = object;

public:
    property_proxy(property const& m, void_pointer ptr) : _if(m.ovi()), _p(ptr) {
        assert(_if);
        property_type_mismatch_exception::verify<std::vector<Vty_>>(m.memory());
    }

    template <bool OtherConstant_>
    property_proxy(property_proxy<Ty_, OtherConstant_> other) : _if(other._if), _p(other._p) {}

    auto size() const { return _if->size(_p); }
    auto empty() const { return size() == 0; }
    auto& emplace_back() { return _if->push_back(_p); }

    auto& operator[](size_t i) { return _if->at(_p, i); }
    auto& operator[](size_t i) const { return _if->at(static_cast<void const*>(_p), i); }

    void reserve(size_t c) { _if->reserve(_p, c); }

    void erase(size_t from, size_t to) { _if->erase(_p, from, to); }
    void erase(size_t at) { _if->erase(_p, at, at + 1); }

    constexpr auto type() { return etype::from_type<Ty_>(); }

private:
    object_vector_interface const* _if;
    void_pointer _p;
};

/**
 * Works similar way with vector specialization of \ref property_proxy
 */
template <typename ValueTy_, bool Constant_>
class property_proxy<u8str_map<ValueTy_>, Constant_> {
public:
    using Ty_ = u8str_map<ValueTy_>;

    using void_pointer  = std::conditional_t<Constant_, void const*, void*>;
    using src_type      = std::conditional_t<Constant_, Ty_ const, Ty_>;
    using pointer       = src_type*;
    using const_pointer = Ty_ const*;
    using reference     = src_type&;

    using mapped_type = typename src_type::mapped_type;

    property_proxy(property const& m, void_pointer ptr) : _p(static_cast<pointer>(ptr)) {
        property_type_mismatch_exception::verify<Ty_>(m.memory());
    }

    template <bool OtherConstant_>
    property_proxy(property_proxy<Ty_, OtherConstant_> other) : _p(other._p) {}

public:
    auto size() const { return _p->size(); }
    auto& at(u8str_view s) { return _p->at(s); }
    auto& at(u8str_view s) const { return static_cast<const_pointer>(_p)->at(s); }

    auto find(u8str_view s) const {
        auto it = static_cast<const_pointer>(_p)->find(s);
        if (it == static_cast<const_pointer>(_p)->end()) { return nullptr; }
        return &it->second;
    }

    auto find(u8str_view s) {
        auto it = _p->find(s);
        if (it == _p->end()) { return nullptr; }
        return &it->second;
    }

    auto& insert(u8str_view s) { return _p[s]; }
    void erase(u8str_view s) { _p->erase(s); }

    template <typename Fn_>
    void for_each(Fn_&& fn) {
        for (auto& pair : *_p) { fn(pair.first, pair.second); }
    }

    template <typename Fn_>
    void for_each(Fn_&& fn) const {
        for (auto& pair : *static_cast<const_pointer>(_p)) { fn(pair.first, pair.second); }
    }

    constexpr auto type() { return etype::from_type<Ty_>(); }

private:
    pointer _p;
};

/**
 * Works similar way with object vector specialization of \ref property_proxy
 */
template <bool Constant_>
class property_proxy<u8str_map<object>, Constant_> {
public:
    using Vty_          = object;
    using Ty_           = u8str_map<object>;
    using void_pointer  = std::conditional_t<Constant_, void const*, void*>;
    using const_pointer = void const*;

    using mapped_type = object;

    property_proxy(property const& m, void_pointer ptr) : _if(m.omi()), _p(ptr) {
        assert(_if);
        property_type_mismatch_exception::verify<u8str_map<Vty_>>(m.memory());
    }

    template <bool OtherConstant_>
    property_proxy(property_proxy<Ty_, OtherConstant_> other) : _if(other._if), _p(other._p) {}

public:
    auto size() const { return _if->size(_p); }
    auto& at(u8str_view s) { return _if->at(_p, s); }
    auto& at(u8str_view s) const { return _if->at(static_cast<const_pointer>(_p), s); }
    auto find(u8str_view s) { return _if->find(_p, s); }
    auto find(u8str_view s) const { return _if->find(static_cast<const_pointer>(_p), s); }
    auto& insert(u8str_view s) { return _if->insert(_p); }
    void erase(u8str_view s) { _if->erase(_p, s); }

    template <typename Fn_>
    void for_each(Fn_&& fn) { _if->for_each(_p, std::forward<Fn_>(fn)); }

    template <typename Fn_>
    void for_each(Fn_&& fn) const { _if->for_each(const_cast<const_pointer>(_p), std::forward<Fn_>(fn)); }

    constexpr auto type() { return etype::from_type<Ty_>(); }

private:
    object_map_interface const* _if;
    void_pointer _p;
};

/**
 * Creates property proxy from object instance and property.
 *
 * Runtime type check will be performed.
 */
template <typename Ty_, typename Vp, typename PropTy_>
auto make_proxy(Vp* base, PropTy_ const& m) {
    enum { is_constant = std::is_const_v<Vp> };
    return property_proxy<Ty_, is_constant>{m, m.memory()(base)};
}

/**
 * \brief On property, invoke operation which will be applied on its actual underlying type. 
 * \tparam BasePtr_ 'object' or 'object const'
 * \tparam PropTy_ 'property' or 'property::attribute'
 * \tparam HandleFn_ return_type HandleFn_(property_proxy<T>)
 * \param obj 
 * \param pr 
 * \param fn This should be template function, which will be instanciated per available property types.
 * \return invocation result of HandleFn_
 */
template <typename BasePtr_, typename PropTy_, typename HandleFn_>
decltype(auto) visit_property(BasePtr_* obj, PropTy_ const& pr, HandleFn_&& fn) {
    static_assert(std::is_same_v<object_baseaddr_t, std::remove_const_t<BasePtr_>>);
    static_assert(std::is_same_v<property, PropTy_> || std::is_same_v<property::attribute, PropTy_>);
    using base_type = std::conditional_t<std::is_const_v<BasePtr_>, const object, object>;

    property::memory_t const& m = pr.memory();

    switch (m.type.get()) {
        case etype::null: return fn(make_proxy<nullptr_t>(obj, pr));
        case etype::boolean: return fn(make_proxy<boolean_t>(obj, pr));
        case etype::integer: return fn(make_proxy<integer_t>(obj, pr));
        case etype::floating_point: return fn(make_proxy<float_t>(obj, pr));
        case etype::string: return fn(make_proxy<u8str>(obj, pr));
        case etype::timestamp: return fn(make_proxy<timestamp_t>(obj, pr));
        case etype::binary: return fn(make_proxy<binary_chunk>(obj, pr));
        case etype::object: return fn(make_proxy<object>(obj, pr));
        default:;
    }

    if constexpr (std::is_same_v<PropTy_, property>) {
        if (m.type.is_array()) {
            switch (m.type.leap()) {
                case etype::null: return fn(make_proxy<std::vector<nullptr_t>>(obj, pr));
                case etype::boolean: return fn(make_proxy<std::vector<boolean_t>>(obj, pr));
                case etype::integer: return fn(make_proxy<std::vector<integer_t>>(obj, pr));
                case etype::floating_point: return fn(make_proxy<std::vector<float_t>>(obj, pr));
                case etype::string: return fn(make_proxy<std::vector<u8str>>(obj, pr));
                case etype::timestamp: return fn(make_proxy<std::vector<timestamp_t>>(obj, pr));
                case etype::binary: return fn(make_proxy<std::vector<binary_chunk>>(obj, pr));
                case etype::object: return fn(make_proxy<std::vector<object>>(obj, pr));
                default:;
            }
        } else if (m.type.is_map()) {
            switch (m.type.leap()) {
                case etype::null: return fn(make_proxy<u8str_map<nullptr_t>>(obj, pr));
                case etype::boolean: return fn(make_proxy<u8str_map<boolean_t>>(obj, pr));
                case etype::integer: return fn(make_proxy<u8str_map<integer_t>>(obj, pr));
                case etype::floating_point: return fn(make_proxy<u8str_map<float_t>>(obj, pr));
                case etype::string: return fn(make_proxy<u8str_map<u8str>>(obj, pr));
                case etype::timestamp: return fn(make_proxy<u8str_map<timestamp_t>>(obj, pr));
                case etype::binary: return fn(make_proxy<u8str_map<binary_chunk>>(obj, pr));
                case etype::object: return fn(make_proxy<u8str_map<object>>(obj, pr));
                default:;
            }
        } else {
            assert(0 && "invalid container type specified.");
        }
    } else {
        assert(0 && "fatal logic error: attribute must not have container type");
    }

    throw;
}
} // namespace kangsw::refl
