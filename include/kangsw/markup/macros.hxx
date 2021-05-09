#pragma once
#include "reflection/static_object_base.hxx"

// INTERNAL_CPPMARKUP_OBJECT_TEMPLATE(object_type)
#define INTERNAL_CPPMARKUP_OBJECT_TEMPLATE(object_type) \
    struct object_type : ::kangsw::refl::static_object_base<object_type>

// INTERNAL_CPPMARKUP_ELEMENT(elem_var, elem_name, default_value, flags)
// INTERNAL_CPPMARKUP_ELEMENT_WITH_ATTR(elem_var, elem_name, default_value, flags, ...)

// > INTERNAL_CPPMARKUP_ENTITY_former(elem_var, ...)

#define INTERNAL_CPPMARKUP_ENTITY_former(elem_var, elem_name) \
    constexpr static auto _##elem_var##_TAG = elem_name;      \
    struct _##elem_var##_HASH_TYPE {}

#define INTERNAL_CPPMARKUP_ENTITY_former_ATTR(elem_var, elem_name, ...)                     \
    INTERNAL_CPPMARKUP_ENTITY_former(elem_var, elem_name);                                  \
    struct _##elem_var##_ATTRIBUTES {                                                       \
        using _internal_ATTRIBUTES_TYPE = _##elem_var##_ATTRIBUTES;                         \
        static size_t _internal_ATTRSTRUCT_OFFSET() { return _##elem_var##_ATTR_OFFSET(); } \
        constexpr static auto _owner_element_TAG = elem_name;                               \
        __VA_ARGS__;                                                                        \
    } elem_var##_;                                                                          \
    static size_t _##elem_var##_ATTR_OFFSET() { return offsetof(self_type, elem_var##_); }

#define INTERNAL_CPPMARKUP_ENTITY_former_NOATTR(elem_var, elem_name) \
    INTERNAL_CPPMARKUP_ENTITY_former(elem_var, elem_name);           \
    static size_t _##elem_var##_ATTR_OFFSET() { return 0; }

// >
// INTERNAL_CPPMARKUP_ATTRIBUTE(attrib_var, attrib_name, default_value)
#define INTERNAL_CPPMARKUP_ATTRIBUTE(attrib_var, attrib_name, default_value)                    \
    using _##attrib_var##_VALUE_TYPE = decltype(::kangsw::refl::etype::deduce(default_value));  \
    struct _##attrib_var##_HASH_TYPE {};                                                        \
    _##attrib_var##_VALUE_TYPE attrib_var;                                                      \
                                                                                                \
    static size_t _##attrib_var##_OFFSET() {                                                    \
        return _internal_ATTRSTRUCT_OFFSET() + offsetof(_internal_ATTRIBUTES_TYPE, attrib_var); \
    }                                                                                           \
                                                                                                \
    static inline ::kangsw::refl::attribute_registration_t<                                     \
        self_type, _##attrib_var##_VALUE_TYPE, _##attrib_var##_HASH_TYPE>                       \
        _##attrib_var##_REGISTER{                                                               \
            _owner_element_TAG,                                                                 \
            attrib_name,                                                                        \
            _##attrib_var##_OFFSET(),                                                           \
            ::kangsw::refl::etype::deduce(default_value)};

// > INTERNAL_CPPMARKUP_ENTITY_latter(elem_var, elem_name, flags)
#define INTERNAL_CPPMAKRUP_ENTITY_latter(elem_var, flags)                          \
    _##elem_var##_VALUE_TYPE elem_var;                                             \
                                                                                   \
    static size_t _##elem_var##_OFFSET() { return offsetof(self_type, elem_var); } \
    static inline ::kangsw::refl::element_regestration_t<                          \
        self_type, _##elem_var##_VALUE_TYPE, _##elem_var##_HASH_TYPE>              \
        _##elem_var##_REGISTER {                                                   \
        _##elem_var##_TAG,                                                         \
            _##elem_var##_OFFSET(),                                                \
            ::kangsw::refl::etype::deduce(_##elem_var##_DEFAULT_VALUE()),          \
            flags                                                                  \
    }
// >

// Custom
#define INTERNAL_CPPMARKUP_ELEMENT_ATTR(elem_var, elem_name, default_value, flags, ...)      \
    INTERNAL_CPPMARKUP_ENTITY_former_ATTR(elem_var, elem_name, ##__VA_ARGS__);               \
    using _##elem_var##_VALUE_TYPE = decltype(::kangsw::refl::etype::deduce(default_value)); \
                                                                                             \
    static inline const auto _##elem_var##_DEFAULT_VALUE = []() { return default_value; };   \
    INTERNAL_CPPMAKRUP_ENTITY_latter(elem_var, flags)

#define INTERNAL_CPPMARKUP_ELEMENT_NOATTR(elem_var, elem_name, default_value, flags)         \
    INTERNAL_CPPMARKUP_ENTITY_former_NOATTR(elem_var, elem_name);                            \
    using _##elem_var##_VALUE_TYPE = decltype(::kangsw::refl::etype::deduce(default_value)); \
                                                                                             \
    static inline const auto _##elem_var##_DEFAULT_VALUE = []() { return default_value; };   \
    INTERNAL_CPPMAKRUP_ENTITY_latter(elem_var, flags)

#define INTERNAL_CPPMARKUP_EMBED_OBJECT_begin_ATTR(elem_var, elem_name, flags, ...) \
    static constexpr auto _##elem_var##_FLAGS = flags;                              \
    INTERNAL_CPPMARKUP_ENTITY_former_ATTR(elem_var, elem_name, ##__VA_ARGS__);      \
    INTERNAL_CPPMARKUP_OBJECT_TEMPLATE(_##elem_var##_VALUE_TYPE)

#define INTERNAL_CPPMARKUP_EMBED_OBJECT_begin_NOATTR(elem_var, elem_name, flags) \
    static constexpr auto _##elem_var##_FLAGS = flags;                           \
    INTERNAL_CPPMARKUP_ENTITY_former_NOATTR(elem_var, elem_name);                \
    INTERNAL_CPPMARKUP_OBJECT_TEMPLATE(_##elem_var##_VALUE_TYPE)

#define INTERNAL_CPPMARKUP_EMBED_OBJECT_end(elem_var)             \
    ;                                                             \
    static inline const auto _##elem_var##_DEFAULT_VALUE =        \
        []() { return _##elem_var##_VALUE_TYPE::get_default(); }; \
    INTERNAL_CPPMAKRUP_ENTITY_latter(elem_var, _##elem_var##_FLAGS)

namespace kangsw::refl::_internal {
template <typename DTy_, typename Ty_, typename... Args_>
auto _deduce_map_impl(u8str_map<DTy_>& acc, u8str_view a, Ty_&& b, Args_&&... args) {
    acc.emplace(a, etype::deduce(std::forward<Ty_>(b)));
    if constexpr (sizeof...(args) > 2)
        return _deduce_map_impl(acc, std::forward<Args_>(args)...);
    else
        return acc;
}

template <typename Ty_, typename... Args_>
auto deduce_map(u8str_view a, Ty_&& b, Args_&&... args) {
    using deduced_t = decltype(etype::deduce(b));
    u8str_map<deduced_t> map;
    return _deduce_map_impl<deduced_t>(
        map, a,
        (std::forward<Ty_>(b)),
        std::forward<Args_>(args)...);
}
} // namespace kangsw::refl::_internal

#define INTERNAL_CPPMARKUP_MAP(...) ::kangsw::refl::_internal::deduce_map(__VA_ARGS__)