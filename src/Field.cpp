#include <postgres/Field.h>

namespace postgres {

Field::Field(PGresult& res, int const row_idx, int const col_idx)
    : res_{&res}, row_idx_{row_idx}, col_idx_{col_idx} {
}

Field::Field(Field const& other) = default;

Field& Field::operator=(Field const& other) = default;

Field::Field(Field&& other) noexcept = default;

Field& Field::operator=(Field&& other) noexcept = default;

Field::~Field() noexcept = default;

void Field::read(Time::Point& out) const {
    Time tmp{};
    read(tmp);
    out = tmp.point();
}

void Field::read(Time& out) const {
    _POSTGRES_CXX_ASSERT(type() == TIMESTAMPOID,
                         "cannot cast field '"
                             << name()
                             << "' of type "
                             << type()
                             << " to timestamp without time zone");

    using ms = std::chrono::microseconds;
    out = Time{Time::EPOCH + ms{internal::orderBytes<int64_t>(value())}};
}

void Field::read(std::string& out) const {
    out.assign(value(), static_cast<size_t>(length()));
}

bool Field::isNull() const {
    return PQgetisnull(res_, row_idx_, col_idx_) == 1;
}

char const* Field::name() const {
    return PQfname(res_, col_idx_);
}

char const* Field::value() const {
    return PQgetvalue(res_, row_idx_, col_idx_);
}

Oid Field::type() const {
    return PQftype(res_, col_idx_);
}

int Field::length() const {
    return PQgetlength(res_, row_idx_, col_idx_);
}

int Field::format() const {
    return PQfformat(res_, col_idx_);
}

}  // namespace postgres
