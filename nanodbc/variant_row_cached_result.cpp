#include <nanodbc/variant_row_cached_result.h>

#ifndef NANODBC_ASSERT
#include <cassert>
#define NANODBC_ASSERT(expr) assert(expr)
#endif

#if defined(_MSC_VER)
#if defined(_DEBUG)
#pragma comment(lib, "comsuppwd.lib")
#else
#pragma comment(lib, "comsuppw.lib")
#endif
#endif

#include <sqlext.h>
#include <windows.h>

namespace
{
static VARIANT const variant_null = {VT_NULL, 0};
_variant_t const variant_null_value(variant_null);
} // unnamed namespace

namespace nanodbc
{

variant_row_cached_result::variant_row_cached_result(nanodbc::result&& result)
    : nanodbc::result(std::move(result))
    , row_size_(columns())
{
}

variant_row_cached_result::variant_row_cached_result(variant_row_cached_result&& rhs) noexcept
    : nanodbc::result(std::move(rhs))
    , row_(std::move(rhs.row_))
    , row_size_(rhs.row_size_)
{
    rhs.row_.clear();
    rhs.row_size_ = 0;
}

variant_row_cached_result&
variant_row_cached_result::operator=(variant_row_cached_result&& rhs) noexcept
{
    if (&rhs != this)
    {
        nanodbc::result::operator=(std::move(rhs));
        row_ = std::move(rhs.row_);
        row_size_ = rhs.row_size_;
        rhs.row_.clear();
        rhs.row_size_ = 0;
    }
    return *this;
}

void variant_row_cached_result::swap(variant_row_cached_result& rhs) noexcept
{
    result::swap(rhs);
    using std::swap;
    swap(row_, rhs.row_);
    swap(row_size_, rhs.row_size_);
}

void variant_row_cached_result::clear_row()
{
    NANODBC_ASSERT(row_size_ > 0);

    row_.clear();
    if (row_.capacity() != row_size_)
        row_.reserve(row_size_);

    NANODBC_ASSERT(row_.capacity() == row_size_);
}

void variant_row_cached_result::fill_row()
{
    NANODBC_ASSERT(row_size_ > 0);
    // FIXME: There is a mess in drivers support for SQL_ATTR_ROW_NUMBER, so at_end is buggy
    // if (result::at_end())
    //     NANODBC_ASSERT(0); // throw programming_error

    if (!row_.empty())
        return; // Do not allow refilling from the same data source tuple

    if (row_.empty())
        row_.resize(row_size_);

    for (short column = 0; column < row_size_; column++)
    {
        NANODBC_ASSERT(row_[column].vt == VT_EMPTY);
        row_[column] = result::get<_variant_t>(column, variant_null_value);
    }

    NANODBC_ASSERT(row_.size() == row_size_);
}

_variant_t const& variant_row_cached_result::get(short column) const
{
    if (column >= row_.size())
        throw index_range_error(); // row cache empty or column index out of range

    return row_[column];
}

bool variant_row_cached_result::is_null(short column) const
{
    if (column >= row_.size())
        throw programming_error("row cache empty or column index out of range");

    return row_[column].vt == VT_NULL;
}

bool variant_row_cached_result::next()
{
    clear_row();
    if (!result::next())
    {
        return false;
    }
    fill_row();
    return true;
}

} // namespace nanodbc
