#pragma once

namespace core {

template <typename T, typename Tag>
struct TaggedData {
    using value_type = T;
    using tag_type = Tag;

    TaggedData() = default;
    TaggedData(T const& d, Tag t) : data{d}, tag{t} {}
    TaggedData(T && d, Tag t) : data{std::move(d)}, tag{t} {}

    T data;
    Tag tag;
};

}// namespace core