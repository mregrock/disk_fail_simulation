#pragma once
#include <compare>
#include <concepts>
#include <arctic/engine/easy.h>

namespace arctic {

class TGroupIdTag;
class TVDiskIdTag;
class TPDiskIdTag;

using TDCId = Ui32;

template <typename T, typename Tag> class TIdWrapper {
private:
    T Raw = {};

public:
    using Type = T;
    using TTag = Tag;

    constexpr TIdWrapper() noexcept {}

    TIdWrapper(TIdWrapper &&value) = default;

    TIdWrapper(const TIdWrapper &other) = default;

    TIdWrapper &operator=(const TIdWrapper &value) = default;

    TIdWrapper &operator=(TIdWrapper &&value) = default;

    std::string ToString() const { return std::to_string(Raw); }

    static constexpr TIdWrapper FromValue(T value) noexcept {
        TIdWrapper id;
        id.Raw = value;
        return id;
    }

    static constexpr TIdWrapper Zero() noexcept { return TIdWrapper(); }

    TIdWrapper &operator+=(const T &other) {
        Raw += other.Raw;
        return *this;
    }

    friend TIdWrapper operator+(const TIdWrapper &first,
                                const T &second) {
        return TIdWrapper(first->Raw + second);
    }

    TIdWrapper &operator++() {
        Raw++;
        return *this;
    }

    TIdWrapper operator++(int) {
        TIdWrapper old = *this;
        operator++();
        return old;
  }

    friend std::ostream &operator<<(std::ostream &out, TIdWrapper &id) {
        return out << id.Raw;
    }

    friend std::ostream &operator<<(std::ostream &out, const TIdWrapper &id) {
        return out << id.Raw;
    }

    constexpr auto operator<=>(const TIdWrapper &) const = default;

    T GetRawId() const { return Raw; }

    friend std::hash<TIdWrapper<T, Tag>>;
};

using TVDiskId = TIdWrapper<Ui32, TVDiskIdTag>;
using TPDiskId = TIdWrapper<Ui32, TPDiskIdTag>;
using TGroupId = TIdWrapper<Ui32, TGroupIdTag>;

} // namespace arctic

template <typename T, typename Tag> struct std::hash<arctic::TIdWrapper<T, Tag>> {
    std::size_t operator()(const arctic::TIdWrapper<T, Tag> &id) const {
        return std::hash<T>{}(id.Raw);
    }
};