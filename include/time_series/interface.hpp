/**
 * @file interface.hpp
 * @author Vincent Berenz
 * license License BSD-3-Clause
 * @copyright Copyright (c) 2019, Max Planck Gesellschaft.
 */

#pragma once

#include <cstddef>
#include <limits>

namespace time_series
{
typedef long int Index;
typedef long double Timestamp;

const Index EMPTY = -1;

/**
 * \brief Interface for time series.
 * A time_series implements  \f$ X_{{oldest}:{newest}} \f$ which can
 * safely be accessed from either multiple threads or multiple processes.
 *
 * this object has the following properties:
 * - an oldest timeindex \f$ oldest\f$,
 * - a newest timeindex \f$ newest \f$,
 * - a value \f$ X_i \f$ for each  \f$ i \in \{oldest, oldest + 1 , ...,
 * newest\} \f$,
 * - a length \f$length\f$
 * - and a maximum length \f$maxlength\f$
 */
template <typename T>
class TimeSeriesInterface
{
public:
    virtual ~TimeSeriesInterface()
    {
    }

    /*! \brief returns \f$ newest \f$ index. If argument wait is true, waits if
     * the time_series is empty.
     * If argument wait is false and the time series is empty,
     * returns time_series::EMPTY immediately.
     */
    virtual Index newest_timeindex(bool wait = true) const = 0;

    /*! \brief returns the number of element that has been contained in the
     * queue, i.e.
     *  the number of elements that have been added from the start.
     */
    virtual Index count_appended_elements() const = 0;

    /*! \brief returns \f$ oldest \f$. waits if the time_series is empty.
     * If argument wait is false and the time series is empty,
     * returns time_series::EMPTY immediately.
     */
    virtual Index oldest_timeindex(bool wait = true) const = 0;

    /*! \brief returns \f$ X_{newest} \f$. waits if the time_series is empty.
     */
    virtual T newest_element() const = 0;

    /*! \brief returns \f$ X_{timeindex} \f$. waits if the time_series is empty
     * or if \f$timeindex > newest \f$.
     */
    virtual T operator[](const Index &timeindex) const = 0;

    /*! \brief returns the time in miliseconds when \f$ X_{timeindex} \f$
     * was appended. Waits if the time_series is empty
     * or if \f$timeindex > newest \f$.
     */
    virtual Timestamp timestamp_ms(const Index &timeindex) const = 0;

    /*! \brief returns the time in seconds when \f$ X_{timeindex} \f$
     * was appended. Waits if the time_series is empty
     * or if \f$timeindex > newest \f$.
     */
    virtual Timestamp timestamp_s(const Index &timeindex) const = 0;

    /*! \brief Wait until the defined time index is reached. If the input time
     * is below the oldest time index that have been registered read an
     * exception is return.
     */
    virtual bool wait_for_timeindex(
        const Index &timeindex,
        const double &max_duration_s =
            std::numeric_limits<double>::quiet_NaN()) const = 0;

    /*! \brief returns the length of the time_series, i.e. \f$0\f$ if it is
     * empty, otherwise \f$newest - oldest +1 \f$.
     */
    virtual std::size_t length() const = 0;

    /** @brief returns the maximum length of the time serie.
     *  @return std::size_t
     */
    virtual std::size_t max_length() const = 0;

    /*! \brief returns boolean indicating whether new elements have been
     * appended since the last time the tag() function was called.
     */
    virtual bool has_changed_since_tag() const = 0;

    /*! \brief tags the current time_series, can later be used to check
     * whether new elements have been added
     */
    virtual void tag(const Index &timeindex) = 0;

    /*! \brief returns the index at which the time series has been tagged.
     * Returns the newest timeindex if the time series has never been tagged.
     */
    virtual Index tagged_timeindex() const = 0;

    /*! \brief appends a new element to the time_series, e.g. we go from
     * \f$ X_{1:10} \f$ to \f$ X_{1:11} \f$ (where \f$ X_{11}=\f$ element).
     * if the time_series length is
     * already equal to its max_length, then the oldest element is discarded,
     * e.g. for a max_length = 10 we would go from \f$ X_{1:10} \f$
     * to \f$ X_{2:11} \f$.
     */
    virtual void append(const T &element) = 0;

    /*! \brief returns true if no element has ever been appended
     *  to the time series.
     */
    virtual bool is_empty() const = 0;
};
}  // namespace time_series
