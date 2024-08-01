#ifndef NANODBC_VARIANT_ROW_CACHED_RESULT_H
#define NANODBC_VARIANT_ROW_CACHED_RESULT_H

#ifndef _MSC_VER
#error This file requires _variant_t and only supports Visual C++ on Windows
#endif

#include <comutil.h> // _variant_t
#include <nanodbc/nanodbc.h>

namespace nanodbc
{

class variant_row_cached_result : private result
{
public:
    // Import minimal operations set for necessary usability
    using result::columns;
    using result::unbind;
    using result::operator bool;

    /// \brief Empty result set.
    variant_row_cached_result() = default;

    /// \brief Converting contructor.
    variant_row_cached_result(result&& result);

    /// \brief Move constructor.
    variant_row_cached_result(variant_row_cached_result&& rhs) noexcept;

    /// \brief Move assignment operator.
    variant_row_cached_result& operator=(variant_row_cached_result&& rhs) noexcept;

    /// Member swap.
    void swap(variant_row_cached_result& rhs) noexcept;

    /// \brief Gets cached data from the given column of the current rowset.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    _variant_t const& get(short column) const;

    /// \brief Returns true if and only if cached value of the given column is of VT_NULL type.
    bool is_null(short column) const;

    /// \brief Returns the next result.
    bool next();

    /// \brief Access to underlying result
    ///
    /// For example, in order to access to columns metadata.
    nanodbc::result& result() { return static_cast<nanodbc::result&>(*this); }

    /// \brief Access to underlying result
    ///
    /// For example, in order to access to columns metadata.
    nanodbc::result const& result() const { return static_cast<nanodbc::result const&>(*this); }

private:
    /// \brief Clears cache of row values as result::next pre-condition.
    void clear_row();

    /// \brief Re-fills cache of row values as result::next post-condition.
    void fill_row();

private:
    std::vector<_variant_t> row_;
    std::size_t row_size_{0};
};

} // namespace nanodbc

#endif
