//
// Created by John O'Sullivan on 5/13/18.
//

#ifndef PLAYGROUND_2DARRAY_H
#define PLAYGROUND_2DARRAY_H

#include <stdint.h>
#include <stdlib.h>

namespace ds {

    template <typename T=char>

    class Array2D {

    private:

        uint32_t rows;
        uint32_t columns;
        T * data;

    public:

        Array2D(uint32_t _row, uint32_t _col) {
            rows = _row;
            columns = _col;
            data = new T[_row * _col];
        }

        ~Array2D() {
            delete [] data;
        }

    private:

        Array2D(const Array2D&);
        Array2D& operator=(const Array2D&);

    public:

        inline const uint32_t row() const {
            return rows;
        }

        inline const uint32_t col() const {
            return columns;
        }

        inline T& operator() (int row, int col) {
            return this->data[row * columns + col];
        }

        const inline T& operator() (int row, int col) const {
            return this->data[row * columns + col];
        }

        inline T* operator[] (int row) {
            return &(data[row * columns]);
        }

        inline const T* operator[] (int row) const {
            return &(data[row * columns]);
        }

        void clear(const T & value) {
            for(uint32_t i = 0; i < rows * columns; i++){
                data[i] = value;
            }
        }
    };
}

#endif
