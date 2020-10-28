#pragma once

#define MATRIX_SIZE 20

// used by the unit tests of this package,
// as (relatively) big serializable instances
// that will be element of the tested time_series.
// Note it implements the serialize function
// required by MultiprocessesTimeSeries.

class Type
{
public:
    Type()
    {
        for (int i = 0; i < MATRIX_SIZE; i++)
        {
            for (int j = 0; j < MATRIX_SIZE; j++)
            {
                data_[i][j] = ((double)rand() / (RAND_MAX));
            }
        }
    }

    bool operator==(const Type& other) const
    {
        bool r = data_ == other.data_;
        return r;
    }

    void print() const
    {
        for (int i = 0; i < MATRIX_SIZE; i++)
        {
            for (int j = 0; j < MATRIX_SIZE; j++)
            {
                std::cout << data_[i][j] << " ";
            }
            std::cout << " | ";
        }
        std::cout << "\n";
    }

    void set(int i, int j, double value)
    {
        data_[i][j] = value;
    }

    double get(int i, int j)
    {
        return data_[i][j];
    }

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(data_);
    }

    std::array<std::array<double, MATRIX_SIZE>, MATRIX_SIZE> data_;
};
