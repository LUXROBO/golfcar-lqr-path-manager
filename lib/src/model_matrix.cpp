#include "model_matrix.h"

#include <cmath>


// ModelMatrix
ModelMatrix::ModelMatrix()
    : row_(4), column_(4) {
    element_.resize(row_ * column_);
}

ModelMatrix::ModelMatrix(const ModelMatrix &other)
    : row_(other.row()), column_(other.column()) {
    element_ = other.element();
}

ModelMatrix::ModelMatrix(const unsigned int row, const unsigned int column)
    : row_(row), column_(column) {
    element_.resize(row_ * column_);
}

ModelMatrix::ModelMatrix(const unsigned int row, const unsigned int column, const q_format_c *element)
    : row_(row), column_(column), element_(element, element + (row * column)) {
}

ModelMatrix::ModelMatrix(const unsigned int row, const unsigned int column, const q_format_c **element)
    : row_(row), column_(column) {
    element_.resize(row_ * column_);

    for (unsigned int r = 0; r < row; r++) {
        for (unsigned int c = 0; c < column; c++) {
            element_[r * column + c] = element[r][c];
        }
    }
}

ModelMatrix::ModelMatrix(const unsigned int row, const unsigned int column, const std::vector<q_format_c> element)
    : row_(row), column_(column), element_(element) {
}

ModelMatrix::~ModelMatrix() {
    element_.clear();
}

unsigned int ModelMatrix::row() const {
    return row_;
}

unsigned int ModelMatrix::column() const {
    return column_;
}

std::vector<q_format_c> ModelMatrix::element() const {
    return element_;
}

q_format_c ModelMatrix::get(const unsigned int row, const unsigned int column) const {
    if (row > row_) {
        return q_format_c();
    } else if (column > column_) {
        return q_format_c();
    } else {
        return element_[row * column_ + column];
    }
}

void ModelMatrix::set(const unsigned int row, const unsigned int column, const q_format_c value) {
    if (row > row_) {
        return;
    } else if (column > column_) {
        return;
    } else {
        element_[row * column_ + column] = value;
    }
}

ModelMatrix ModelMatrix::zero(const unsigned int row, const unsigned int column) {
    return ModelMatrix(row, column);
}

ModelMatrix ModelMatrix::one(const unsigned int row, const unsigned int column) {
    std::vector<q_format_c> mat(row * column);
    for (unsigned int r = 0; r < row; r++) {
        for (unsigned int c = 0; c < column; c++) {
            // mat[r * column + c] = 1.0;
            mat[r * column + c] = Q_FORMAT_ONE;
        }
    }
    return ModelMatrix(row, column, mat);
}

ModelMatrix ModelMatrix::identity(const unsigned int row, const unsigned int column) {
    std::vector<q_format_c> mat(row * column);
    for (unsigned int r = 0; r < row; r++) {
        for (unsigned int c = 0; c < column; c++) {
            if (r == c) {
                // mat[r * column + c] = 1.0;
                mat[r * column + c] = Q_FORMAT_ONE;
            } else {
                // mat[r * column + c] = 0.0;
                mat[r * column + c] = 0;
            }
        }
    }
    return ModelMatrix(row, column, mat);
}

ModelMatrix ModelMatrix::transpose() {
    std::vector<q_format_c> ele(row_ * column_);
    for (unsigned int r = 0; r < row_; r++) {
        for (unsigned int c = 0; c < column_; c++) {
            ele[c * row_ + r] = element_[r * column_ + c];
        }
    }
    return ModelMatrix(column_, row_, ele);
}

q_format_c ModelMatrix::determinant() {
    if (row_ == column_) {
        return determinant(element_, row_);
    } else if (row_ > column_) {
        return determinant((this->transpose() * (*this)).element(), column_);
    } else {
        return determinant(((*this) * this->transpose()).element(), row_);
    }
}

ModelMatrix ModelMatrix::inverse() {
    if (row_ == column_) {
        // square matrix
        return ModelMatrix(row_, column_,  matrixInversion(element_, row_));
    } else {
        // rectangular matrix
        return pseudoInverse();
    }
}

ModelMatrix ModelMatrix::inverse(const q_format_c sigma) {
    if (row_ <= column_) {
        // m by n matrix (n >= m)
        // generate sigma digonal matrix
        ModelMatrix temp = ModelMatrix::identity(row_, row_) * sigma;
        // calculation of inverse matrix
        ModelMatrix temp2 = (*this) * (this->transpose());
        ModelMatrix temp3 = temp2 + temp;
        return this->transpose() * temp3.inverse();
    } else {
        // generate sigma digonal matrix
        ModelMatrix temp = ModelMatrix::identity(row_, row_) * sigma;
        // calculation of inverse matrix
        return (this->transpose() * (*this) + temp).inverse() * this->transpose();
    }
}

q_format_c ModelMatrix::length() const {
    q_format_c l = 0;
    for (unsigned int r = 0; r < row_; r++) {
        for (unsigned int c = 0; c < column_; c++) {
            l = l + q_format_mult(element_[r * column_ + c], element_[r * column_ + c]);
        }
    }
    return to_q_format(std::sqrt(to_double(l)));
}

ModelMatrix ModelMatrix::normalize() const {
    q_format_c l = length();
    if (l == 0) {
        return ModelMatrix::identity(row_, column_);
    } else {
        std::vector<q_format_c> ele(row_, column_);
        for (unsigned int r = 0; r < row_; r++) {
            for (unsigned int c = 0; c < column_; c++) {
                ele[r * column_ + c] = q_format_div(element_[r * column_ + c], l);
            }
        }
        return ModelMatrix(row_, column_, ele);
    }
}

q_format_c ModelMatrix::dot(const ModelMatrix &rhs) {
    if (row_ == rhs.row() && column_ == rhs.column()) {
        q_format_c dot = 0;
        for (unsigned int r = 0; r < row_; r++) {
            for (unsigned int c = 0; c < column_; c++) {
                dot += q_format_mult(element_[r * column_ + c], rhs.element()[r * column_ + c]);
            }
        }
        return dot;
    } else {
        return 0;
    }
}

ModelMatrix ModelMatrix::cross(const ModelMatrix &rhs) {
    if (row_ == 3 && column_ == 1 && rhs.row() == 3 && rhs.column() == 1) {
        std::vector<q_format_c> ele(3);
        ele[0] = q_format_mult(element_[1], rhs.element()[2]) - q_format_mult(element_[2], rhs.element()[1]);
        ele[1] = q_format_mult(element_[2], rhs.element()[0]) - q_format_mult(element_[0], rhs.element()[2]);
        ele[2] = q_format_mult(element_[0], rhs.element()[1]) - q_format_mult(element_[1], rhs.element()[0]);
        return ModelMatrix(row_, column_, ele);
    } else {
        return ModelMatrix::zero(3, 1);
    }
}

ModelMatrix ModelMatrix::cross() {
    if (row_ == 3 && column_ == 1) {
        std::vector<q_format_c> ele(3 * 3);
        ele[0 * 3 + 0] = 0;
        ele[0 * 3 + 1] = -element_[2];
        ele[0 * 3 + 2] = element_[1];
        ele[1 * 3 + 0] = element_[2];
        ele[1 * 3 + 1] = 0;
        ele[1 * 3 + 2] = -element_[0];
        ele[2 * 3 + 0] = -element_[1];
        ele[2 * 3 + 1] = element_[0];
        ele[2 * 3 + 2] = 0;
        return ModelMatrix(3, 3, ele);
    } else {
        return ModelMatrix::zero(3, 3);
    }
}

ModelMatrix &ModelMatrix::operator=(const ModelMatrix &other) {
    this->row_ = other.row_;
    this->column_ = other.column();
    this->element_ = other.element();
    return *this;
}

ModelMatrix ModelMatrix::operator+(const q_format_c &rhs) {
    ModelMatrix right = ModelMatrix::one(row_, column_) * rhs;
	return (*this) + right;
}

ModelMatrix ModelMatrix::operator+(const ModelMatrix &rhs) {
    if (row_ == rhs.row() && column_ == rhs.column()) {
        std::vector<q_format_c> temp(row_ * column_);
        for (unsigned int r = 0; r < row_; r++) {
            for (unsigned int c = 0; c < column_; c++) {
                temp[r * column_ + c] = element_[r * column_ + c] + rhs.element()[r * column_ + c];
            }
        }
        return ModelMatrix(row_, column_, temp);
    } else {
        return ModelMatrix::zero(row_, column_);
    }
}

ModelMatrix ModelMatrix::operator-(const q_format_c &rhs) {
    ModelMatrix right = ModelMatrix::one(row_, column_) * rhs;
	return (*this) - right;
}

ModelMatrix ModelMatrix::operator-(const ModelMatrix &rhs) {
    if (row_ == rhs.row() && column_ == rhs.column()) {
        std::vector<q_format_c> temp(row_ * column_);
        for (unsigned int r = 0; r < row_; r++) {
            for (unsigned int c = 0; c < column_; c++) {
                temp[r * column_ + c] = element_[r * column_ + c] - rhs.element()[r * column_ + c];
            }
        }
        return ModelMatrix(row_, column_, temp);
    } else {
        return ModelMatrix::zero(row_, column_);
    }
}

ModelMatrix ModelMatrix::operator*(const q_format_c &rhs) {
    std::vector<q_format_c> temp(row_ * column_);
    for (unsigned int r = 0; r < row_; r++) {
        for (unsigned int c = 0; c < column_; c++) {
            temp[r * column_ + c] = q_format_mult(element_[r * column_ + c], rhs);
        }
    }
    return ModelMatrix(row_, column_, temp);
}

ModelMatrix ModelMatrix::operator*(const ModelMatrix &rhs) {
    if (column_ == rhs.row()) {
		std::vector<q_format_c> temp(row_ * rhs.column());
        for (unsigned int r = 0; r < row_; r++) {
            for (unsigned int c = 0; c < rhs.column(); c++) {
                temp[r * rhs.column() + c] = 0;
                for (unsigned int k = 0; k < column_; k++) {
                    temp[r * rhs.column() + c] += q_format_mult(element_[r * column_ + k], rhs.element()[k * rhs.column() + c]);
                }
            }
        }
        return ModelMatrix(row_, rhs.column(), temp);
    } else {
		return ModelMatrix::zero(row_, column_);
    }
}

ModelMatrix operator+(const q_format_c &lhs, const ModelMatrix &rhs) {
    ModelMatrix left = ModelMatrix::one(rhs.row(), rhs.column()) * lhs;
    return left + rhs;
}

ModelMatrix operator-(const q_format_c &lhs, const ModelMatrix &rhs) {
    ModelMatrix left = ModelMatrix::one(rhs.row(), rhs.column()) * lhs;
    return left - rhs;
}

ModelMatrix operator*(const q_format_c &lhs, const ModelMatrix &rhs) {
    std::vector<q_format_c> temp(rhs.row() * rhs.column());
    for (unsigned int r = 0; r < rhs.row(); r++) {
        for (unsigned int c = 0; c < rhs.column(); c++) {
            temp[r * rhs.column() + c] = q_format_mult(rhs.element()[r * rhs.column() + c], lhs);
        }
    }
    return ModelMatrix(rhs.row(), rhs.column(), temp);
}

ModelMatrix ModelMatrix::pseudoInverse() {
    if (row_ == column_) {
        return inverse();
    } else if (row_ > column_) {
        return pseudoInverseL();
    } else {
        return pseudoInverseR();
    }
}

ModelMatrix ModelMatrix::pseudoInverseR() {
    return this->transpose() * ((*this) * (this->transpose())).inverse();
}

ModelMatrix ModelMatrix::pseudoInverseL() {
    return ((this->transpose()) * (*this)).inverse() * this->transpose();
}

q_format_c ModelMatrix::determinant(std::vector<q_format_c> matrix, int order) {
    // the determinant value
    q_format_c det = Q_FORMAT_ONE;

    // stop the recursion when matrix is a single element
    if (order == 1) {
        det = matrix[0];
    } else if (order == 2) {
        det = q_format_mult(matrix[0 * 2 + 0], matrix[1 * 2 + 1]) - q_format_mult(matrix[0 * 2 + 1], matrix[1 * 2 + 0]);
    } else if (order == 3) {
        det = q_format_mult(q_format_mult(matrix[0 * 3 + 0], matrix[1 * 3 + 1]), matrix[2 * 3 + 2])
             + q_format_mult(q_format_mult(matrix[0 * 3 + 1], matrix[1 * 3 + 2]), matrix[2 * 3 + 0])
             + q_format_mult(q_format_mult(matrix[0 * 3 + 2], matrix[1 * 3 + 0]), matrix[2 * 3 + 1])
             - q_format_mult(q_format_mult(matrix[0 * 3 + 0], matrix[1 * 3 + 2]), matrix[2 * 3 + 1])
             - q_format_mult(q_format_mult(matrix[0 * 3 + 1], matrix[1 * 3 + 0]), matrix[2 * 3 + 2])
             - q_format_mult(q_format_mult(matrix[0 * 3 + 2], matrix[1 * 3 + 1]), matrix[2 * 3 + 0]);
    } else {
        // generation of temporary matrix
        std::vector<q_format_c> temp_matrix = matrix;

        // gaussian elimination
        for (int i = 0; i < order; i++) {
            // find max low
            q_format_c temp = 0;
            int max_row = i;
            for (int j = i; j < order; j++) {
                if (abs(temp_matrix[j * order + i]) > temp) {
                    temp = abs(temp_matrix[j * order + i]);
                    max_row = j;
                }
            }
            if (abs(temp_matrix[max_row * order + i]) > to_q_format(0.0001)) {
                // transfer row
                if (max_row != i) {
                    for (int j = 0; j < order; j++) {
                        temp -= temp_matrix[max_row * order + j];
                        temp_matrix[max_row * order + j] = temp_matrix[i * order + j];
                        temp_matrix[i * order + j] = temp;
                    }
                }
                // elemination
                for (int j = i + 1; j < order; j++) {
                    temp = q_format_div(temp_matrix[j * order + i], temp_matrix[i * order + i]);
                    for (int k = i; k < order; k++) {
                        temp_matrix[j * order + k] = temp_matrix[j * order + k] - q_format_mult(temp_matrix[i * order + k], temp);
                    }
                }
            }
        }

        for (int i = 0; i < order; i++) {
            det = q_format_mult(det, temp_matrix[i * order + i]);
        }
    }

    return det;
}

std::vector<q_format_c> ModelMatrix::matrixInversion(std::vector<q_format_c> matrix, int order) {
    std::vector<q_format_c> matA = matrix;
    std::vector<q_format_c> matB = ModelMatrix::identity(order, order).element();

    // Gauss-Jordan
    // Forward
    for (int i = 0; i < order; i++) {
        // max row
        q_format_c temp = 0.000;
        int max_row = i;
        for (int j = i; j < order; j++) {
            if (abs(matA[j * order + i]) > temp) {
                temp = abs(matA[j * order + i]);
                max_row = j;
            }
        }
        // change row
        q_format_c temp2 = matA[max_row * order + i];
        for (int j = 0; j < order; j++) {
            temp = matA[max_row * order + j];
            matA[max_row * order + j] = matA[i * order + j];
            matA[i * order + j] = q_format_div(temp, temp2);

            temp = matB[max_row * order + j];
            matB[max_row * order + j] = matB[i * order + j];
            matB[i * order + j] = q_format_div(temp, temp2);
        }
        for (int j = i + 1; j < order; j++) {
            temp = matA[j * order + i];
            for (int k = 0; k < order; k++) {
                matA[j * order + k] -= q_format_mult(matA[i * order + k], temp);
                matB[j * order + k] -= q_format_mult(matB[i * order + k], temp);
            }
        }
    }

    //Backward
    for (int i = order - 1; i >= 0; i--) {
        for (int j = i - 1; j >= 0; j--) {
            q_format_c temp = matA[j * order + i];
            for (int k = 0; k < order; k++) {
                matA[j * order + k] -= q_format_mult(matA[i * order + k], temp);
                matB[j * order + k] -= q_format_mult(matB[i * order + k], temp);
            }
        }
    }

    return matB;
}
