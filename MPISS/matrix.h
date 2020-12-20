#pragma once
#include <Windows.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <vector>
#include <string>
#include <queue>
#include <cmath>
#include <map>

using line = std::vector<double>;
constexpr double EPSILON = 1.0e-10; //difference epsilon.

class matrix {
public:
	matrix();
	matrix(size_t rows, size_t cols);
	matrix(size_t size);
	matrix(const matrix& rightMatrix);
	matrix(const std::initializer_list<std::vector<double>>& list);
	matrix(const std::vector<line>& list);
	static matrix E_matrix(size_t size);
	static matrix Diagonal(const line& diag_values);
	inline ~matrix() {};

	inline std::pair<size_t, size_t> size() const;
	inline size_t rows() const;
	inline size_t cols() const;
	inline void resize(size_t newRows, size_t newCols);
	inline void swap(matrix& rightMatrix);

	inline double& at(size_t x, size_t y);
	inline const double& at(size_t x, size_t y) const;
	inline line& operator[](size_t rows);
	inline const line& operator[](size_t rows) const;

	inline matrix get_row(size_t row) const;
	inline matrix get_col(size_t col) const;
	inline matrix set_row(size_t row, const matrix& mx_row);
	inline matrix set_col(size_t col, const matrix& mx_col);

	inline matrix& operator=(const matrix& rightMatrix);
	inline matrix operator*(double a) const;
	inline matrix operator*=(double a);
	inline matrix operator+(const matrix& rightMatrix) const;
	inline matrix operator-(const matrix& rightMatrix) const;
	inline matrix operator+=(const matrix& rightMatrix);
	inline matrix operator-=(const matrix& rightMatrix);
	inline matrix operator/(double a) const;
	inline matrix operator/=(double a);
	inline matrix operator*(const matrix& rightMatrix) const;
	inline matrix operator^(int64_t degree) const;
	inline bool operator==(const matrix& rightMatrix) const;

	inline matrix ppow(double p) const;
	inline matrix sppow(double p);
	inline matrix apply(const std::function<void(double&)>& func) const;
	inline matrix sapply(const std::function<void(double&)>& func);
	inline double psum() const;
	inline matrix pabs() const;
	inline matrix spabs();
	inline void normalize(double p = 2);
	inline double norma(double p = 2) const;

	inline matrix transpose() const;
	inline double trace() const;
	inline matrix inverse() const;
	inline double determinant() const;
	friend inline std::ostream& operator<<(std::ostream& out, const matrix& rightMatrix);
	friend inline std::istream& operator>>(std::istream& in, matrix& rightMatrix);
	friend inline matrix operator*(double a, const matrix& rightMatrix);

	inline matrix minor_matrix(const size_t& x_minor, const size_t& y_minor) const;

	inline static matrix cross_prod(const matrix& points_in_matrix);

	inline bool operator<(const matrix& comparation_m) const;
	inline bool operator<=(const matrix& comparation_m) const;
private:
	std::vector<line> _matrix;
	double utilization = 0;
};

matrix::matrix() : _matrix(1, line(1, 0.)) {}

matrix::matrix(size_t rows, size_t cols) {
	_matrix.assign(rows, line(cols, 0.));
}

matrix::matrix(size_t size) {
	_matrix.assign(size, line(size, 0.));
}

matrix::matrix(const matrix& rightMatrix) {
	_matrix = rightMatrix._matrix;
}

inline matrix::matrix(const std::initializer_list<std::vector<double>>& list) {
	for (auto&& line : list) {
		_matrix.push_back(line);
	}
}
matrix::matrix(const std::vector<line>& list) {
	for (auto&& line : list) {
		_matrix.push_back(line);
	}
}

matrix matrix::E_matrix(size_t size) {
	matrix unitMatrix(size);
	for (int i = 0; i < size; i++)
		unitMatrix._matrix[i][i] = 1.;
	return unitMatrix;
}

matrix matrix::Diagonal(const line& diagValues) {
	matrix diagMatrix(diagValues.size());
	for (int i = 0; i < diagValues.size(); i++)
		diagMatrix._matrix[i][i] = diagValues[i];
	return diagMatrix;
}

inline size_t matrix::rows() const {
	return _matrix.size();
}
inline size_t matrix::cols() const {
	if (_matrix.size())
		return _matrix.front().size();
	return 0;
}
inline std::pair<size_t, size_t> matrix::size() const {
	return { rows(),cols() };
}

inline void matrix::resize(size_t newRows, size_t newCols) {
	for (auto& l : _matrix) {
		l.resize(newCols, 0);
	}
	_matrix.resize(newRows, line(newCols));
}

inline void matrix::swap(matrix& rightMatrix) {
	_matrix.swap(rightMatrix._matrix);
}

inline double& matrix::at(size_t x, size_t y) {
	if (x < cols() && y < rows()) {
		return _matrix[y][x];
	}
	else
		return utilization;
}

inline const double& matrix::at(size_t x, size_t y) const {
	if (x < cols() && y < rows()) {
		return _matrix[y][x];
	}
	else
		return utilization;
}

inline line& matrix::operator[](size_t rows) {
	return _matrix[rows];
}

inline const line& matrix::operator[](size_t rows) const {
	return _matrix[rows];
}

inline matrix& matrix::operator=(const matrix& rightMatrix) {
	_matrix = rightMatrix._matrix;
	return *this;
}

inline matrix matrix::operator*(double Num) const {
	matrix prodMatrix(*this);
	for (auto&& l : prodMatrix._matrix) {
		for (auto&& d : l) {
			d *= Num;
		}
	}
	return prodMatrix;
}

inline matrix matrix::operator*=(double Num) {
	for (auto&& l : _matrix) {
		for (auto&& d : l) {
			d *= Num;
		}
	}
	return *this;
}

inline matrix matrix::operator/=(double Num) {
	for (auto&& l : _matrix) {
		for (auto&& d : l) {
			d /= Num;
		}
	}
	return *this;
}

inline matrix matrix::operator+(const matrix& rightMatrix) const {
	matrix sumMatrix(*this);
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			sumMatrix.at(x, y) += rightMatrix.at(x, y);
		}
	}
	return sumMatrix;
}

inline matrix matrix::operator+=(const matrix& rightMatrix) {
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			at(x, y) += rightMatrix.at(x, y);
		}
	}
	return (*this);
} 

inline matrix matrix::operator-=(const matrix& rightMatrix) {
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			at(x, y) -= rightMatrix.at(x, y);
		}
	}
	return (*this);
}

inline matrix matrix::operator-(const matrix& rightMatrix) const {
	matrix diffMatrix(*this);
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			diffMatrix.at(x, y) -= rightMatrix.at(x, y);
		}
	}
	return diffMatrix;
}

inline matrix matrix::operator/(double Number) const {
	return (*this * (1. / Number));
}

inline matrix matrix::operator*(const matrix& rightMatrix) const {
	if (cols() != rightMatrix.rows())
		return matrix();
	matrix prodMatrix(rows(), rightMatrix.cols());
	for (size_t y = 0; y < prodMatrix.rows(); y++) {
		for (size_t x = 0; x < prodMatrix.cols(); x++) {
			for (size_t i = 0; i < cols(); i++) {
				prodMatrix[y][x] += _matrix[y][i] * rightMatrix[i][x];
			}
		}
	}
	return prodMatrix;
}

inline matrix matrix::operator^(int64_t degree) const {
	if (rows() != cols())
		return matrix();
	bool inverse = false;
	if (degree < 0) {
		inverse = true;
		degree = -degree;
		//std::cout << a;
	}
	else if (!degree)
		return matrix::E_matrix(rows());
	matrix curMatrix = matrix::E_matrix(rows()), degCoMatrix = *this;
	while (degree) {
		switch (degree & 1) {
		case 1:
			curMatrix = curMatrix * degCoMatrix;
			degCoMatrix = degCoMatrix * degCoMatrix;
			break;
		case 0:
			degCoMatrix = degCoMatrix * degCoMatrix;
			break;
		}
		degree >>= 1;
	}
	return inverse ? curMatrix.inverse() : curMatrix;
}

inline matrix matrix::transpose() const {
	matrix transposedMatix(cols(), rows());
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			transposedMatix[x][y] = _matrix[y][x];
		}
	}
	return transposedMatix;
}

inline double matrix::trace() const {
	double sum = 0;
	for (size_t i = 0; i < rows(); i++) {
		sum += at(i, i);
	}
	return sum;
}

inline double matrix::determinant() const {//gauss
	if (rows() != cols())
		return 0;
	double multiplier = 1, max = 0;
	matrix thisMatrix(*this);
	size_t maxID = 0;
	for (size_t curColumn = 0; curColumn < rows(); curColumn++) {
		maxID = curColumn;
		max = 0;
		for (size_t curRow = maxID; curRow < rows(); curRow++) {
			if (abs(thisMatrix[curRow][curColumn]) > max) {
				max = abs(thisMatrix[curRow][curColumn]);
				maxID = curRow;
			}
		}
		if (abs(thisMatrix[maxID][curColumn]) >= EPSILON)
			thisMatrix[maxID].swap(thisMatrix[curColumn]);
		else
			return 0;
		for (size_t curRow = 0; curRow < rows(); curRow++) {
			if (curRow == curColumn)
				continue;
			double rowMultiplier = -1 * (thisMatrix[curRow][curColumn] / thisMatrix[curColumn][curColumn]);
			for (size_t sumColumn = curColumn; sumColumn < cols(); sumColumn++) {
				thisMatrix[curRow][sumColumn] += rowMultiplier * thisMatrix[curColumn][sumColumn];
			}
		}
	}
	for (size_t i = 0; i < rows(); i++)
		multiplier *= thisMatrix[i][i];
	return multiplier;
}

inline matrix matrix::inverse() const {
	if (rows() != cols())
		return matrix();
	matrix thisMatrix(*this);
	matrix finalMatrix;
	finalMatrix.resize(rows(), rows());
	double max = 0;
	thisMatrix.resize(rows(), rows() * 2);
	for (size_t i = 0; i < rows(); i++)
		thisMatrix[i][i + rows()] = 1;
	size_t maxID = 0;
	for (size_t curColumn = 0; curColumn < rows(); curColumn++) {
		max = 0;
		maxID = curColumn;
		for (size_t curRow = maxID; curRow < rows(); curRow++) {
			if (abs(thisMatrix[curRow][curColumn]) > max) {
				max = abs(thisMatrix[curRow][curColumn]);
				maxID = curRow;
			}
		}
		if (abs(thisMatrix[maxID][curColumn]) >= EPSILON)
			thisMatrix[maxID].swap(thisMatrix[curColumn]);
		else
			return matrix(rows(), rows());
		for (size_t curRow = 0; curRow < rows(); curRow++) {
			if (curRow == curColumn || abs(thisMatrix[curRow][curColumn]) < EPSILON)
				continue;
			//std::cout << M << '\n';
			double rowMultiplier = -1 * (thisMatrix[curRow][curColumn] / thisMatrix[curColumn][curColumn]);
			for (size_t sumColumn = curColumn; sumColumn < thisMatrix.cols(); sumColumn++) {
				thisMatrix[curRow][sumColumn] += rowMultiplier * thisMatrix[curColumn][sumColumn];
			}
		}
	}
	for (size_t i = 0; i < rows(); i++) {
		double multiplier = 1 / thisMatrix[i][i];
		for (size_t col = i; col < 2 * rows(); col++) {
			thisMatrix[i][col] *= multiplier;
		}
	}
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			if (abs(thisMatrix[y][x + rows()]) < EPSILON)
				thisMatrix[y][x + rows()] = abs(thisMatrix[y][x + rows()]);
			finalMatrix[y][x] = thisMatrix[y][x + rows()];
		}
	}
	return finalMatrix;
}

inline matrix matrix::minor_matrix(const size_t& x_minor, const size_t& y_minor) const {
	auto minor_index = [](size_t x, size_t y, size_t minor_x, size_t minor_y) -> std::pair<int64_t, int64_t> {
		if (x == minor_x)
			return { -1,-1 };
		if (y == minor_y)
			return { -1,-1 };
		if (x > minor_x)
			x -= 1;
		if (y > minor_y)
			y -= 1;
		return { x,y };
	};
	matrix M;
	M.resize(rows() - 1, cols() - 1);
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			auto mxy = minor_index(x, y, x_minor, y_minor);
			int64_t mx = mxy.first;
			int64_t my = mxy.second;
			if (mx < 0 || my < 0)
				continue;
			M.at(mx, my) = at(x, y);
		}
	}
	return M;
}

inline matrix matrix::cross_prod(const matrix& points_in_matrix) {
	size_t dims = points_in_matrix.rows(); /* ex: [1,2] */
	if (points_in_matrix.rows() + 1 != points_in_matrix.cols())
		return {};
	if (dims > 0) {
		matrix answer;
		matrix mx = points_in_matrix;
		answer.resize(1, dims + 1);
		mx.resize(dims + 1, dims + 1);
		for (size_t i = dims; i >= 1; i--)
			mx[i] = mx[i - 1];
		for (size_t i = 0; i < dims+1; i++)
			answer.at(i, 0) = (mx.minor_matrix(i, 0).determinant() * ((i & 1) ? (1.) : (-1.)));
		return answer;
	}
	else
		return {};
}

inline matrix matrix::get_row(size_t row) const {
	return matrix({ this->operator[](row) });
}

inline matrix matrix::get_col(size_t col) const {
	matrix new_mx;
	new_mx.resize(this->rows(), 1);
	for (size_t i = 0; i < new_mx.rows(); i++)
		new_mx[i][0] = at(col, i);
	return new_mx;
}

inline matrix matrix::set_row(size_t row, const matrix& mx_row) {
	if (cols() != mx_row.cols() || row >= rows())
		return *this;
	_matrix[row] = mx_row[0];
	return *this;
}

inline matrix matrix::set_col(size_t col, const matrix& mx_col) {
	if (rows() != mx_col.rows() || col >= cols())
		return *this;
	for (size_t i = 0; i < mx_col.rows(); i++)
		_matrix[i][0] = mx_col[i][0];
	return *this;
}

inline matrix operator*(double number, const matrix& rightMatrix) {
	return rightMatrix * number;
}

inline bool matrix::operator==(const matrix& rightMatrix) const {
	if (rows() != rightMatrix.rows() && cols() != rightMatrix.cols())
		return false;
	for (size_t y = 0; y < rows(); y++) {
		for (size_t x = 0; x < cols(); x++) {
			if (_matrix[y][x] != rightMatrix._matrix[y][x])
				return false;
		}
	}
	return true;
}


inline std::istream& operator>>(std::istream& in, matrix& rightMatrix) {
	size_t y, x;
	in >> y >> x;
	rightMatrix.resize(y, x);
	for (auto&& line : rightMatrix._matrix)
		for (auto&& digit : line)
			in >> digit;
	return in;
}

inline std::ostream& operator<<(std::ostream& out, const matrix& rightMatrix) {
	for (auto&& line : rightMatrix._matrix) {
		for (auto&& digit : line) {
			out << std::setprecision(6) << std::setw(10) << digit;
		}
		out << '\n';
	}
	return out;
}

inline bool matrix::operator<(const matrix& comparation_m) const {
	if (rows() != comparation_m.rows() || cols() != comparation_m.cols())
		throw std::runtime_error("matrix sizes mismatch on inequality comparison");
	for (size_t y = 0; y < rows(); y++)
		for (size_t x = 0; x < cols(); x++)
			if (at(x, y) >= comparation_m.at(x, y))
				return false;
	return true;
}

inline bool matrix::operator<=(const matrix& comparation_m) const {
	if (rows() != comparation_m.rows() || cols() != comparation_m.cols())
		throw std::runtime_error("matrix sizes mismatch on inequality comparison");
	for (size_t y = 0; y < rows(); y++)
		for (size_t x = 0; x < cols(); x++)
			if (at(x, y) > comparation_m.at(x, y))
				return false;
	return true;
}

inline matrix matrix::ppow(double p) const {
	matrix mx(*this);
	mx.sppow(p);
	return mx;
}

inline matrix matrix::sppow(double p) {
	for (auto& line : _matrix)
		for (auto& val : line)
			val = std::pow(val, p);
	return *this;
}

inline matrix matrix::pabs() const {
	matrix mx(*this);
	mx.spabs();
	return mx;
}

inline matrix matrix::spabs() {
	for (auto& line : _matrix)
		for (auto& val : line)
			val = std::abs(val);
	return *this;
}

inline double matrix::psum() const {
	double s = 0;
	for (auto& line : _matrix)
		for (auto& val : line)
			s += val;
	return s;
}

inline matrix matrix::sapply(const std::function<void(double&)>& func) {
	for (auto& line : _matrix)
		for (auto& val : line)
			func(val);
	return *this;
}

inline matrix matrix::apply(const std::function<void(double&)>& func) const {
	matrix mx(*this);
	mx.sapply(func);
	return mx;
}

inline void matrix::normalize(double p) {
	*this /= std::pow(ppow(2).psum(), 1 / p);
}

inline double matrix::norma(double p) const {
	double sum = 0;
	for (auto& line : _matrix)
		for (auto& val : line)
			sum += std::pow(std::abs(val),p);
	return std::pow(sum, 1. / p);
}